#include "pch.h"
#include "MdConvertDlg.h"
#include "AppLang.h"
#include "MdConverter.h"
#include "PdfWriter.h"
#include <richedit.h>

static CString ReadMdFile(const CString& path);
static std::vector<PdfWriter::ImageData> RenderMdToPdfPages(CWnd* pParent,
                                                             const std::string& rtf);

BEGIN_MESSAGE_MAP(CMdConvertDlg, CTabDlgBase)
    ON_WM_DROPFILES()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_MD_BROWSE,   &CMdConvertDlg::OnBnClickedOutputBrowse)
    ON_BN_CLICKED(IDC_RADIO_MD_HTML,    &CMdConvertDlg::OnBnClickedFormatRadio)
    ON_BN_CLICKED(IDC_RADIO_MD_PDF,     &CMdConvertDlg::OnBnClickedFormatRadio)
    ON_BN_CLICKED(IDC_RADIO_MD_HTMLPDF, &CMdConvertDlg::OnBnClickedFormatRadio)
    ON_NOTIFY(NM_CLICK,        IDC_LIST_MD, &CMdConvertDlg::OnNMClickListMd)
    ON_NOTIFY(NM_RCLICK,       IDC_LIST_MD, &CMdConvertDlg::OnNMRClickListMd)
    ON_NOTIFY(NM_DBLCLK,      IDC_LIST_MD, &CMdConvertDlg::OnNMDblclkListMd)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MD, &CMdConvertDlg::OnLvnItemChangedListMd)
END_MESSAGE_MAP()

CMdConvertDlg::CMdConvertDlg(CWnd* pParent)
    : CTabDlgBase(IDD_TAB3, pParent)
{
}

void CMdConvertDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlgBase::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_MD,             m_listMd);
    DDX_Control(pDX, IDC_LBL_MD_FORMAT,    m_lblFormat);
    DDX_Control(pDX, IDC_RADIO_MD_HTML,    m_radHtml);
    DDX_Control(pDX, IDC_RADIO_MD_PDF,     m_radPdf);
    DDX_Control(pDX, IDC_RADIO_MD_HTMLPDF, m_radHtmlPdf);
    DDX_Control(pDX, IDC_LBL_MD_OUTPUT,    m_lblOutputPath);
    DDX_Control(pDX, IDC_EDIT_MD_OUTPUT,   m_editOutput);
    DDX_Control(pDX, IDC_BTN_MD_BROWSE,    m_btnOutputBrowse);
    DDX_Control(pDX, IDC_STATIC_MD_STATUS, m_lblStatus);
}

BOOL CMdConvertDlg::OnInitDialog()
{
    CTabDlgBase::OnInitDialog();
    DragAcceptFiles(TRUE);
    SetBackgroundColor(RGB(248, 249, 252));

    m_btnOutputBrowse.SetColors(RGB(255, 255, 255), RGB(235, 243, 255), RGB(37, 99, 235));

    m_listMd.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listMd.InsertColumn(0, LS(IDS_MD_COL0), LVCFMT_LEFT,   160);
    m_listMd.InsertColumn(1, LS(IDS_MD_COL1), LVCFMT_LEFT,   160);
    m_listMd.InsertColumn(2, LS(IDS_MD_COL2), LVCFMT_LEFT,    60);

    m_radHtml.SetCheck(BST_CHECKED);

    // 플레이스홀더 Static 컨트롤 위치를 기준으로 RichEdit를 동적 생성
    {
        CWnd* pPlace = GetDlgItem(IDC_STATIC_MD_PREVIEW);
        ASSERT(pPlace);
        pPlace->GetWindowRect(&m_rcPreview0);
        ScreenToClient(&m_rcPreview0);
        pPlace->DestroyWindow();
    }
    m_preview.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        m_rcPreview0, this, IDC_STATIC_MD_PREVIEW);
    m_preview.SetBackgroundColor(FALSE, RGB(248, 249, 250));
    if (CFont* pFont = GetFont()) m_preview.SetFont(pFont);
    m_preview.SetWindowText(LS(IDS_MD_PREVIEW_INIT));

    CRect rcClient;
    GetClientRect(&rcClient);
    m_initCx = rcClient.Width();
    m_initCy = rcClient.Height();

    auto cap = [&](CWnd& w, CRect& rc) { w.GetWindowRect(&rc); ScreenToClient(&rc); };
    cap(m_listMd,          m_rcList0);
    // m_rcPreview0은 위에서 이미 캡처함
    cap(m_lblFormat,       m_rcFmt0);
    cap(m_radHtml,         m_rcRadHtml0);
    cap(m_radPdf,          m_rcRadPdf0);
    cap(m_radHtmlPdf,      m_rcRadHtmlPdf0);
    cap(m_lblOutputPath,   m_rcOutputLbl0);
    cap(m_editOutput,      m_rcOutput0);
    cap(m_btnOutputBrowse, m_rcBrowse0);
    cap(m_lblStatus,       m_rcStatus0);

    AdjustListColumns();

    m_toolTip.Create(this, TTS_ALWAYSTIP);
    m_toolTip.SetMaxTipWidth(260);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 400);
    m_toolTip.AddTool(&m_listMd,          LS(IDS_MD_TIP_LIST));
    m_toolTip.AddTool(&m_preview,         LS(IDS_MD_TIP_PREVIEW));
    m_toolTip.AddTool(&m_radHtml,         LS(IDS_MD_TIP_HTML));
    m_toolTip.AddTool(&m_radPdf,          LS(IDS_MD_TIP_PDF));
    m_toolTip.AddTool(&m_radHtmlPdf,      LS(IDS_MD_TIP_HTMLPDF));
    m_toolTip.AddTool(&m_editOutput,      LS(IDS_MD_TIP_OUTPUT));
    m_toolTip.AddTool(&m_btnOutputBrowse, LS(IDS_MD_TIP_BROWSE));
    m_toolTip.Activate(TRUE);

    return TRUE;
}

