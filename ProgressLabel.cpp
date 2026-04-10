#include "pch.h"
#include "ProgressLabel.h"

BEGIN_MESSAGE_MAP(CProgressLabel, CStatic)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CProgressLabel::SetIdle()
{
    m_idle = true;
    if (GetSafeHwnd()) Invalidate();
}

void CProgressLabel::SetCounts(int success, int fail, int total)
{
    m_success = success;
    m_fail    = fail;
    m_total   = total;
    m_idle    = false;
    if (GetSafeHwnd()) Invalidate();
}

BOOL CProgressLabel::OnEraseBkgnd(CDC*) { return TRUE; }

void CProgressLabel::OnPaint()
{
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);

    dc.FillSolidRect(&rc, ::GetSysColor(COLOR_BTNFACE));
    dc.SetBkMode(TRANSPARENT);

    CFont* pOldFont = dc.SelectObject(GetFont());

    if (m_idle)
    {
        dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
        dc.DrawText(_T("대기 중"), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    else
    {
        CString sSucc, sFail, sTotal;
        sSucc.Format(_T("%d"), m_success);
        sFail.Format(_T("%d"), m_fail);
        sTotal.Format(_T("%d"), m_total);

        COLORREF clrDefault = ::GetSysColor(COLOR_WINDOWTEXT);
        COLORREF clrGreen   = RGB(0, 170, 60);
        COLORREF clrRed     = RGB(210, 40, 40);

        struct Seg { CString text; COLORREF color; };
        Seg segs[] = {
            { _T("("),  clrDefault },
            { sSucc,    clrGreen   },
            { _T("/"),  clrDefault },
            { sFail,    clrRed     },
            { _T("/"),  clrDefault },
            { sTotal,   clrDefault },
            { _T(")"),  clrDefault },
        };

        int x = rc.left;
        for (auto& s : segs)
        {
            CSize sz = dc.GetTextExtent(s.text);
            CRect segRc(x, rc.top, x + sz.cx, rc.bottom);
            dc.SetTextColor(s.color);
            dc.DrawText(s.text, &segRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            x += sz.cx;
        }
    }

    if (pOldFont) dc.SelectObject(pOldFont);
}
