#include "pch.h"
#include "ImagePreviewCtrl.h"
#include <compressapi.h>
#pragma comment(lib, "Cabinet.lib")

BEGIN_MESSAGE_MAP(CImagePreviewCtrl, CStatic)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// ── OOXML ZIP 썸네일 추출 헬퍼 ───────────────────────────────────

static std::vector<BYTE> ReadFileBytes(const CString& path)
{
    HANDLE h = CreateFile(path, GENERIC_READ, FILE_SHARE_READ,
                          nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return {};

    LARGE_INTEGER sz{};
    GetFileSizeEx(h, &sz);
    if (sz.QuadPart == 0 || sz.QuadPart > 100LL * 1024 * 1024)
    { CloseHandle(h); return {}; }

    std::vector<BYTE> buf((size_t)sz.QuadPart);
    DWORD read = 0;
    ReadFile(h, buf.data(), (DWORD)buf.size(), &read, nullptr);
    CloseHandle(h);
    return (read == buf.size()) ? buf : std::vector<BYTE>{};
}

struct ZipEntry { WORD method; DWORD compSize, uncompSize, dataOffset; };

static bool FindZipEntry(const std::vector<BYTE>& d, const char* target, ZipEntry& out)
{
    if (d.size() < 22) return false;

    // End-of-Central-Directory 검색 (파일 끝에서 역방향)
    const DWORD EOCD = 0x06054b50;
    int eocd = -1;
    for (int i = (int)d.size() - 22; i >= max(0, (int)d.size() - 22 - 65535); --i)
        if (*(const DWORD*)&d[i] == EOCD) { eocd = i; break; }
    if (eocd < 0) return false;

    WORD  total    = *(const WORD*) &d[eocd + 10];
    DWORD cdOffset = *(const DWORD*)&d[eocd + 16];

    std::string tgt(target);
    std::transform(tgt.begin(), tgt.end(), tgt.begin(), ::tolower);

    const DWORD CD_SIG = 0x02014b50;
    int pos = (int)cdOffset;
    for (int i = 0; i < total; ++i)
    {
        if (pos + 46 > (int)d.size() || *(const DWORD*)&d[pos] != CD_SIG) break;

        WORD  method   = *(const WORD*) &d[pos + 10];
        DWORD csz      = *(const DWORD*)&d[pos + 20];
        DWORD usz      = *(const DWORD*)&d[pos + 24];
        WORD  nameLen  = *(const WORD*) &d[pos + 28];
        WORD  extraLen = *(const WORD*) &d[pos + 30];
        WORD  cmtLen   = *(const WORD*) &d[pos + 32];
        DWORD localOff = *(const DWORD*)&d[pos + 42];

        if (pos + 46 + nameLen <= (int)d.size())
        {
            std::string name((const char*)&d[pos + 46], nameLen);
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            if (name == tgt)
            {
                int lh = (int)localOff;
                if (lh + 30 <= (int)d.size())
                {
                    WORD lnl = *(const WORD*)&d[lh + 26];
                    WORD lel = *(const WORD*)&d[lh + 28];
                    DWORD ds = localOff + 30 + lnl + lel;
                    if (ds + csz <= d.size())
                    {
                        out = { method, csz, usz, ds };
                        return true;
                    }
                }
            }
        }
        pos += 46 + nameLen + extraLen + cmtLen;
    }
    return false;
}

// OOXML(docx/pptx) 파일에서 docProps/thumbnail.* 를 추출해 반환
static std::vector<BYTE> ExtractOoxmlThumbnail(const CString& path)
{
    auto d = ReadFileBytes(path);
    if (d.size() < 4 || *(const DWORD*)d.data() != 0x04034b50) return {};

    const char* candidates[] = {
        "docprops/thumbnail.jpeg",
        "docprops/thumbnail.jpg",
        "docprops/thumbnail.png",
        "docprops/thumbnail.wmf",
    };

    ZipEntry e{};
    bool found = false;
    for (auto c : candidates)
        if (FindZipEntry(d, c, e)) { found = true; break; }
    if (!found) return {};

    // Method 0: Stored — 그대로 반환
    if (e.method == 0)
        return std::vector<BYTE>(d.begin() + e.dataOffset,
                                  d.begin() + e.dataOffset + e.compSize);

    // Method 8: Deflate — "CK" 헤더 붙여 MSZIP으로 해제
    if (e.method == 8)
    {
        std::vector<BYTE> mszip(2 + e.compSize);
        mszip[0] = 'C'; mszip[1] = 'K';
        memcpy(&mszip[2], &d[e.dataOffset], e.compSize);

        std::vector<BYTE> result(e.uncompSize);
        DECOMPRESSOR_HANDLE hD = nullptr;
        if (CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, nullptr, &hD))
        {
            SIZE_T outLen = 0;
            bool ok = Decompress(hD, mszip.data(), mszip.size(),
                                  result.data(), e.uncompSize, &outLen) != 0;
            CloseDecompressor(hD);
            if (ok) { result.resize(outLen); return result; }
        }
    }

    return {};
}