BOOL CMdConvertDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd())
        m_toolTip.RelayEvent(pMsg);
    return CTabDlgBase::PreTranslateMessage(pMsg);
}

// ── CTabDlgBase interface ─────────────────────────────────────

void CMdConvertDlg::OnTabActivated(CEdit& editPath, CButton& checkMerge)
{
    { CString cue = LS(IDS_MD_CUE_PATH); editPath.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue); }
    checkMerge.ShowWindow(SW_HIDE);
    m_listMd.SetFocus();
}

void CMdConvertDlg::OnCommonBrowse(CEdit& editPath)
{
    auto filterVec = BuildFilter(IDS_MD_FILTER_LABEL, _T("*.md;*.markdown"));
    TCHAR szFile[32768] = {};
    OPENFILENAME ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = GetSafeHwnd();
    ofn.lpstrFilter  = filterVec.data();
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = _countof(szFile);
    ofn.Flags        = OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if (!GetOpenFileName(&ofn)) return;

    std::vector<CString> paths;
    TCHAR* p = szFile;
    CString dir(p);
    p += dir.GetLength() + 1;

    if (*p == _T('\0'))
    {
        paths.push_back(dir);
    }
    else
    {
        while (*p != _T('\0'))
        {
            CString name(p);
            paths.push_back(dir + _T("\\") + name);
            p += name.GetLength() + 1;
        }
    }

    if (!paths.empty())
    {
        CString firstDir = paths[0].Left(paths[0].ReverseFind(_T('\\')));
        editPath.SetWindowText(firstDir);
    }
    AddMdFiles(paths);
}

