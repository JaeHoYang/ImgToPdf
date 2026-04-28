#pragma once
#include "pch.h"
#include "resource.h"
#include "TabDlgBase.h"
#include "ColorButton.h"

/**
 * @brief PDF 페이지 범위 문자열을 0-based 페이지 인덱스 목록으로 변환한다.
 *
 * 입력 형식 예시: `"1,3,5-7,10"` → `{0, 2, 4, 5, 6, 9}`
 *
 * - 1-based 입력을 0-based 인덱스로 변환한다.
 * - 범위 표기(`-`)와 쉼표 구분을 지원한다.
 * - 잘못된 형식이거나 pageCount 를 초과하는 값이 포함되면 빈 벡터를 반환한다.
 *
 * @param s         페이지 범위 문자열 (예: "1,3,5-7")
 * @param pageCount PDF 의 전체 페이지 수 (유효 범위 검증에 사용)
 * @return 0-based 페이지 인덱스 목록. 오류 시 빈 벡터.
 */
std::vector<int> ParsePageRanges(const CString& s, int pageCount);

// ────────────────────────────────────────────────────────────

/**
 * @brief 탭2 — PDF 도구(분할/합치기/페이지 추출) 및 AI 요약 다이얼로그.
 *
 * 주요 기능:
 * - **분할(Split)**: 각 페이지를 별도 PDF 로 분리하거나 범위 지정 분할
 * - **합치기(Merge)**: 여러 PDF 를 하나의 다중 페이지 PDF 로 결합
 * - **추출(Extract)**: 지정 페이지를 JPG 또는 단일 PDF 로 추출
 * - **AI 요약**: WinRT OCR(최대 5페이지) → Ollama llama3.2:3b → 메모장 열기
 *
 * ### 스레드 구조
 * - `m_worker`: 분할/합치기/추출 작업 스레드. WM_PDF_TOOL_DONE 으로 결과 전달.
 * - `m_summaryWorker`: OCR + Ollama 요약 스레드. raw HWND 캡처 후 detach() 실행.
 *   **반드시** `::IsWindow()` 가드 후 UI 조작 — 다이얼로그 소멸 후 크래시 방지.
 *
 * ### PDF 드롭 후 페이지 수 비동기 로드
 * `AddPdfFiles` → pages=-1("..." 표시) → 백그라운드 GetPageCount → WM_PDF_PAGES_LOADED
 * → OnPdfPagesLoaded 갱신. PostMessage 실패 시 lParam new vector* 즉시 delete 필수.
 */
class CPdfToolsDlg : public CTabDlgBase
{
public:
    /**
     * @brief 생성자.
     * @param pParent 부모 윈도우 포인터 (nullptr 가능)
     */
    explicit CPdfToolsDlg(CWnd* pParent = nullptr);

    /** @brief 탭2 다이얼로그 리소스 ID. */
    enum { IDD = IDD_TAB2 };

    /**
     * @brief 호스트가 WM_PDF_TOOL_DONE 을 수신했을 때 호출한다.
     *
     * 작업 완료 상태로 전환하고 상태 레이블과 버튼 상태를 갱신한다.
     *
     * @param bSuccess true 이면 성공, false 이면 실패
     */
    void NotifyRunDone(bool bSuccess);

    /**
     * @brief PDF 파일 경로 목록을 목록에 추가한다.
     *
     * 각 파일의 페이지 수는 즉시 -1 로 설정("..." 표시)하고,
     * 백그라운드 스레드에서 GetPageCount 완료 후 WM_PDF_PAGES_LOADED 로 갱신한다.
     *
     * @param paths     추가할 PDF 파일 경로 목록
     * @param pEditPath 호스트의 경로 에디트박스 (nullptr 가능)
     */
    void AddPdfFiles(const std::vector<CString>& paths, CEdit* pEditPath = nullptr);

    // ── CTabDlgBase 인터페이스 구현 ──────────────────────────

    /**
     * @brief 호스트 [찾아보기] 클릭 시 PDF 파일 선택 대화상자를 열고 결과를 추가한다.
     * @param editPath 호스트의 경로 에디트박스
     */
    virtual void OnCommonBrowse(CEdit& editPath) override;

    /**
     * @brief 호스트 [변환] 클릭 시 현재 모드(Split/Merge/Extract)에 따라 작업을 시작한다.
     * @param bMerge 탭2에서는 사용되지 않음
     */
    virtual void OnCommonRun(bool bMerge) override;

    /** @brief 선택 항목을 위로 이동한다. */
    virtual void OnCommonMoveUp() override;

    /** @brief 선택 항목을 아래로 이동한다. */
    virtual void OnCommonMoveDown() override;

    /**
     * @brief 목록 삭제/초기화를 수행한다.
     * @param bConvDone true 이면 전체 초기화, false 이면 선택 항목 제거
     */
    virtual void OnCommonClear(bool bConvDone) override;