static Gdiplus::Bitmap* BitmapFromBytes(const std::vector<BYTE>& bytes)
{
    if (bytes.empty()) return nullptr;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (!hMem) return nullptr;
    memcpy(GlobalLock(hMem), bytes.data(), bytes.size());
    GlobalUnlock(hMem);
    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream)))
    { GlobalFree(hMem); return nullptr; }
    auto* bmp = Gdiplus::Bitmap::FromStream(pStream);
    pStream->Release();
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) { delete bmp; return nullptr; }
    return bmp;
}

// ── CImagePreviewCtrl ────────────────────────────────────────────

void CImagePreviewCtrl::LoadImage(const CString& path)
{
    m_bitmap.reset();
    m_bLoadFailed = false;

    m_bitmap.reset(Gdiplus::Bitmap::FromFile(path));
    if (!m_bitmap || m_bitmap->GetLastStatus() != Gdiplus::Ok)
    {
        m_bitmap.reset();
        m_bLoadFailed = true;
    }
    Invalidate();
}

void CImagePreviewCtrl::LoadShellThumbnail(const CString& path)
{
    m_bitmap.reset();
    m_bLoadFailed = false;

    // 1순위: OOXML 임베디드 썸네일 (docProps/thumbnail.*)
    auto thumbBytes = ExtractOoxmlThumbnail(path);
    if (!thumbBytes.empty())
        m_bitmap.reset(BitmapFromBytes(thumbBytes));

    // 2순위: Shell IShellItemImageFactory (OS 썸네일 캐시 → 아이콘 폴백)
    if (!m_bitmap || m_bitmap->GetLastStatus() != Gdiplus::Ok)
    {
        m_bitmap.reset();

        IShellItemImageFactory* pFactory = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&pFactory))))
        {
            SIZE sz = { 256, 256 };
            HBITMAP hBmp = nullptr;

            HRESULT hr = pFactory->GetImage(sz, SIIGBF_RESIZETOFIT, &hBmp);
            if (FAILED(hr) || !hBmp)
            {
                if (hBmp) { DeleteObject(hBmp); hBmp = nullptr; }
                hr = pFactory->GetImage(sz, SIIGBF_ICONONLY, &hBmp);
            }
            pFactory->Release();

            if (SUCCEEDED(hr) && hBmp)
            {
                DIBSECTION ds = {};
                bool handled = false;
                if (GetObject(hBmp, sizeof(ds), &ds) == sizeof(ds)
                    && ds.dsBm.bmBits && ds.dsBm.bmBitsPixel == 32)
                {
                    int   stride = ds.dsBmih.biHeight > 0
                                    ? -(int)ds.dsBm.bmWidthBytes
                                    : (int)ds.dsBm.bmWidthBytes;
                    BYTE* pBits  = ds.dsBmih.biHeight > 0
                                    ? static_cast<BYTE*>(ds.dsBm.bmBits)
                                      + (LONG_PTR)(ds.dsBm.bmHeight - 1) * ds.dsBm.bmWidthBytes
                                    : static_cast<BYTE*>(ds.dsBm.bmBits);
                    Gdiplus::Bitmap tmp(ds.dsBm.bmWidth, ds.dsBm.bmHeight,
                                        stride, PixelFormat32bppPARGB, pBits);
                    if (tmp.GetLastStatus() == Gdiplus::Ok)
                    {
                        m_bitmap.reset(tmp.Clone(0, 0,
                            ds.dsBm.bmWidth, ds.dsBm.bmHeight, PixelFormat32bppARGB));
                        handled = true;
                    }
                }
                if (!handled)
                    m_bitmap.reset(Gdiplus::Bitmap::FromHBITMAP(hBmp, nullptr));
                DeleteObject(hBmp);
            }
        }
    }

    if (!m_bitmap || m_bitmap->GetLastStatus() != Gdiplus::Ok)
    {
        m_bitmap.reset();
        m_bLoadFailed = true;
    }
    Invalidate();
}

void CImagePreviewCtrl::Clear()
{
    m_bitmap.reset();
    m_bLoadFailed = false;
    Invalidate();
}

BOOL CImagePreviewCtrl::OnEraseBkgnd(CDC* pDC)
{
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(&rc, RGB(30, 30, 35));
    return TRUE;
}

void CImagePreviewCtrl::OnPaint()
{
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);

    dc.FillSolidRect(&rc, RGB(30, 30, 35));

    if (m_bLoadFailed || !m_bitmap)
    {
        if (m_bLoadFailed)
        {
            dc.SetTextColor(RGB(160, 80, 80));
            dc.SetBkMode(TRANSPARENT);
            dc.DrawText(_T("미리보기를 불러올 수 없습니다"),
                &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        return;
    }

    Gdiplus::Graphics g(dc.m_hDC);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    UINT imgW = m_bitmap->GetWidth();
    UINT imgH = m_bitmap->GetHeight();
    if (imgW == 0 || imgH == 0) return;

    float scale = min((float)rc.Width() / imgW, (float)rc.Height() / imgH);
    int dw = (int)(imgW * scale);
    int dh = (int)(imgH * scale);
    int dx = rc.left + (rc.Width()  - dw) / 2;
    int dy = rc.top  + (rc.Height() - dh) / 2;

    g.DrawImage(m_bitmap.get(), dx, dy, dw, dh);
}