void CMdConvertDlg::OnCommonRun(bool /*bMerge*/)
{
    if (m_bRunning) return;
    if (m_entries.empty()) return;

    CString outDir;
    m_editOutput.GetWindowText(outDir);
    outDir.Trim();

    if (!outDir.IsEmpty())
    {
        DWORD attr = GetFileAttributes(outDir);
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            ::MessageBoxW(GetSafeHwnd(), LS(IDS_MD_ERR_OUTDIR), LS(IDS_MD_ERR_TITLE), MB_OK | MB_ICONWARNING);
            return;
        }
    }

    if (outDir.IsEmpty())
        outDir = m_entries[0].path.Left(m_entries[0].path.ReverseFind(_T('\\')));

    bool bPdf     = (m_radPdf.GetCheck()     == BST_CHECKED);
    bool bHtmlPdf = (m_radHtmlPdf.GetCheck() == BST_CHECKED);
    bool bNeedPdf = bPdf || bHtmlPdf;

    for (auto& e : m_entries) e.status = LS(IDS_MD_STATUS_WAIT);
    RefreshList();

    m_bRunning = true;
    m_bDone    = false;
    NotifyHostStateChanged();

    // PDF/HTML+PDF 모드: 페이지 렌더링을 UI 스레드에서 미리 수행
    std::vector<std::vector<PdfWriter::ImageData>> allPages;
    if (bNeedPdf)
    {
        SetStatus(LS(IDS_MD_STATUS_RENDERING));
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            m_entries[i].status = LS(IDS_MD_STATUS_RENDERING_ITEM);
            RefreshList();
            CString mdText = ReadMdFile(m_entries[i].path);
            if (mdText.IsEmpty())
            {
                allPages.push_back({});
            }
            else
            {
                std::string rtf = MdConverter::ToRtf(mdText);
                allPages.push_back(RenderMdToPdfPages(this, rtf));
            }
        }
        SetStatus(LS(IDS_MD_STATUS_CONVERTING));
    }
    else
    {
        SetStatus(LS(IDS_MD_STATUS_CONVERTING));
    }

    if (m_worker.joinable()) m_worker.join();

    HWND hNotify = m_hHostNotify;

    m_worker = std::thread([this, outDir, bPdf, bHtmlPdf,
                            allPages = std::move(allPages), hNotify]()
    {
        int fail = 0;
        for (int i = 0; i < (int)m_entries.size(); ++i)
        {
            auto& entry = m_entries[i];
            entry.status = LS(IDS_MD_STATUS_CONVERTING_ITEM);

            CString stem = entry.name;
            int dot = stem.ReverseFind(_T('.'));
            if (dot >= 0) stem = stem.Left(dot);

            bool ok = true;

            // HTML 출력 (HTML 모드 또는 HTML+PDF 모드)
            if (!bPdf)
            {
                CString mdText;
                {
                    FILE* f = nullptr;
                    _wfopen_s(&f, entry.path, L"rb");
                    if (!f) { entry.status = LS(IDS_MD_STATUS_FAIL); ++fail; continue; }

                    fseek(f, 0, SEEK_END);
                    long sz = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    std::vector<char> buf(sz + 1, 0);
                    fread(buf.data(), 1, sz, f);
                    fclose(f);

                    int offset = 0;
                    if (sz >= 3 && (unsigned char)buf[0] == 0xEF &&
                                   (unsigned char)buf[1] == 0xBB &&
                                   (unsigned char)buf[2] == 0xBF)
                        offset = 3;

                    int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                   buf.data() + offset, sz - offset, nullptr, 0);
                    if (wlen > 0)
                    {
                        std::vector<wchar_t> wb(wlen + 1, 0);
                        MultiByteToWideChar(CP_UTF8, 0,
                            buf.data() + offset, sz - offset, wb.data(), wlen);
                        mdText = CString(wb.data());
                    }
                }

                CString html    = MdConverter::ToHtml(mdText, stem);
                CString outPath = outDir + _T("\\") + stem + _T(".html");
                {
                    std::wstring ws(html.GetString());
                    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1,
                                                  nullptr, 0, nullptr, nullptr);
                    std::vector<char> utf8(len);
                    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1,
                                        utf8.data(), len, nullptr, nullptr);
                    FILE* f = nullptr;
                    _wfopen_s(&f, outPath, L"wb");
                    if (f)
                    {
                        unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
                        fwrite(bom, 1, 3, f);
                        fwrite(utf8.data(), 1, (size_t)len - 1, f);
                        fclose(f);
                    }
                    else
                    {
                        ok = false;
                    }
                }
            }

            // PDF 출력 (PDF 모드 또는 HTML+PDF 모드)
            if (bPdf || bHtmlPdf)
            {
                const auto& pages = allPages[i];
                if (pages.empty())
                {
                    ok = false;
                }
                else
                {
                    CString outPath = outDir + _T("\\") + stem + _T(".pdf");
                    CString errMsg;
                    if (!PdfWriter::WriteMergedFromData(pages, outPath, errMsg))
                        ok = false;
                }
            }

            entry.status = ok ? LS(IDS_MD_STATUS_DONE) : LS(IDS_MD_STATUS_FAIL);
            if (!ok) ++fail;
        }

        ::PostMessage(hNotify, WM_MD_CONVERT_DONE, fail == 0 ? 1 : 0, 0);
    });
}

