#pragma once
#include "pch.h"
#include "resource.h"
#include "TabDlgBase.h"
#include "ColorButton.h"
#include <thread>

/**
 * @brief 탭3 — Markdown 파일을 HTML / PDF / 양쪽으로 변환하는 다이얼로그.
 *
 * 주요 기능:
 * - 드래그앤드롭 및 찾아보기로 .md 파일 추가
 * - 출력 형식 선택: HTML 단독 / PDF 단독 / HTML+PDF 동시
 * - 변환은 단일 백그라운드 스레드(m_worker)에서 순차 실행
 * - CRichEditCtrl 미리보기: MdConverter::ToRtf() 결과를 StreamIn
 * - 출력 폴더 지정 (비워두면 원본 파일 위치)
 * - WM_MD_CONVERT_DONE 수신 시 NotifyConvertDone() 호출
 */
class CMdConvertDlg : public CTabDlgBase
{
public:
    /**
     * @brief 생성자.
     * @param pParent 부모 윈도우 포인터 (nullptr 가능)
     */
    explicit CMdConvertDlg(CWnd* pParent = nullptr);

    /** @brief 탭3 다이얼로그 리소스 ID. */
    enum { IDD = IDD_TAB3 };

    /**
     * @brief 호스트가 WM_MD_CONVERT_DONE 을 수신했을 때 호출한다.
     *
     * 변환 스레드 join, 완료 상태 전환, 진행 레이블 갱신을 수행하고
     * 호스트에 WM_TAB_STATE_CHANGED 를 알린다.
     *
     * @param bSuccess true 이면 변환 성공, false 이면 실패
     */
    void NotifyConvertDone(bool bSuccess);

    /**
     * @brief Markdown 파일 경로 목록을 목록에 추가한다.
     *
     * .md 확장자만 허용하고 중복 경로는 건너뛴다.
     *
     * @param paths 추가할 파일 경로 목록
     */
    void AddMdFiles(const std::vector<CString>& paths);

    // ── CTabDlgBase 인터페이스 구현 ──────────────────────────

    /**
     * @brief 호스트 [찾아보기] 클릭 시 파일 선택 대화상자를 열고 결과를 추가한다.
     * @param editPath 호스트의 경로 에디트박스
     */
    virtual void OnCommonBrowse(CEdit& editPath) override;

    /**
     * @brief 호스트 [변환] 클릭 시 변환 스레드를 시작한다.
     *
     * 출력 폴더 유효성 검사 후 m_worker 스레드를 생성한다.
     *
     * @param bMerge 탭3에서는 사용되지 않음 (항상 false)
     */
    virtual void OnCommonRun(bool bMerge) override;

    /** @brief 선택 항목을 위로 이동한다 (m_entries 및 리스트 갱신). */
    virtual void OnCommonMoveUp() override;

    /** @brief 선택 항목을 아래로 이동한다 (m_entries 및 리스트 갱신). */
    virtual void OnCommonMoveDown() override;

    /**
     * @brief 목록 삭제/초기화를 수행한다.
     * @param bConvDone true 이면 변환 완료 후 전체 초기화, false 이면 선택 항목 제거
     */
    virtual void OnCommonClear(bool bConvDone) override;

    /**
     * @brief 탭이 활성화될 때 공통 editPath 플레이스홀더와 합치기 체크박스를 설정한다.
     *
     * 탭3은 합치기 미지원이므로 checkMerge 는 숨긴다.
     *
     * @param editPath   호스트의 공통 경로 에디트박스
     * @param checkMerge 호스트의 합치기 체크박스
     */
    virtual void OnTabActivated(CEdit& editPath, CButton& checkMerge) override;

    virtual bool    IsRunning()   override; ///< 변환 스레드 실행 중이면 true
    virtual bool    IsDone()      override; ///< 변환 완료 상태이면 true
    virtual bool    CanMoveUp()   override; ///< 선택 항목 위 이동 가능 여부
    virtual bool    CanMoveDown() override; ///< 선택 항목 아래 이동 가능 여부
    virtual bool    CanDelete()   override; ///< 삭제 가능 여부
    virtual bool    CanRun()      override; ///< 변환 실행 가능 여부

    /** @return 항상 false — 탭3은 합치기 체크박스를 숨긴다. */
    virtual bool    ShowMerge()    override { return false; }

    /** @return 변환 중이면 "중단", 아니면 "변환" 레이블 */
    virtual CString RunLabel()     override;

    /** @return 현재 상태에 맞는 삭제 버튼 툴팁 문자열 */
    virtual CString ClearTooltip() override;

