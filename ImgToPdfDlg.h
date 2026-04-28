#pragma once
#include "pch.h"
#include "resource.h"
#include "AppLang.h"
#include "TabDlgBase.h"
#include "ImgConvertDlg.h"
#include "PdfToolsDlg.h"
#include "MdConvertDlg.h"
#include "WordConvertDlg.h"
#include "PptConvertDlg.h"
#include "ProgressLabel.h"
#include "ColorButton.h"

/**
 * @brief Visual Theme 없이 단색 배경을 렌더링하는 탭 컨트롤 서브클래스.
 *
 * 기본 CTabCtrl 은 비스타/에어로 테마를 적용하여 탭 안쪽 배경이 어둡게 보일 수 있다.
 * `SetWindowTheme(L"", L"")` 으로 테마를 제거하고, `WM_ERASEBKGND` 를 직접 처리하여
 * 탭 클라이언트 영역을 RGB(248, 249, 252) 단색으로 채운다.
 *
 * `TCS_OWNERDRAWFIXED` 스타일로 탭 라벨 그리기를 호스트(CImgToPdfDlg::OnDrawItem) 에
 * 위임한다.
 */
class CSimpleTabCtrl : public CTabCtrl
{
protected:
    /**
     * @brief 서브클래싱 완료 시 테마를 제거하고 TCS_OWNERDRAWFIXED 스타일을 추가한다.
     *
     * DDX_Control 바인딩 후 자동으로 호출된다.
     */
    void PreSubclassWindow() override {
        CTabCtrl::PreSubclassWindow();
        SetWindowTheme(GetSafeHwnd(), L"", L"");
        ModifyStyle(0, TCS_OWNERDRAWFIXED);
    }

    /**
     * @brief WM_ERASEBKGND 를 처리하여 탭 클라이언트 배경을 단색으로 채운다.
     *
     * 그 외 메시지는 기본 CTabCtrl 로 전달한다.
     *
     * @param msg    Windows 메시지 ID
     * @param wp     wParam (WM_ERASEBKGND 에서는 HDC)
     * @param lp     lParam
     * @return 기본 WindowProc 처리 결과
     */
    LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp) override {
        if (msg == WM_ERASEBKGND) {
            CDC dc; dc.Attach((HDC)wp);
            CRect rc; GetClientRect(&rc);
            dc.FillSolidRect(rc, RGB(248, 249, 252));
            dc.Detach();
            return 1;
        }
        return CTabCtrl::WindowProc(msg, wp, lp);
    }
};

/**
 * @brief 애플리케이션 메인 다이얼로그 — 5개 탭 다이얼로그를 호스팅하는 셸.
 *
 * ### 탭 구조
 * | 인덱스 | 클래스             | 기능                      |
 * |--------|--------------------|---------------------------|
 * | 0      | CImgConvertDlg     | 이미지 ↔ PDF 변환 (탭1)  |
 * | 1      | CPdfToolsDlg       | PDF 도구 + AI 요약 (탭2)  |
 * | 2      | CMdConvertDlg      | Markdown 변환 (탭3)       |
 * | 3      | CWordConvertDlg    | Word → PDF 변환 (탭4)     |
 * | 4      | CPptConvertDlg     | PPT → PDF 변환 (탭5)      |
 *
 * ### 공통 컨트롤
 * 호스트 창에 경로 에디트박스, [찾아보기], [변환], 진행바, 이동/삭제 버튼을 배치하며,
 * 활성 탭의 CTabDlgBase 인터페이스를 통해 각 탭 다이얼로그와 상호작용한다.
 *
 * ### 탭 순서 저장
 * 사용자가 탭을 드래그(SwapTabs) 하면 현재 순서를 레지스트리에 저장(SaveTabOrder)하고
 * 다음 실행 시 복원(LoadTabOrder)한다.
 *
 * ### 드롭 파일 라우팅
 * 메인 창 또는 탭으로 드롭된 파일/폴더를 RouteDroppedFiles 가 확장자별로
 * 적절한 탭 다이얼로그에 분배한다. WM_ROUTE_DROP 으로도 수신한다.
 *
 * ### 언어 전환
 * 시스템 메뉴 IDM_LANG_TOGGLE(0x1002) → SetLang() → ApplyLanguage() 호출 체인으로
 * 재시작 없이 런타임 한국어/영어 전환을 제공한다.
 */
class CImgToPdfDlg : public CDialogEx
{
public:
    /**
     * @brief 생성자.
     * @param pParent 부모 윈도우 포인터 (nullptr 가능)
     */
    explicit CImgToPdfDlg(CWnd* pParent = nullptr);

