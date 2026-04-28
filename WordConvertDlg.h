#pragma once
#include "pch.h"
#include "resource.h"
#include "TabDlgBase.h"
#include "ImagePreviewCtrl.h"
#include "ColorButton.h"
#include <thread>

/**
 * @brief 탭4 — Word 파일(.docx/.doc)을 PDF 로 변환하는 다이얼로그.
 *
 * ### 변환 엔진 감지 순서
 * 1. **Microsoft Word** — `FindWordExe()`: HKCU/HKLM 레지스트리에서 WINWORD.EXE 경로 탐색.
 *    COM IDispatch 를 통해 `Word.Application` → `Documents.Open` →
 *    `ExportAsFixedFormat(wdExportFormatPDF=17)` → `Close` 순으로 처리.
 * 2. **LibreOffice** — `FindLibreOfficeExe()`: 고정 경로 + 레지스트리 탐색.
 *    `CreateProcess("soffice.exe --headless --convert-to pdf --outdir ...")` 로 실행.
 *
 * ### 스레드 모델
 * 변환은 단일 백그라운드 스레드(m_worker)에서 순차 실행한다.
 * 각 항목 완료 시 WM_WORD_ITEM_DONE(WM_USER+23), 전체 완료 시
 * WM_WORD_CONVERT_DONE(WM_USER+24) 을 PostMessage 로 UI 스레드에 전달한다.
 *
 * ### 미리보기
 * CImagePreviewCtrl 로 선택 항목의 쉘 썸네일 또는 이미지를 표시한다.
 * Word/PPT 파일은 썸네일이 없으면 파일 아이콘으로 대체한다.
 */
class CWordConvertDlg : public CTabDlgBase
{
public:
    /**
     * @brief 생성자.
     * @param pParent 부모 윈도우 포인터 (nullptr 가능)
     */
    explicit CWordConvertDlg(CWnd* pParent = nullptr);

    /** @brief 탭4 다이얼로그 리소스 ID. */
    enum { IDD = IDD_TAB4 };

    /**
     * @brief 호스트가 WM_WORD_ITEM_DONE 을 수신했을 때 호출한다.
     *
     * 해당 항목의 상태 아이콘과 상태 문자열을 갱신하고 버튼 상태를 업데이트한다.
     *
     * @param idx 완료된 항목 인덱스 (0-based)
     * @param ok  true 이면 성공, false 이면 실패
     */
    void NotifyItemDone(int idx, bool ok);

    /**
     * @brief 호스트가 WM_WORD_CONVERT_DONE 을 수신했을 때 호출한다.
     *
     * 변환 스레드를 join 하고 완료 상태로 전환한 뒤 호스트에 WM_TAB_STATE_CHANGED 를 알린다.
     *
     * @param ok true 이면 전체 성공, false 이면 일부/전체 실패
     */
    void NotifyConvertDone(bool ok);

    /**
     * @brief Word 파일 경로 목록을 목록에 추가한다.
     *
     * .docx / .doc 확장자만 허용하고 중복 경로는 건너뛴다.
     * 각 항목에 대해 FindWordExe / FindLibreOfficeExe 결과를 engine 필드에 기록한다.
     *
     * @param paths 추가할 파일 경로 목록
     */
    void AddWordFiles(const std::vector<CString>& paths);

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
     * 진행 중이면 중단 처리한다 (현재 미구현 — Word/LibreOffice 프로세스 강제 종료 불가).
     *
     * @param bMerge 탭4에서는 사용되지 않음
     */
    virtual void OnCommonRun(bool bMerge) override;

    /**
     * @brief 목록 삭제/초기화를 수행한다.
     * @param bConvDone true 이면 전체 초기화, false 이면 선택 항목 제거
     */
    virtual void OnCommonClear(bool bConvDone) override;

    /**
     * @brief 탭이 활성화될 때 공통 editPath 플레이스홀더와 합치기 체크박스를 설정한다.
     *
     * 탭4는 합치기 미지원이므로 checkMerge 는 숨긴다.
     *
     * @param editPath   호스트의 공통 경로 에디트박스
     * @param checkMerge 호스트의 합치기 체크박스
     */
    virtual void OnTabActivated(CEdit& editPath, CButton& checkMerge) override;

    virtual bool    IsRunning()   override { return m_bRunning; } ///< 변환 스레드 실행 중이면 true
    virtual bool    IsDone()      override { return m_bDone; }    ///< 변환 완료 상태이면 true
    virtual bool    CanRun()      override; ///< 항목이 있고 미실행 중이면 true
    virtual bool    CanDelete()   override; ///< 선택 항목이 있고 미실행 중이면 true

