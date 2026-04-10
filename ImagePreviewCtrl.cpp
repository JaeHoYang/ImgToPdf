#include "pch.h"
#include "ImagePreviewCtrl.h"

BEGIN_MESSAGE_MAP(CImagePreviewCtrl, CStatic)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CImagePreviewCtrl::LoadImage(const CString& path)
{
    m_bitmap.reset();
    m_bLoadFailed = false;

    // CString → wstring 변환 후 GDI+ Bitmap 로드
    m_bitmap.reset(Gdiplus::Bitmap::FromFile(path));
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

    // 배경
    dc.FillSolidRect(&rc, RGB(30, 30, 35));

    if (m_bLoadFailed || !m_bitmap)
    {
        if (m_bLoadFailed)
        {
            dc.SetTextColor(RGB(160, 80, 80));
            dc.SetBkMode(TRANSPARENT);
            dc.DrawText(_T("이미지를 불러올 수 없습니다"),
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

    // 비율 유지 Fit 계산
    float scaleX = (float)rc.Width()  / imgW;
    float scaleY = (float)rc.Height() / imgH;
    float scale  = min(scaleX, scaleY);

    int dw = (int)(imgW * scale);
    int dh = (int)(imgH * scale);
    int dx = rc.left + (rc.Width()  - dw) / 2;
    int dy = rc.top  + (rc.Height() - dh) / 2;

    g.DrawImage(m_bitmap.get(), dx, dy, dw, dh);
}