    /** @brief 메인 다이얼로그 리소스 ID. */
    enum { IDD = IDD_IMGTOPDF_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;
    virtual void OnOK() override {}
    virtual void OnCancel() override;

    // ── 공통 컨트롤 (모든 탭이 공유) ──────────────────────────
    CStatic        m_lblPath;           ///< "경로" 레이블
    CEdit          m_editPath;          ///< 경로/출력 에디트박스 (플레이스홀더 포함)
    CButton        m_checkMerge;        ///< 합치기 체크박스 (탭1에서만 표시)
    CColorButton   m_btnBrowse;         ///< [찾아보기] 버튼
    CColorButton   m_btnConvert;        ///< [변환/중단] 버튼
    CProgressCtrl  m_progress;          ///< 변환 진행 프로그레스바
    CProgressLabel m_staticProgressTxt; ///< (완료/실패/총) 3색 텍스트 레이블
    CMFCButton     m_btnMoveUp;         ///< [▲] 위로 이동 버튼
    CMFCButton     m_btnMoveDown;       ///< [▼] 아래로 이동 버튼
    CMFCButton     m_btnClear;          ///< [삭제/초기화] 버튼
    CToolTipCtrl   m_toolTip;           ///< 공통 컨트롤 툴팁

    // ── 탭 컨트롤 ────────────────────────────────────────────
    CSimpleTabCtrl m_tabCtrl;    ///< 탭 컨트롤 (TCS_OWNERDRAWFIXED)
    CMFCButton     m_btnTabPrev; ///< [◀] 탭 이전 버튼
    CMFCButton     m_btnTabNext; ///< [▶] 탭 다음 버튼

    // ── 탭 자식 다이얼로그 ────────────────────────────────────
    CImgConvertDlg  m_dlgTab1; ///< 탭1 — 이미지 변환 다이얼로그
    CPdfToolsDlg    m_dlgTab2; ///< 탭2 — PDF 도구 다이얼로그
    CMdConvertDlg   m_dlgTab3; ///< 탭3 — Markdown 변환 다이얼로그
    CWordConvertDlg m_dlgTab4; ///< 탭4 — Word 변환 다이얼로그
    CPptConvertDlg  m_dlgTab5; ///< 탭5 — PPT 변환 다이얼로그

    /**
     * @brief 현재 탭 표시 순서 배열.
     *
     * 사용자가 탭 순서를 변경하면 이 배열의 포인터 순서가 바뀐다.
     * m_tabDlgs[m_activeTab] 이 현재 활성 탭을 가리킨다.
     */
    CTabDlgBase* m_tabDlgs[5] = {};

    int m_activeTab  = 0; ///< 현재 활성 탭 인덱스 (0-based)
    int m_cntSuccess = 0; ///< 현재 변환 세션의 성공 항목 수
    int m_cntFail    = 0; ///< 현재 변환 세션의 실패 항목 수
    int m_cntTotal   = 0; ///< 현재 변환 세션의 전체 항목 수

    // ── 레이아웃 기준점 ───────────────────────────────────────
    int   m_initCx = 0, m_initCy = 0; ///< 초기 클라이언트 영역 크기
    CRect m_rcEdit0, m_rcMerge0, m_rcBrowse0, m_rcConvert0;
    CRect m_rcProg0, m_rcProgTxt0, m_rcMoveUp0, m_rcMoveDown0, m_rcClear0, m_rcTab0; ///< 초기 컨트롤 rect

    // ── 메시지 핸들러 ─────────────────────────────────────────

    /**
     * @brief 시스템 메뉴 항목 클릭을 처리한다.
     *
     * IDM_HELP_USAGE → ShowHelpDialog(),
     * IDM_LANG_TOGGLE(0x1002) → SetLang() → ApplyLanguage().
     */
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

    /** @brief F1 키 누름 시 사용법 대화상자를 표시한다. */
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

    /** @brief 메인 창에 드롭된 파일/폴더를 RouteDroppedFiles 로 분배한다. */
    afx_msg void OnDropFiles(HDROP hDropInfo);

    /** @brief 창 크기 변경 시 공통 컨트롤과 탭 컨트롤 위치를 재조정한다. */
    afx_msg void OnSize(UINT nType, int cx, int cy);

    /** @brief 최소 창 크기를 설정한다. */
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

    /** @brief 종료 시 탭 순서를 레지스트리에 저장하고 리소스를 해제한다. */
    afx_msg void OnDestroy();

    /** @brief 탭 선택 변경 시 이전 탭을 숨기고 새 탭을 ShowTab 으로 활성화한다. */
    afx_msg void OnTcnSelchangeTabCtrl(NMHDR* pNMHDR, LRESULT* pResult);