    /** @return 항상 false — 탭4는 합치기 체크박스를 숨긴다. */
    virtual bool    ShowMerge()   override { return false; }

    /** @return 변환 중이면 "중단", 아니면 "변환" 레이블 */
    virtual CString RunLabel()    override;

    /** @brief 언어 전환 시 컨트롤 텍스트와 툴팁을 갱신한다. */
    virtual void    ApplyLanguage() override;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK()     override {}
    virtual void OnCancel() override {}

    /**
     * @brief 변환 대기열의 Word 항목 하나를 나타내는 구조체.
     */
    struct WordEntry
    {
        CString path;          ///< 파일 전체 경로
        CString name;          ///< 파일명 (표시용)
        CString engine;        ///< 사용할 변환 엔진 이름 ("Word" / "LibreOffice" / "없음")
        CString status;        ///< 변환 상태 문자열 (대기/변환 중/완료/실패)
        bool    done   = false; ///< 변환 완료 여부
        bool    failed = false; ///< 변환 실패 여부
    };
    std::vector<WordEntry> m_entries; ///< 변환 대기 항목 목록

    CListCtrl         m_listWord;        ///< Word 파일 목록 컨트롤
    CImagePreviewCtrl m_preview;         ///< 파일 썸네일/아이콘 미리보기
    CStatic           m_lblOutputPath;   ///< "출력 폴더" 레이블
    CEdit             m_editOutput;      ///< 출력 폴더 경로 에디트박스
    CColorButton      m_btnOutputBrowse; ///< 출력 폴더 탐색 버튼
    CStatic           m_lblStatus;       ///< 전체 변환 상태 레이블

    bool         m_bRunning = false; ///< 변환 스레드 실행 중 플래그
    bool         m_bDone    = false; ///< 변환 완료 플래그
    std::thread  m_worker;           ///< 변환 백그라운드 스레드
    CToolTipCtrl m_toolTip;          ///< 컨트롤 툴팁

    int   m_initCx = 0, m_initCy = 0; ///< 초기 클라이언트 영역 크기
    CRect m_rcList0, m_rcPreview0, m_rcOutputLbl0, m_rcOutput0, m_rcBrowse0, m_rcStatus0; ///< 초기 컨트롤 rect

    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    /** @brief 드롭된 파일/폴더를 목록에 추가한다. */
    afx_msg void OnDropFiles(HDROP hDropInfo);

    /** @brief 창 크기 변경 시 컨트롤 위치를 재조정한다. */
    afx_msg void OnSize(UINT nType, int cx, int cy);

    /** @brief 소멸 시 변환 스레드를 join 하고 리소스를 해제한다. */
    afx_msg void OnDestroy();

    /** @brief 출력 폴더 탐색 버튼 클릭 시 폴더 선택 대화상자를 열고 경로를 설정한다. */
    afx_msg void OnBnClickedOutputBrowse();

    /** @brief 목록 항목 클릭 시 선택 항목의 파일을 미리보기에 로드한다. */
    afx_msg void OnNMClickListWord(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 우클릭 시 컨텍스트 메뉴를 표시한다 (삭제 등). */
    afx_msg void OnNMRClickListWord(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 더블클릭 시 파일을 외부 앱으로 연다. */
    afx_msg void OnNMDblclkListWord(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 선택 변경 시 버튼 상태와 미리보기를 갱신한다. */
    afx_msg void OnLvnItemChangedListWord(NMHDR* pNMHDR, LRESULT* pResult);

    /**
     * @brief WM_WORD_ITEM_DONE 수신 핸들러.
     *
     * wParam=항목 인덱스, lParam=성공 여부(1/0).
     * NotifyItemDone(wParam, lParam != 0) 을 호출한다.
     */
    afx_msg LRESULT OnWordItemDone(WPARAM wParam, LPARAM lParam);

    /**
     * @brief m_entries 전체를 리스트 컨트롤에 다시 그린다.
     *
     * 항목 추가·삭제·상태 변경 후 호출하여 뷰를 동기화한다.
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

    /**
     * @brief 특정 항목의 리스트 행을 m_entries[idx] 내용으로 갱신한다.
     *
     * 전체 RefreshList() 를 호출하지 않고 변경된 행만 업데이트할 때 사용한다.
     *
     * @param idx 갱신할 항목 인덱스 (0-based)
     */
    void UpdateEntryRow(int idx);

    DECLARE_MESSAGE_MAP()
};