void CMdConvertDlg::OnCommonMoveUp()
{
    int idx = m_listMd.GetNextItem(-1, LVNI_SELECTED);
    if (idx <= 0 || idx >= (int)m_entries.size()) return;
    std::swap(m_entries[idx], m_entries[idx - 1]);
    RefreshList();
    m_listMd.SetItemState(idx - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    NotifyHostStateChanged();
}

void CMdConvertDlg::OnCommonMoveDown()
{
    int idx = m_listMd.GetNextItem(-1, LVNI_SELECTED);
    int n   = (int)m_entries.size();
    if (idx < 0 || idx >= n - 1) return;
    std::swap(m_entries[idx], m_entries[idx + 1]);
    RefreshList();
    m_listMd.SetItemState(idx + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    NotifyHostStateChanged();
}

void CMdConvertDlg::OnCommonClear(bool bConvDone)
{
    if (bConvDone)
    {
        if (::MessageBoxW(GetSafeHwnd(), LS(IDS_MD_CONFIRM_RESET), LS(IDS_MD_CONFIRM_TITLE),
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        m_entries.clear();
        m_bDone = false;
        m_editOutput.SetWindowText(_T(""));
        RefreshList();
        SetStatus(_T(""));
    }
    else
    {
        int idx = m_listMd.GetNextItem(-1, LVNI_SELECTED);
        if (idx < 0 || idx >= (int)m_entries.size()) return;
        m_entries.erase(m_entries.begin() + idx);
        RefreshList();
    }
    NotifyHostStateChanged();
}

bool CMdConvertDlg::IsRunning()   { return m_bRunning; }
bool CMdConvertDlg::IsDone()      { return m_bDone; }

bool CMdConvertDlg::CanMoveUp()
{
    int idx = m_listMd.GetNextItem(-1, LVNI_SELECTED);
    return !m_bRunning && idx > 0;
}

bool CMdConvertDlg::CanMoveDown()
{
    int idx = m_listMd.GetNextItem(-1, LVNI_SELECTED);
    return !m_bRunning && idx >= 0 && idx < (int)m_entries.size() - 1;
}

bool CMdConvertDlg::CanDelete()
{
    if (m_bRunning) return false;
    if (m_bDone) return true;
    return m_listMd.GetNextItem(-1, LVNI_SELECTED) >= 0;
}

bool CMdConvertDlg::CanRun()
{
    return !m_bRunning && !m_entries.empty();
}

CString CMdConvertDlg::RunLabel()
{
    return LS(IDS_MD_BTN_CONVERT);
}

CString CMdConvertDlg::ClearTooltip()
{
    if (m_bDone)
        return LS(IDS_MD_CLEAR_DONE);
    if (m_listMd.GetNextItem(-1, LVNI_SELECTED) >= 0)
        return LS(IDS_MD_CLEAR_SEL);
    return LS(IDS_MD_CLEAR_NONE);
}

// ── called by host ────────────────────────────────────────────

void CMdConvertDlg::NotifyConvertDone(bool bSuccess)
{
    if (m_worker.joinable()) m_worker.join();
    m_bRunning = false;
    m_bDone    = true;
    RefreshList();
    if (!bSuccess)
    {
        SetStatus(LS(IDS_MD_STATUS_PARTIAL_FAIL));
    }
    else
    {
        bool bPdf     = (m_radPdf.GetCheck()     == BST_CHECKED);
        bool bHtmlPdf = (m_radHtmlPdf.GetCheck() == BST_CHECKED);
        if (bHtmlPdf)     SetStatus(LS(IDS_MD_STATUS_DONE_HTML_PDF));
        else if (bPdf)    SetStatus(LS(IDS_MD_STATUS_DONE_PDF));
        else              SetStatus(LS(IDS_MD_STATUS_DONE_HTML));
    }
    NotifyHostStateChanged();
}

void CMdConvertDlg::AddMdFiles(const std::vector<CString>& paths)
{
    for (const auto& path : paths)
    {
        CString ext = PathFindExtension(path);
        ext.MakeLower();
        if (ext != _T(".md") && ext != _T(".markdown")) continue;

        bool dup = false;
        for (const auto& e : m_entries)
            if (e.path.CompareNoCase(path) == 0) { dup = true; break; }
        if (dup) continue;

        MdEntry entry;
        entry.path   = path;
        entry.name   = PathFindFileName(path);
        entry.status = LS(IDS_MD_STATUS_WAIT);
        m_entries.push_back(entry);
    }
    // 출력경로가 비어있으면 첫 항목의 부모 폴더로 자동 설정
    if (m_editOutput.GetSafeHwnd() && !m_entries.empty())
    {
        CString cur;
        m_editOutput.GetWindowText(cur);
        if (cur.IsEmpty())
        {
            CString dir = m_entries[0].path.Left(m_entries[0].path.ReverseFind(_T('\\')));
            m_editOutput.SetWindowText(dir);
        }
    }

    RefreshList();
    NotifyHostStateChanged();
}

// ── helpers ───────────────────────────────────────────────────

void CMdConvertDlg::RefreshList()
{
    bool bPdf     = m_radPdf.GetSafeHwnd()     && (m_radPdf.GetCheck()     == BST_CHECKED);
    bool bHtmlPdf = m_radHtmlPdf.GetSafeHwnd() && (m_radHtmlPdf.GetCheck() == BST_CHECKED);

    m_listMd.SetRedraw(FALSE);
    m_listMd.DeleteAllItems();
    for (int i = 0; i < (int)m_entries.size(); ++i)
    {
        const auto& e = m_entries[i];
        m_listMd.InsertItem(i, e.name);

        CString stem = e.name.Left(e.name.ReverseFind(_T('.')));
        CString outName;
        if (bHtmlPdf)     outName = stem + _T(".html / .pdf");
        else if (bPdf)    outName = stem + _T(".pdf");
        else              outName = stem + _T(".html");
        m_listMd.SetItemText(i, 1, outName);
        m_listMd.SetItemText(i, 2, e.status);
    }
    m_listMd.SetRedraw(TRUE);
    AdjustListColumns();
}

void CMdConvertDlg::AdjustListColumns()
{
    CRect rc;
    m_listMd.GetClientRect(&rc);
    int avail = rc.Width();
    if (avail <= 60) return;

    int col0w = avail * 2 / 5;
    int col1w = avail * 2 / 5;
    int col2w = avail - col0w - col1w;
    m_listMd.SetColumnWidth(0, col0w);
    m_listMd.SetColumnWidth(1, col1w);
    m_listMd.SetColumnWidth(2, col2w);
}

void CMdConvertDlg::SetStatus(const CString& msg)
{
    if (m_lblStatus.GetSafeHwnd())
        m_lblStatus.SetWindowText(msg);
}

void CMdConvertDlg::NotifyHostStateChanged()
{
    if (m_hHostNotify)
        ::PostMessage(m_hHostNotify, WM_TAB_STATE_CHANGED, 0, 0);
}

// ── drag-and-drop ─────────────────────────────────────────────

void CMdConvertDlg::OnDropFiles(HDROP hDropInfo)
{
    UINT count = DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0);
    std::vector<CString> paths;
    for (UINT i = 0; i < count; ++i)
    {
        TCHAR buf[MAX_PATH] = {};
        DragQueryFile(hDropInfo, i, buf, MAX_PATH);
        paths.emplace_back(buf);
    }
    DragFinish(hDropInfo);
    if (m_hHostNotify)
        ::SendMessage(m_hHostNotify, WM_ROUTE_DROP, 0, (LPARAM)&paths);
}

// ── format radio ──────────────────────────────────────────────

void CMdConvertDlg::OnBnClickedFormatRadio()
{
    RefreshList();
}

// ── output folder browse ──────────────────────────────────────

void CMdConvertDlg::OnBnClickedOutputBrowse()
{
    TCHAR szPath[MAX_PATH] = {};
    BROWSEINFO bi          = {};
    bi.hwndOwner           = GetSafeHwnd();
    bi.pszDisplayName      = szPath;
    CString browseTitle = LS(IDS_FOLDER_SELECT); bi.lpszTitle = browseTitle;
    bi.ulFlags             = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pIdl = SHBrowseForFolder(&bi);
    if (!pIdl) return;
    SHGetPathFromIDList(pIdl, szPath);
    CoTaskMemFree(pIdl);
    m_editOutput.SetWindowText(szPath);
}

// ── MD → PDF rendering helpers ────────────────────────────────

static bool GetJpegClsid(CLSID& clsid)
{
    UINT n = 0, sz = 0;
    Gdiplus::GetImageEncodersSize(&n, &sz);
    if (sz == 0) return false;
    std::vector<BYTE> buf(sz);
    Gdiplus::GetImageEncoders(n, sz, reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data()));
    for (UINT i = 0; i < n; ++i)
    {
        auto* c = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data()) + i;
        if (wcscmp(c->MimeType, L"image/jpeg") == 0) { clsid = c->Clsid; return true; }
    }
    return false;
}

static bool BitmapToJpegBytes(HBITMAP hBmp, std::vector<uint8_t>& out)
{
    CLSID jpegClsid;
    if (!GetJpegClsid(jpegClsid)) return false;

    Gdiplus::Bitmap bmp(hBmp, nullptr);

    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &pStream))) return false;

    Gdiplus::EncoderParameters ep = {};
    ep.Count = 1;
    ep.Parameter[0].Guid              = Gdiplus::EncoderQuality;
    ep.Parameter[0].Type              = Gdiplus::EncoderParameterValueTypeLong;
    ep.Parameter[0].NumberOfValues    = 1;
    ULONG quality = 90;
    ep.Parameter[0].Value             = &quality;

    bool ok = (bmp.Save(pStream, &jpegClsid, &ep) == Gdiplus::Ok);
    if (ok)
    {
        HGLOBAL hg = nullptr;
        GetHGlobalFromStream(pStream, &hg);
        SIZE_T sz = GlobalSize(hg);
        void*  pData = GlobalLock(hg);
        if (pData && sz > 0)
            out.assign(static_cast<uint8_t*>(pData),
                       static_cast<uint8_t*>(pData) + sz);
        GlobalUnlock(hg);
        ok = !out.empty();
    }
    pStream->Release();
    return ok;
}