    /**
     * @brief TCS_OWNERDRAWFIXED 탭 항목을 직접 그린다.
     *
     * 활성 탭과 비활성 탭의 배경색·텍스트색을 구분하여 렌더링한다.
     */
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);

    /** @brief [◀] 버튼 클릭 시 이전 탭으로 이동하고, 탭 순서를 변경한다. */
    afx_msg void OnBnClickedTabPrev();

    /** @brief [▶] 버튼 클릭 시 다음 탭으로 이동하고, 탭 순서를 변경한다. */
    afx_msg void OnBnClickedTabNext();

    /** @brief [찾아보기] 클릭 시 활성 탭의 OnCommonBrowse 를 호출한다. */
    afx_msg void OnBnClickedBrowse();

    /** @brief [변환] 클릭 시 활성 탭의 OnCommonRun 을 호출한다. */
    afx_msg void OnBnClickedConvert();

    /** @brief [▲] 클릭 시 활성 탭의 OnCommonMoveUp 을 호출한다. */
    afx_msg void OnBnClickedMoveUp();

    /** @brief [▼] 클릭 시 활성 탭의 OnCommonMoveDown 을 호출한다. */
    afx_msg void OnBnClickedMoveDown();

    /** @brief [삭제] 클릭 시 활성 탭의 OnCommonClear 를 호출한다. */
    afx_msg void OnBnClickedClear();

    /** @brief 합치기 체크박스 변경 시 활성 탭의 OnCommonMergeChanged 를 호출한다. */
    afx_msg void OnBnClickedCheckMerge();

    /**
     * @brief 탭1 워커의 WM_CONVERT_PROGRESS 수신 핸들러.
     *
     * wParam=항목 인덱스, lParam=ConvertStatus. m_dlgTab1.SetFileStatus() 를 호출한다.
     */
    afx_msg LRESULT OnConvertProgress(WPARAM wParam, LPARAM lParam);

    /**
     * @brief 탭1 워커의 WM_CONVERT_DONE 수신 핸들러.
     *
     * wParam=성공 수, lParam=실패 수. 프로그레스 레이블을 갱신하고
     * m_dlgTab1.NotifyConvertDone() 을 호출한다.
     */
    afx_msg LRESULT OnConvertDone(WPARAM wParam, LPARAM lParam);

    /**
     * @brief 탭2 워커의 WM_PDF_TOOL_DONE 수신 핸들러.
     *
     * wParam=성공 여부(1/0). m_dlgTab2.NotifyRunDone() 을 호출한다.
     */
    afx_msg LRESULT OnPdfToolDone(WPARAM wParam, LPARAM lParam);

    /**
     * @brief 탭에서 보내는 WM_TAB_STATE_CHANGED 수신 핸들러.
     *
     * UpdateCommonState() 를 호출하여 공통 버튼 활성 상태를 재계산한다.
     */
    afx_msg LRESULT OnTabStateChanged(WPARAM wParam, LPARAM lParam);

    /**
     * @brief 탭에서 보내는 WM_ROUTE_DROP 수신 핸들러.
     *
     * lParam 은 `new vector<CString>*` — 이 핸들러에서 RouteDroppedFiles 후 delete.
     */
    afx_msg LRESULT OnRouteDrop(WPARAM wParam, LPARAM lParam);

    /**
     * @brief 탭3 워커의 WM_MD_CONVERT_DONE 수신 핸들러.
     *
     * wParam=성공 여부(1/0). m_dlgTab3.NotifyConvertDone() 을 호출한다.
     */
    afx_msg LRESULT OnMdConvertDone(WPARAM wParam, LPARAM lParam);

    /**
     * @brief 탭4 워커의 WM_WORD_CONVERT_DONE 수신 핸들러.
     *
     * wParam=성공 여부(1/0). m_dlgTab4.NotifyConvertDone() 을 호출한다.
     */
    afx_msg LRESULT OnWordConvertDone(WPARAM wParam, LPARAM lParam);

    /**
     * @brief 탭5 워커의 WM_PPT_CONVERT_DONE 수신 핸들러.
     *
     * wParam=성공 여부(1/0). m_dlgTab5.NotifyConvertDone() 을 호출한다.
     */
    afx_msg LRESULT OnPptConvertDone(WPARAM wParam, LPARAM lParam);

    // ── 헬퍼 ─────────────────────────────────────────────────

    /** @return 현재 활성 탭 다이얼로그 포인터 */
    CTabDlgBase* ActiveTab() { return m_tabDlgs[m_activeTab]; }

