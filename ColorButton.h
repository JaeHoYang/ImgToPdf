#pragma once
#include "pch.h"

/**
 * @brief 유니코드 텍스트(한글 포함)를 올바르게 렌더링하는 커스텀 버튼 (CButton 서브클래스).
 *
 * BS_OWNERDRAW + DrawItem 직접 구현으로 렌더링한다.
 * CMFCButton::SetFaceColor 는 내부 텍스트 렌더 경로가 달라 한글이 깨지므로 사용하지 않는다.
 *
 * 기능:
 * - 일반/호버/눌림/비활성 상태별 배경색 및 테두리 색 변경
 * - WM_MOUSEMOVE + TrackMouseEvent 로 호버 감지
 * - DrawText 로 유니코드 텍스트 렌더링 (GetWindowText 에서 읽음)
 */
class CColorButton : public CButton
{
public:
    /**
     * @brief 버튼 색상을 설정한다.
     *
     * 호버/눌림 시에는 crHover 배경색이 사용된다.
     * 비활성 시에는 내부적으로 회색 계열로 고정된다.
     *
     * @param crFace  일반 상태 배경색
     * @param crHover 호버/눌림 상태 배경색
     * @param crText  텍스트 색상
     */
    void SetColors(COLORREF crFace, COLORREF crHover, COLORREF crText)
    {
        m_crFace = crFace;
        m_crHover = crHover;
        m_crText = crText;
    }

protected:
    /**
     * @brief 서브클래싱 완료 시 BS_OWNERDRAW 스타일을 추가한다.
     *
     * DDX_Control 바인딩 후 자동으로 호출된다.
     */
    void PreSubclassWindow() override
    {
        CButton::PreSubclassWindow();
        ModifyStyle(0, BS_OWNERDRAW);
    }

    /**
     * @brief 버튼 영역을 직접 그린다.
     *
     * 상태(일반/호버/눌림/비활성)에 따라 배경색과 테두리를 선택하고,
     * DrawText 로 버튼 텍스트를 중앙 정렬하여 렌더링한다.
     * 포커스 상태이면 점선 포커스 사각형을 추가로 그린다.
     *
     * @param lpDIS Windows 가 전달하는 DRAWITEMSTRUCT 포인터
     */
    void DrawItem(LPDRAWITEMSTRUCT lpDIS) override
    {
        CDC dc;
        dc.Attach(lpDIS->hDC);
        CRect rc(lpDIS->rcItem);

        bool bPressed  = (lpDIS->itemState & ODS_SELECTED) != 0;
        bool bDisabled = (lpDIS->itemState & ODS_DISABLED) != 0;

        COLORREF crFace   = bDisabled    ? RGB(210, 215, 225)
                          : (m_bHover || bPressed) ? m_crHover
                          : m_crFace;
        COLORREF crText   = bDisabled    ? RGB(155, 160, 170) : m_crText;
        COLORREF crBorder = bDisabled    ? RGB(190, 195, 205)
                          : (m_bHover || bPressed) ? RGB(29, 78, 216)
                          : RGB(180, 185, 200);

        dc.FillSolidRect(rc, crFace);
        dc.Draw3dRect(rc, crBorder, crBorder);

        if (bPressed) rc.OffsetRect(1, 1);

        // GetWindowText -> DrawText: Unicode 빌드에서 한글 포함 모든 문자 정상 출력
        CString text;
        GetWindowText(text);
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(crText);
        CFont* pOld = dc.SelectObject(GetFont());
        dc.DrawText(text, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (pOld) dc.SelectObject(pOld);

        if ((lpDIS->itemState & ODS_FOCUS) && !bDisabled)
        {
            CRect rf = rc;
            rf.DeflateRect(3, 3);
            dc.DrawFocusRect(rf);
        }

        dc.Detach();
    }

    /**
     * @brief WM_MOUSEMOVE / WM_MOUSELEAVE 를 처리하여 호버 상태를 추적한다.
     *
     * 메시지 맵 없이 WindowProc 오버라이드로 구현한다.
     * WM_MOUSEMOVE 수신 시 TrackMouseEvent 로 WM_MOUSELEAVE 를 예약한다.
     *
     * @param msg    Windows 메시지 ID
     * @param wParam 메시지 파라미터 (wParam)
     * @param lParam 메시지 파라미터 (lParam)
     * @return 기본 WindowProc 처리 결과
     */
    LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override
    {
        if (msg == WM_MOUSEMOVE && !m_bHover)
        {
            m_bHover = true;
            Invalidate();
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, GetSafeHwnd() };
            TrackMouseEvent(&tme);
        }
        else if (msg == WM_MOUSELEAVE)
        {
            m_bHover = false;
            Invalidate();
        }
        return CButton::WindowProc(msg, wParam, lParam);
    }

    COLORREF m_crFace  = RGB(240, 240, 240); ///< 일반 상태 배경색
    COLORREF m_crHover = RGB(220, 220, 220); ///< 호버/눌림 상태 배경색
    COLORREF m_crText  = RGB(30, 30, 30);    ///< 텍스트 색상
    bool     m_bHover  = false;              ///< 현재 호버 상태 플래그
};