// Renders RTF text to A4 PDF pages using EM_FORMATRANGE.
// Must be called on the UI thread.
static std::vector<PdfWriter::ImageData> RenderMdToPdfPages(
    CWnd* pParent, const std::string& rtf)
{
    const int A4W_TWIPS    = 11906;
    const int A4H_TWIPS    = 16838;
    const int MARGIN_TWIPS = 1008; // ~0.7 inch

    HDC hScreen = ::GetDC(nullptr);
    int dpiX    = GetDeviceCaps(hScreen, LOGPIXELSX);
    int dpiY    = GetDeviceCaps(hScreen, LOGPIXELSY);
    int pageW   = MulDiv(A4W_TWIPS, dpiX, 1440);
    int pageH   = MulDiv(A4H_TWIPS, dpiY, 1440);

    // Hidden RichEdit for off-screen rendering
    CRichEditCtrl renderCtrl;
    if (!renderCtrl.Create(WS_CHILD | ES_MULTILINE | ES_READONLY,
                           CRect(0, 0, pageW, pageH), pParent, 9901))
                           {
        ::ReleaseDC(nullptr, hScreen);
        return {};
    }

    // Load RTF
    {
        struct Ctx { const char* ptr; size_t size; size_t pos; };
        Ctx ctx = { rtf.c_str(), rtf.size(), 0 };
        EDITSTREAM es = {};
        es.dwCookie    = (DWORD_PTR)&ctx;
        es.pfnCallback = [](DWORD_PTR dw, LPBYTE pb, LONG cb, LONG* pcb) -> DWORD
        {
            auto* c = reinterpret_cast<Ctx*>(dw);
            size_t n = ((size_t)cb < c->size - c->pos) ? (size_t)cb : c->size - c->pos;
            if (n) memcpy(pb, c->ptr + c->pos, n);
            c->pos += n; *pcb = (LONG)n; return 0;
        };
        renderCtrl.StreamIn(SF_RTF, es);
    }

    int totalChars = renderCtrl.GetTextLength();

    HDC hDC = ::CreateCompatibleDC(hScreen);

    // Flush any cached EM_FORMATRANGE state
    renderCtrl.SendMessage(EM_FORMATRANGE, FALSE, 0);

    std::vector<PdfWriter::ImageData> result;
    int curChar = 0;

    while (true)
    {
        BITMAPINFO bmi            = {};
        bmi.bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth     = pageW;
        bmi.bmiHeader.biHeight    = -pageH; // top-down
        bmi.bmiHeader.biPlanes    = 1;
        bmi.bmiHeader.biBitCount  = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        void*   pBits = nullptr;
        HBITMAP hBmp  = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        if (!hBmp) break;
        HBITMAP hOld  = (HBITMAP)SelectObject(hDC, hBmp);

        RECT rfill = { 0, 0, pageW, pageH };
        HBRUSH hBr = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hDC, &rfill, hBr);
        DeleteObject(hBr);

        FORMATRANGE fr     = {};
        fr.hdc             = fr.hdcTarget  = hDC;
        fr.rcPage.left     = 0;
        fr.rcPage.top      = 0;
        fr.rcPage.right    = A4W_TWIPS;
        fr.rcPage.bottom   = A4H_TWIPS;
        fr.rc.left         = MARGIN_TWIPS;
        fr.rc.top          = MARGIN_TWIPS;
        fr.rc.right        = A4W_TWIPS - MARGIN_TWIPS;
        fr.rc.bottom       = A4H_TWIPS - MARGIN_TWIPS;
        fr.chrg.cpMin      = curChar;
        fr.chrg.cpMax      = totalChars;

        int nextChar = (int)renderCtrl.SendMessage(EM_FORMATRANGE, TRUE, (LPARAM)&fr);

        SelectObject(hDC, hOld);

        PdfWriter::ImageData img;
        img.width  = pageW;
        img.height = pageH;
        img.pageW  = 595;   // A4 PDF points
        img.pageH  = 842;
        img.isJpeg = true;

        if (BitmapToJpegBytes(hBmp, img.raw))
            result.push_back(std::move(img));

        DeleteObject(hBmp);

        if (nextChar <= curChar || nextChar >= totalChars)
            break;
        curChar = nextChar;
    }

    renderCtrl.SendMessage(EM_FORMATRANGE, FALSE, 0);
    renderCtrl.DestroyWindow();
    ::DeleteDC(hDC);
    ::ReleaseDC(nullptr, hScreen);

    return result;
}