    /**
     * @brief 지정 인덱스의 탭을 활성화한다.
     *
     * 이전 탭을 ShowWindow(SW_HIDE) 하고, 새 탭을 ShowWindow(SW_SHOW) 한 뒤
     * OnTabActivated() 를 호출하여 공통 컨트롤을 초기화한다.
     *
     * @param idx 활성화할 탭 인덱스 (0-based)
     */
    void ShowTab(int idx);

    /**
     * @brief 드롭된 파일/폴더를 확장자별로 적절한 탭 다이얼로그에 분배한다.
     *
     * .md → 탭3, .docx/.doc → 탭4, .pptx/.ppt → 탭5, .pdf → 탭2,
     * 이미지 확장자 → 탭1, 폴더 → ScanFolder 후 재분배.
     *
     * @param paths 드롭된 파일/폴더 경로 목록
     */
    void RouteDroppedFiles(const std::vector<CString>& paths);

    /**
     * @brief 폴더를 재귀 탐색하여 파일 경로를 수집한다.
     * @param folder 탐색할 폴더 경로
     * @param out    수집된 파일 경로 출력 벡터
     */
    void ScanFolder(const CString& folder, std::vector<CString>& out);

    /**
     * @brief m_tabDlgs 배열에서 두 탭의 순서를 교환한다.
     *
     * 탭 라벨, 포인터 배열, 탭 컨트롤 순서를 모두 동기화한다.
     *
     * @param a 교환할 첫 번째 인덱스
     * @param b 교환할 두 번째 인덱스
     */
    void SwapTabs(int a, int b);

    /**
     * @brief 모든 탭 자식 다이얼로그의 위치를 탭 컨트롤 클라이언트 영역에 맞게 조정한다.
     *
     * ShowTab() 또는 OnSize() 호출 후 레이아웃을 동기화할 때 사용한다.
     */
    void RepositionTabDialogs();

    /** @brief [◀][▶] 탭 이동 버튼을 생성하여 탭 컨트롤 우측에 배치한다. */
    void CreateTabNavButtons();

    /** @brief 탭 컨트롤 크기에 따라 [◀][▶] 버튼 위치를 조정한다. */
    void PositionTabNavButtons();

    /**
     * @brief 현재 탭 위치에 따라 [◀][▶] 버튼 활성 여부를 갱신한다.
     *
     * 맨 첫 번째 탭이면 [◀] 비활성, 맨 마지막 탭이면 [▶] 비활성.
     */
    void UpdateTabNavButtons();

    /** @brief 현재 m_tabDlgs 순서를 레지스트리에 저장한다. */
    void SaveTabOrder();

    /** @brief 레지스트리에서 탭 순서를 불러와 m_tabDlgs 를 재배열한다. */
    void LoadTabOrder();

    /**
     * @brief CTabDlgBase 포인터로 m_tabDlgs 배열에서의 인덱스를 찾는다.
     * @param pDlg 찾을 탭 다이얼로그 포인터
     * @return 인덱스 (없으면 -1)
     */
    int  GetDlgIndex(CTabDlgBase* pDlg);

    /**
     * @brief 활성 탭의 상태를 읽어 공통 버튼(변환/삭제/이동/합치기)의 활성 여부를 갱신한다.
     *
     * WM_TAB_STATE_CHANGED 수신 또는 탭 전환 시 호출한다.
     */
    void UpdateCommonState();

    /**
     * @brief 창 크기 변경 시 공통 컨트롤(에디트박스, 버튼, 진행바 등)과 탭 컨트롤의
     *        위치를 초기 rect 와 delta 를 사용하여 재조정한다.
     * @param cx 현재 클라이언트 너비
     * @param cy 현재 클라이언트 높이
     */
    void ResizeCommonControls(int cx, int cy);

    /**
     * @brief F1 키 또는 시스템 메뉴 IDM_HELP_USAGE 클릭 시 사용법 대화상자를 표시한다.
     *
     * ::MessageBoxW 로 사용법 텍스트와 제작자 정보(jaeho9697@gmail.com)를 표시한다.
     */
    void ShowHelpDialog();

    /**
     * @brief 모든 공통 컨트롤과 5개 탭 자식 다이얼로그의 언어 문자열을 갱신한다.
     *
     * SetLang() 직후 호출된다. 탭 라벨, 버튼 텍스트, 툴팁, 각 탭의 ApplyLanguage() 포함.
     */
    void ApplyLanguage();

    /**
     * @brief 시스템 메뉴의 언어 전환 항목 텍스트를 현재 언어에 맞게 갱신한다.
     *
     * 한국어 상태이면 "Switch to English", 영어 상태이면 "한국어로 전환" 표시.
     */
    void UpdateLangMenuItem();

    DECLARE_MESSAGE_MAP()
};