    /** @brief 언어 전환 시 컨트롤 텍스트와 툴팁을 갱신한다. */
    virtual void    ApplyLanguage() override;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override {}
    virtual void OnCancel() override {}

    /**
     * @brief 변환 대기열의 Markdown 항목 하나를 나타내는 구조체.
     */
    struct MdEntry
    {
        CString path;   ///< 파일 전체 경로
        CString name;   ///< 파일명 (표시용)
        CString status; ///< 변환 상태 문자열 (대기/진행/완료/실패)
    };
    std::vector<MdEntry> m_entries; ///< 변환 대기 항목 목록

    CListCtrl     m_listMd;          ///< Markdown 파일 목록 컨트롤
    CRichEditCtrl m_preview;         ///< RTF 미리보기 (MdConverter::ToRtf 출력)
    CStatic       m_lblFormat;       ///< "출력 형식" 레이블
    CButton       m_radHtml;         ///< HTML 단독 출력 라디오
    CButton       m_radPdf;          ///< PDF 단독 출력 라디오
    CButton       m_radHtmlPdf;      ///< HTML+PDF 동시 출력 라디오
    CStatic       m_lblOutputPath;   ///< "출력 폴더" 레이블
    CEdit         m_editOutput;      ///< 출력 폴더 경로 에디트박스
    CColorButton  m_btnOutputBrowse; ///< 출력 폴더 탐색 버튼
    CStatic       m_lblStatus;       ///< 전체 변환 상태 레이블

    bool         m_bRunning = false; ///< 변환 스레드 실행 중 플래그
    bool         m_bDone    = false; ///< 변환 완료 플래그
    std::thread  m_worker;           ///< 변환 백그라운드 스레드
    CToolTipCtrl m_toolTip;          ///< 컨트롤 툴팁

    int   m_initCx = 0, m_initCy = 0; ///< 초기 클라이언트 영역 크기
    CRect m_rcList0, m_rcPreview0, m_rcFmt0, m_rcRadHtml0, m_rcRadPdf0, m_rcRadHtmlPdf0;
    CRect m_rcOutputLbl0, m_rcOutput0, m_rcBrowse0, m_rcStatus0; ///< 초기 컨트롤 rect

    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    /** @brief 드롭된 파일/폴더를 목록에 추가한다. */
    afx_msg void OnDropFiles(HDROP hDropInfo);

    /** @brief 출력 형식 라디오 변경 시 호출된다. */
    afx_msg void OnBnClickedFormatRadio();

    /** @brief 출력 폴더 탐색 버튼 클릭 시 폴더 선택 대화상자를 열고 경로를 설정한다. */
    afx_msg void OnBnClickedOutputBrowse();

    /** @brief 목록 항목 클릭 시 선택 항목의 MD 파일을 미리보기에 로드한다. */
    afx_msg void OnNMClickListMd(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 우클릭 시 컨텍스트 메뉴를 표시한다 (삭제 등). */
    afx_msg void OnNMRClickListMd(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 더블클릭 시 파일을 외부 앱으로 연다. */
    afx_msg void OnNMDblclkListMd(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 선택 변경 시 버튼 상태를 업데이트한다. */
    afx_msg void OnLvnItemChangedListMd(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 창 크기 변경 시 컨트롤 위치를 재조정한다. */
    afx_msg void OnSize(UINT nType, int cx, int cy);

    /** @brief 소멸 시 변환 스레드를 join 하고 리소스를 해제한다. */
    afx_msg void OnDestroy();

    /**
     * @brief m_entries 전체를 리스트 컨트롤에 다시 그린다.
     *
     * 항목 추가·삭제·이동 후 호출하여 뷰를 동기화한다.
     */
    void RefreshList();

    /**
     * @brief 리스트 컨트롤의 컬럼 너비를 클라이언트 폭에 맞게 조정한다.
     *
     * 가로 스크롤이 생기지 않도록 SM_CXVSCROLL 을 미리 차감한다.
     */
    void AdjustListColumns();

    /**
     * @brief 초기 rect 와 delta 를 사용하여 컨트롤 위치를 재조정한다.
     * @param cx 현재 클라이언트 너비
     * @param cy 현재 클라이언트 높이
     */
    void ResizeControls(int cx, int cy);

    /**
     * @brief 하단 상태 레이블(m_lblStatus) 텍스트를 설정한다.
     * @param msg 표시할 문자열
     */
    void SetStatus(const CString& msg);

    /** @brief 버튼 상태 재계산을 호스트에 요청하는 WM_TAB_STATE_CHANGED 를 PostMessage 한다. */
    void NotifyHostStateChanged();

    DECLARE_MESSAGE_MAP()
};