// ── preview ───────────────────────────────────────────────────

static CString ReadMdFile(const CString& path)
{
    FILE* f = nullptr;
    _wfopen_s(&f, path, L"rb");
    if (!f) return _T("");

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<char> buf(sz + 1, 0);
    fread(buf.data(), 1, sz, f);
    fclose(f);

    int offset = 0;
    if (sz >= 3 && (unsigned char)buf[0] == 0xEF &&
                   (unsigned char)buf[1] == 0xBB &&
                   (unsigned char)buf[2] == 0xBF) offset = 3;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data() + offset, sz - offset, nullptr, 0);
    if (wlen <= 0) return _T("");
    std::vector<wchar_t> wb(wlen + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, buf.data() + offset, sz - offset, wb.data(), wlen);
    return CString(wb.data());
}

static void StreamRtf(CRichEditCtrl& ctrl, const std::string& rtf)
{
    struct Ctx { const char* ptr; size_t size; size_t pos; };
    Ctx ctx = { rtf.c_str(), rtf.size(), 0 };

    EDITSTREAM es = {};
    es.dwCookie   = (DWORD_PTR)&ctx;
    es.pfnCallback = [](DWORD_PTR dw, LPBYTE pb, LONG cb, LONG* pcb) -> DWORD
    {
        auto* c = reinterpret_cast<Ctx*>(dw);
        size_t n = ((size_t)cb < c->size - c->pos) ? (size_t)cb : c->size - c->pos;
        if (n) memcpy(pb, c->ptr + c->pos, n);
        c->pos += n;
        *pcb = (LONG)n;
        return 0;
    };
    ctrl.StreamIn(SF_RTF, es);
}

