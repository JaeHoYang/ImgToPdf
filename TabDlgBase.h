#pragma once
#include "pch.h"
#include "AppLang.h"

/**
 * @brief 모든 탭 자식 다이얼로그의 공통 인터페이스 기반 클래스.
 *
 * 호스트 다이얼로그(CImgToPdfDlg)가 탭을 다형적으로 제어하기 위해
 * 각 탭 클래스는 이 클래스를 상속하고 순수 가상 함수를 구현해야 한다.
 *
 * m_hHostNotify 에 호스트 HWND를 설정하면, 탭이 WM_TAB_STATE_CHANGED 등
 * 커스텀 메시지를 호스트에 PostMessage 로 전달할 수 있다.
 */
class CTabDlgBase : public CDialogEx
{
public:
    /** @brief 상태 변경 알림을 수신할 호스트 다이얼로그 HWND. */
    HWND m_hHostNotify = nullptr;

    // ── 호스트가 호출하는 공통 UI 액션 ──────────────────────────

    /**
     * @brief 호스트의 [찾아보기] 버튼 클릭 시 탭이 처리한다.
     * @param editPath 호스트의 경로 에디트박스 참조 (탭이 경로를 채울 수 있다)
     */
    virtual void OnCommonBrowse(CEdit& editPath) = 0;

    /**
     * @brief 호스트의 [변환/실행] 버튼 클릭 시 탭이 처리한다.
     * @param bMerge 합치기 체크박스 상태
     */
    virtual void OnCommonRun(bool bMerge) = 0;

    /** @brief 호스트의 [▲] 버튼 클릭 시 탭이 처리한다. */
    virtual void OnCommonMoveUp() {}

    /** @brief 호스트의 [▼] 버튼 클릭 시 탭이 처리한다. */
    virtual void OnCommonMoveDown() {}

    /**
     * @brief 호스트의 [삭제] 버튼 클릭 시 탭이 처리한다.
     * @param bConvDone true 이면 변환 완료 후 전체 초기화, false 이면 선택 항목 제거
     */
    virtual void OnCommonClear(bool bConvDone) {}

    /**
     * @brief 호스트의 합치기 체크박스 상태 변경 시 탭에 알린다.
     * @param bMerge 변경된 체크박스 상태
     */
    virtual void OnCommonMergeChanged(bool bMerge) {}

    /**
     * @brief 탭이 활성화(포커스 이동)될 때 호스트가 호출한다.
     *
     * 탭은 이 시점에 공통 editPath 의 플레이스홀더를 설정하고,
     * checkMerge 표시 여부를 결정한다.
     *
     * @param editPath  호스트의 공통 경로 에디트박스
     * @param checkMerge 호스트의 합치기 체크박스
     */
    virtual void OnTabActivated(CEdit& editPath, CButton& checkMerge) {}

    // ── 호스트가 읽는 탭 상태 쿼리 ──────────────────────────────

    /** @brief 현재 변환/작업이 실행 중이면 true. */
    virtual bool    IsRunning()    { return false; }

    /** @brief 변환/작업이 완료된 상태이면 true. */
    virtual bool    IsDone()       { return false; }

    /** @brief 현재 선택 항목이 위로 이동 가능하면 true. */
    virtual bool    CanMoveUp()    { return false; }

    /** @brief 현재 선택 항목이 아래로 이동 가능하면 true. */
    virtual bool    CanMoveDown()  { return false; }

    /** @brief 목록에서 항목을 삭제 가능한 상태이면 true. */
    virtual bool    CanDelete()    { return false; }

    /** @brief 현재 상태에서 변환/실행 가능하면 true. */
    virtual bool    CanRun()       { return false; }

    /** @brief 합치기 기능을 사용할 수 있는 상태이면 true. */
    virtual bool    CanMerge()     { return false; }

    /**
     * @brief 호스트에서 합치기 체크박스를 표시할지 여부.
     * @return 탭이 합치기를 지원하면 true (탭1만 true)
     */
    virtual bool    ShowMerge()    { return false; }

    /**
     * @brief 호스트 [변환/실행] 버튼에 표시할 레이블 문자열.
     * @return 현재 언어에 맞는 버튼 레이블
     */
    virtual CString RunLabel()     { return LS(IDS_IMG_BTN_CONVERT); }

    /**
     * @brief 호스트 [삭제] 버튼의 툴팁 문자열.
     * @return 현재 상태에 맞는 툴팁 문자열
     */
    virtual CString ClearTooltip() { return LS(IDS_TIP_CLEAR_BASE); }

    /**
     * @brief 언어 전환 시 호스트가 호출한다.
     *
     * 탭은 자체 컨트롤(컬럼 헤더, 버튼 텍스트, 툴팁 등)을 현재 언어로 갱신해야 한다.
     */
    virtual void    ApplyLanguage() {}

protected:
    /**
     * @brief 다이얼로그 리소스 ID로 초기화하는 생성자.
     * @param nIDTemplate 탭 다이얼로그 리소스 ID (예: IDD_TAB1)
     * @param pParent     부모 윈도우 포인터 (nullptr 가능)
     */
    explicit CTabDlgBase(UINT nIDTemplate, CWnd* pParent = nullptr)
        : CDialogEx(nIDTemplate, pParent) {}
};