    /**
     * @brief 탭이 활성화될 때 공통 editPath 플레이스홀더와 합치기 체크박스를 설정한다.
     *
     * 탭2는 합치기 체크박스를 숨긴다.
     *
     * @param editPath   호스트의 공통 경로 에디트박스
     * @param checkMerge 호스트의 합치기 체크박스
     */
    virtual void OnTabActivated(CEdit& editPath, CButton& checkMerge) override;

    virtual bool    IsRunning()   override; ///< 작업 스레드 실행 중이면 true
    virtual bool    CanMoveUp()   override; ///< 선택 항목 위 이동 가능 여부
    virtual bool    CanMoveDown() override; ///< 선택 항목 아래 이동 가능 여부
    virtual bool    CanDelete()   override; ///< 삭제 가능 여부

    /**
     * @brief 변환 실행 가능 여부.
     *
     * pages < 0 인 항목(페이지 수 로딩 중)이 있으면 false 를 반환한다.
     */
    virtual bool    CanRun()      override;

    /** @return 항상 false — 탭2는 합치기 체크박스를 숨긴다. */
    virtual bool    ShowMerge()   override { return false; }

    /** @return 현재 작업 중이면 "중단", 아니면 모드에 따른 레이블 */
    virtual CString RunLabel()    override;

    /** @brief 언어 전환 시 컨트롤 텍스트와 툴팁을 갱신한다. */
    virtual void    ApplyLanguage() override;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override {}
    virtual void OnCancel() override {}

    CButton m_radSplit;   ///< "분할" 모드 라디오
    CButton m_radMerge;   ///< "합치기" 모드 라디오
    CButton m_radExtract; ///< "추출" 모드 라디오

    CListCtrl m_listPdf; ///< PDF 파일 목록 컨트롤

    /**
     * @brief 변환 대기열의 PDF 항목 하나를 나타내는 구조체.
     */
    struct PdfEntry
    {
        CString path;          ///< 파일 전체 경로
        CString name;          ///< 파일명 (표시용)
        int     pages = 0;     ///< 페이지 수 (-1 이면 로딩 중, "..." 표시)
        CString remark;        ///< 변환 결과 비고 문자열
    };
    std::vector<PdfEntry> m_entries; ///< PDF 파일 항목 목록

    CButton m_radSplitEach;  ///< "각 페이지" 분할 라디오
    CButton m_radSplitRange; ///< "범위 지정" 분할 라디오
    CEdit   m_editSplitRange; ///< 범위 지정 에디트박스 (예: "1,3,5-7")

    CStatic m_lblMergeName;  ///< "출력 파일명" 레이블 (합치기 모드)
    CEdit   m_editMergeName; ///< 합치기 출력 파일명 에디트박스

    CStatic m_lblExtractPages; ///< "추출 페이지" 레이블
    CStatic m_lblExtractHint;  ///< 페이지 형식 힌트 레이블
    CEdit   m_editExtractPages; ///< 추출 페이지 범위 에디트박스
    CButton m_radExtSingle;    ///< "단일 PDF" 추출 라디오
    CButton m_radExtEach;      ///< "각각 JPG" 추출 라디오

    CStatic      m_lblOutputPath;   ///< "출력 폴더" 레이블
    CEdit        m_editOutput;      ///< 출력 폴더 경로 에디트박스
    CColorButton m_btnOutputBrowse; ///< 출력 폴더 탐색 버튼
    CStatic      m_lblStatus;       ///< 전체 작업 상태 레이블

    CButton m_radioSummKo;    ///< AI 요약 언어 — 한국어 라디오
    CButton m_radioSummEn;    ///< AI 요약 언어 — 영어 라디오
    CButton m_btnSummarize;   ///< AI 요약 시작 버튼
    CButton m_btnOpenNotepad; ///< 요약 결과 메모장 열기 버튼 (성공 후 활성)
    CEdit   m_editSummary;    ///< 요약 결과 표시 에디트박스 (읽기 전용)

    /**
     * @brief 현재 선택된 PDF 도구 모드.
     *
     * 라디오 버튼 클릭 시 ApplyMode() 로 전환되며, 관련 옵션 컨트롤을 표시/숨긴다.
     */
    enum class Mode { Split, Merge, Extract } m_mode = Mode::Split;

    bool         m_bRunning = false; ///< 분할/합치기/추출 작업 실행 중 플래그
    std::thread  m_worker;           ///< PDF 작업 백그라운드 스레드
    std::thread  m_summaryWorker;    ///< AI 요약 백그라운드 스레드 (detach 실행)

    CToolTipCtrl m_toolTip; ///< 컨트롤 툴팁