void CMdConvertDlg::OnNMClickListMd(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx >= 0 && idx < (int)m_entries.size())
    {
        CString mdText = ReadMdFile(m_entries[idx].path);
        if (!mdText.IsEmpty())
            StreamRtf(m_preview, MdConverter::ToRtf(mdText));
        else
            m_preview.SetWindowText(LS(IDS_MD_PREVIEW_ERR));
    }
    *pResult = 0;
}

// ── list notification ─────────────────────────────────────────

void CMdConvertDlg::OnNMDblclkListMd(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx >= 0 && idx < (int)m_entries.size())
        ShellExecute(nullptr, _T("open"), _T("notepad.exe"),
                     m_entries[idx].path, nullptr, SW_SHOWNORMAL);
    *pResult = 0;
}

void CMdConvertDlg::OnNMRClickListMd(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    bool hasItem = (!m_bRunning && idx >= 0 && idx < (int)m_entries.size());

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 1, LS(IDS_MD_MENU_REMOVE));
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 2, LS(IDS_MD_MENU_OPEN_LOC));
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 4, LS(IDS_MD_MENU_OPEN_NOTEPAD));
    menu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)nullptr);
    menu.AppendMenu((m_bRunning || m_entries.empty()) ? MF_STRING | MF_GRAYED : MF_STRING, 3, LS(IDS_MD_MENU_CLEAR_ALL));

    CPoint pt;
    GetCursorPos(&pt);
    int cmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, this);

    if (cmd == 1)
    {
        m_entries.erase(m_entries.begin() + idx);
        RefreshList();
        NotifyHostStateChanged();
    }
    else if (cmd == 2)
    {
        CString dir = m_entries[idx].path.Left(m_entries[idx].path.ReverseFind(_T('\\')));
        ShellExecute(nullptr, _T("explore"), dir, nullptr, nullptr, SW_SHOWNORMAL);
    }
    else if (cmd == 4)
    {
        ShellExecute(nullptr, _T("open"), _T("notepad.exe"),
                     m_entries[idx].path, nullptr, SW_SHOWNORMAL);
    }
    else if (cmd == 3)
    {
        m_entries.clear();
        m_bDone = false;
        m_editOutput.SetWindowText(_T(""));
        RefreshList();
        SetStatus(_T(""));
        NotifyHostStateChanged();
    }
    *pResult = 0;
}