    int   m_initCx = 0, m_initCy = 0; ///< 초기 클라이언트 영역 크기
    CRect m_rcList0, m_rcOutputLbl0, m_rcOutput0, m_rcBrowse0, m_rcStatus0;
    CRect m_rcSummKo0, m_rcSummEn0, m_rcSummarize0, m_rcOpenNotepad0, m_rcSummaryEdit0; ///< 초기 컨트롤 rect

    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    /** @brief 드롭된 PDF 파일/폴더를 목록에 추가한다. */
    afx_msg void OnDropFiles(HDROP hDropInfo);

    /**
     * @brief [요약] 버튼 클릭 시 AI 요약 스레드를 시작한다.
     *
     * 선택된 PDF 의 최대 5페이지를 OCR → Ollama 로 요약한다.
     * detach() 스레드이므로 내부에서 반드시 ::IsWindow() 가드 필요.
     */
    afx_msg void OnBnClickedSummarize();

    /**
     * @brief [메모장 열기] 버튼 클릭 시 요약 결과를 임시 .txt 파일로 저장하고 메모장으로 연다.
     *
     * GetTempFileNameW → MoveFileW(.txt 확장자) → UTF-8 BOM 저장 → ShellExecuteW(notepad.exe).
     * 실패 경로마다 DeleteFileW 필수.
     */
    afx_msg void OnBnClickedOpenNotepad();

    /**
     * @brief 백그라운드 스레드의 GetPageCount 완료 시 수신하는 커스텀 메시지 핸들러.
     *
     * lParam 은 `new vector<pair<CString,int>>*` — 반드시 이 핸들러에서 delete 할 것.
     */
    afx_msg LRESULT OnPdfPagesLoaded(WPARAM wParam, LPARAM lParam);

    /** @brief [분할] 라디오 클릭 시 모드를 Split 으로 전환한다. */
    afx_msg void OnBnClickedRadioSplit();

    /** @brief [합치기] 라디오 클릭 시 모드를 Merge 로 전환하고 출력 파일명을 자동 입력한다. */
    afx_msg void OnBnClickedRadioMerge();

    /** @brief [추출] 라디오 클릭 시 모드를 Extract 로 전환한다. */
    afx_msg void OnBnClickedRadioExtract();

    /** @brief 출력 폴더 탐색 버튼 클릭 시 폴더 선택 대화상자를 열고 경로를 설정한다. */
    afx_msg void OnBnClickedOutputBrowse();

    /** @brief [범위 지정] 라디오 클릭 시 범위 에디트박스를 활성/비활성화한다. */
    afx_msg void OnBnClickedRadioSplitRange();

    /** @brief 목록 선택 변경 시 버튼 상태와 요약 버튼을 갱신한다. */
    afx_msg void OnLvnItemChangedListPdf(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 우클릭 시 컨텍스트 메뉴를 표시한다. */
    afx_msg void OnNMRClickListPdf(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 더블클릭 시 파일을 외부 앱으로 연다. */
    afx_msg void OnNMDblclkListPdf(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 창 크기 변경 시 컨트롤 위치를 재조정한다. */
    afx_msg void OnSize(UINT nType, int cx, int cy);

    /**
     * @brief 소멸 시 작업 스레드를 join 하고 리소스를 해제한다.
     *
     * m_summaryWorker 는 detach() 이므로 join 불가 — OnDestroy 에서 처리하지 않는다.
     * m_worker 는 joinable() 검사 후 join().
     */
    afx_msg void OnDestroy();

    /**
     * @brief 모드(Split/Merge/Extract)에 따라 관련 컨트롤을 표시/숨기고
     *        m_mode 를 갱신한 후 버튼 상태를 재계산한다.
     * @param mode 전환할 새 모드
     */
    void ApplyMode(Mode mode);

    /**
     * @brief 합치기 모드에서 m_entries 의 첫 번째 파일명을 기반으로
     *        m_editMergeName 에 출력 파일명을 자동 입력한다.
     */
    void AutoFillMergeName();

    /**
     * @brief m_entries 전체를 리스트 컨트롤에 다시 그린다.
     *
     * 항목 추가·삭제·이동·페이지 수 갱신 후 호출하여 뷰를 동기화한다.
     */
    void RefreshList();

    /**
     * @brief 리스트 컨트롤의 컬럼 너비를 클라이언트 폭에 맞게 조정한다.
     */
    void AdjustListColumns();

    /**
     * @brief 현재 상태에 맞게 실행/삭제/요약 버튼 활성 여부를 갱신한다.
     *
     * CanRun(), IsRunning(), 선택 항목 유무를 종합하여 각 버튼 Enable/Disable.
     */
    void UpdateButtonState();

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

    /** @brief 분할 작업을 백그라운드 스레드에서 실행한다. */
    void RunSplit();

    /** @brief 합치기 작업을 백그라운드 스레드에서 실행한다. */
    void RunMerge();

    /** @brief 페이지 추출 작업을 백그라운드 스레드에서 실행한다. */
    void RunExtract();

    DECLARE_MESSAGE_MAP()
};