void CMdConvertDlg::OnLvnItemChangedListMd(NMHDR*, LRESULT* pResult)
{
    NotifyHostStateChanged();
    *pResult = 0;
}

void CMdConvertDlg::ApplyLanguage()
{
    m_toolTip.UpdateTipText(LS(IDS_MD_TIP_LIST),    &m_listMd);
    m_toolTip.UpdateTipText(LS(IDS_MD_TIP_PREVIEW), &m_preview);
    m_toolTip.UpdateTipText(LS(IDS_MD_TIP_HTML),    &m_radHtml);
    m_toolTip.UpdateTipText(LS(IDS_MD_TIP_PDF),     &m_radPdf);
    m_toolTip.UpdateTipText(LS(IDS_MD_TIP_HTMLPDF), &m_radHtmlPdf);
    m_toolTip.UpdateTipText(LS(IDS_MD_TIP_OUTPUT),  &m_editOutput);
    m_toolTip.UpdateTipText(LS(IDS_MD_TIP_BROWSE),  &m_btnOutputBrowse);

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT;
    CString col0 = LS(IDS_MD_COL0); lvc.pszText = col0.GetBuffer(); m_listMd.SetColumn(0, &lvc); col0.ReleaseBuffer();
    CString col1 = LS(IDS_MD_COL1); lvc.pszText = col1.GetBuffer(); m_listMd.SetColumn(1, &lvc); col1.ReleaseBuffer();
    CString col2 = LS(IDS_MD_COL2); lvc.pszText = col2.GetBuffer(); m_listMd.SetColumn(2, &lvc); col2.ReleaseBuffer();
}

// ── resize ────────────────────────────────────────────────────

void CMdConvertDlg::OnSize(UINT nType, int cx, int cy)
{
    CTabDlgBase::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED || !m_listMd.GetSafeHwnd()) return;
    ResizeControls(cx, cy);
}

void CMdConvertDlg::ResizeControls(int cx, int cy)
{
    if (m_initCx == 0) return;
    int dw = cx - m_initCx;
    int dh = cy - m_initCy;

    auto move = [](CWnd& w, CRect r) { w.MoveWindow(r.left, r.top, r.Width(), r.Height()); };

    CRect r;
    // List: width-only expands (height fixed so preview gets the extra space)
    r = m_rcList0;    r.right  += dw;                       move(m_listMd,          r); AdjustListColumns();
    // Preview: expands both width and height
    r = m_rcPreview0; r.right  += dw; r.bottom += dh;       move(m_preview,         r);
    // Bottom controls shift down by dh
    r = m_rcFmt0;     r.OffsetRect(0, dh);                  move(m_lblFormat,       r);
    r = m_rcRadHtml0; r.OffsetRect(0, dh);                  move(m_radHtml,         r);
    r = m_rcRadPdf0;  r.OffsetRect(0, dh);                  move(m_radPdf,          r);
    r = m_rcRadHtmlPdf0; r.OffsetRect(0, dh);              move(m_radHtmlPdf,      r);
    r = m_rcOutputLbl0; r.OffsetRect(0, dh);                 move(m_lblOutputPath,   r);
    r = m_rcOutput0;  r.right  += dw; r.OffsetRect(0, dh);  move(m_editOutput,      r);
    r = m_rcBrowse0;  r.OffsetRect(dw, dh);                 move(m_btnOutputBrowse, r);
    r = m_rcStatus0;  r.right  += dw; r.OffsetRect(0, dh);  move(m_lblStatus,       r);

    Invalidate();
}

// ── destroy ───────────────────────────────────────────────────

void CMdConvertDlg::OnDestroy()
{
    m_bRunning = false;
    if (m_worker.joinable()) m_worker.join();
    CTabDlgBase::OnDestroy();
}
