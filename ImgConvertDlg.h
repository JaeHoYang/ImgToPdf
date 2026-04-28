#pragma once
#include "pch.h"
#include "resource.h"
#include "TabDlgBase.h"
#include "FileListCtrl.h"
#include "ImagePreviewCtrl.h"
#include "ConvertWorker.h"
#include "ColorButton.h"

/**
 * @brief 탭1 — 이미지 ↔ PDF 양방향 변환 다이얼로그.
 *
 * 주요 기능:
 * - 드래그앤드롭 및 찾아보기로 이미지/PDF 파일 추가
 * - 이미지 → PDF 병렬 변환 (ConvertWorker 스레드풀)
 * - PDF → 페이지별 JPG 추출 (PdfConverter, WinRT)
 * - 합치기 모드: 여러 이미지를 단일 다중페이지 PDF 로
 * - JPEG 품질 슬라이더 (10~100)
 * - 출력 폴더 지정 (비워두면 원본 파일 위치)
 * - 이미지/PDF 미리보기 (CImagePreviewCtrl)
 */
class CImgConvertDlg : public CTabDlgBase
{
public:
    /**
     * @brief 생성자.
     * @param pParent 부모 윈도우 포인터 (nullptr 가능)
     */
    explicit CImgConvertDlg(CWnd* pParent = nullptr);

    /** @brief 탭1 다이얼로그 리소스 ID. */
    enum { IDD = IDD_TAB1 };

    /**
     * @brief 진행 중인 변환을 즉시 중단 요청한다.
     *
     * m_bStopRequested 플래그를 true 로 설정하여 워커 스레드들이
     * 다음 파일부터 건너뛰도록 한다.
     */
    void Stop();

    /**
     * @brief 호스트가 WM_CONVERT_PROGRESS 를 수신했을 때 호출한다.
     *
     * 해당 항목의 상태 아이콘과 비고 텍스트를 갱신한다.
     *
     * @param idx 항목 인덱스 (0-based)
     * @param st  새 변환 상태 (ConvertStatus)
     */
    void SetFileStatus(int idx, ConvertStatus st);

    /**
     * @brief 호스트가 WM_CONVERT_DONE 을 수신했을 때 호출한다.
     *
     * 변환 완료 상태로 전환하고, 호스트에 WM_TAB_STATE_CHANGED 를 알린다.
     */
    void NotifyConvertDone();

    /**
     * @brief 현재 목록의 항목 수를 반환한다.
     * @return m_listFiles 의 항목 수
     */
    int  GetEntryCount() const { return (int)m_listFiles.GetEntries().size(); }

    /**
     * @brief 파일 경로 목록을 목록에 추가한다.
     *
     * 각 경로의 확장자를 검사하여 지원 형식만 추가하고,
     * 이미 있는 경로(중복)는 건너뛴다.
     *
     * @param paths 추가할 파일 경로 목록
     */
    void AddFiles(const std::vector<CString>& paths);

    // ── CTabDlgBase 인터페이스 구현 ──────────────────────────

    /**
     * @brief 호스트 [찾아보기] 클릭 시 파일/폴더 선택 대화상자를 열고 결과를 추가한다.
     * @param editPath 호스트의 경로 에디트박스 (경로 표시용)
     */
    virtual void OnCommonBrowse(CEdit& editPath) override;

    /**
     * @brief 호스트 [변환] 클릭 시 변환을 시작한다.
     *
     * 출력 폴더 유효성 검사 후 ConvertWorker::Start() 를 호출한다.
     * 진행 중이면 Stop() 을 호출한다.
     *
     * @param bMerge true 이면 합치기 모드
     */
    virtual void OnCommonRun(bool bMerge) override;

    /** @brief 선택 항목을 위로 이동한다. */
    virtual void OnCommonMoveUp() override;

    /** @brief 선택 항목을 아래로 이동한다. */
    virtual void OnCommonMoveDown() override;

    /**
     * @brief 목록 삭제/초기화를 수행한다.
     * @param bConvDone true 이면 변환 완료 후 전체 초기화, false 이면 선택 항목 제거
     */
    virtual void OnCommonClear(bool bConvDone) override;

    /**
     * @brief 합치기 체크박스 변경 시 FileListCtrl 의 합치기 모드를 동기화한다.
     * @param bMerge 새 합치기 상태
     */
    virtual void OnCommonMergeChanged(bool bMerge) override;

    /**
     * @brief 탭이 활성화될 때 공통 editPath 플레이스홀더와 합치기 체크박스를 설정한다.
     * @param editPath  호스트의 공통 경로 에디트박스
     * @param checkMerge 호스트의 합치기 체크박스 (탭1에서만 표시)
     */
    virtual void OnTabActivated(CEdit& editPath, CButton& checkMerge) override;

    virtual bool    IsRunning()    override; ///< 변환 중이면 true
    virtual bool    IsDone()       override; ///< 변환 완료 상태이면 true
    virtual bool    CanMoveUp()    override; ///< 선택 항목 위 이동 가능 여부
    virtual bool    CanMoveDown()  override; ///< 선택 항목 아래 이동 가능 여부
    virtual bool    CanDelete()    override; ///< 삭제 가능 여부
    virtual bool    CanRun()       override; ///< 변환 실행 가능 여부
    virtual bool    CanMerge()     override; ///< 합치기 모드 사용 가능 여부

    /** @return 항상 true — 탭1은 합치기 체크박스를 표시한다. */
    virtual bool    ShowMerge()    override { return true; }

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
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    CFileListCtrl     m_listFiles;       ///< 파일 목록 (FileListCtrl)
    CImagePreviewCtrl m_preview;         ///< 이미지/썸네일 미리보기
    CSliderCtrl       m_sliderQuality;   ///< JPEG 출력 품질 슬라이더 (10~100)
    CStatic           m_lblQuality;      ///< 품질 값 표시 레이블
    CStatic           m_lblOutputPath;   ///< "출력 폴더" 레이블
    CEdit             m_editOutput;      ///< 출력 폴더 경로 에디트박스
    CColorButton      m_btnOutputBrowse; ///< 출력 폴더 탐색 버튼
    CToolTipCtrl      m_toolTip;         ///< 컨트롤 툴팁

    int   m_initCx = 0, m_initCy = 0;   ///< 초기 클라이언트 영역 크기
    CRect m_rcList0, m_rcPreview0, m_rcSlider0, m_rcQualLbl0;
    CRect m_rcOutputLbl0, m_rcOutput0, m_rcOutputBrowse0; ///< 초기 컨트롤 rect (리사이징 기준)

    bool              m_bConverting     = false; ///< 변환 진행 중 플래그
    bool              m_bConversionDone = false; ///< 변환 완료 플래그
    std::atomic<bool> m_bStopRequested{ false };  ///< 중단 요청 플래그
    ConvertWorker     m_worker;                  ///< 멀티스레드 변환 워커
    CString           m_sTempPreviewJpg;         ///< PDF 페이지 미리보기용 임시 JPG 경로

    /** @brief 품질 슬라이더 이동 시 레이블을 갱신한다. */
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

    /** @brief 드롭된 파일/폴더를 목록에 추가한다. */
    afx_msg void OnDropFiles(HDROP hDropInfo);

    /** @brief 출력 폴더 탐색 버튼 클릭 시 폴더 선택 대화상자를 열고 경로를 설정한다. */
    afx_msg void OnBnClickedOutputBrowse();

    /** @brief 목록 선택 변경 시 미리보기를 갱신하고 버튼 상태를 업데이트한다. */
    afx_msg void OnLvnItemChangedListFiles(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 클릭 시 선택 항목의 이미지를 미리보기에 로드한다. */
    afx_msg void OnNMClickListFiles(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 목록 항목 더블클릭 시 파일을 외부 앱으로 연다. */
    afx_msg void OnNMDblclkListFiles(NMHDR* pNMHDR, LRESULT* pResult);

    /**
     * @brief FileListCtrl 의 RemoveSelected 완료 후 수신하는 커스텀 메시지 핸들러.
     *
     * erase 완료 후 합치기 체크박스 상태와 버튼 상태를 갱신한다.
     */
    afx_msg LRESULT OnListEntriesChanged(WPARAM wParam, LPARAM lParam);

    /** @brief 창 크기 변경 시 컨트롤 위치를 재조정한다. */
    afx_msg void OnSize(UINT nType, int cx, int cy);

    /** @brief 소멸 시 임시 미리보기 파일을 삭제한다. */
    afx_msg void OnDestroy();

    /**
     * @brief 폴더 경로에서 지원 확장자 파일을 재귀적으로 수집한다.
     * @param folderPath 탐색할 폴더 경로
     */
    void AddFolder(const CString& folderPath);

    /**
     * @brief 파일 확장자가 지원 형식인지 검사한다.
     * @param ext 소문자 확장자 (점 포함, 예: L".jpg")
     * @return 지원 형식이면 true
     */
    bool IsSupportedExt(const CString& ext) const;

    /**
     * @brief 초기 rect 와 delta 를 사용하여 컨트롤 위치를 재조정한다.
     * @param cx 현재 클라이언트 너비
     * @param cy 현재 클라이언트 높이
     */
    void ResizeControls(int cx, int cy);

    /**
     * @brief PDF 페이지 항목 유무에 따라 합치기 체크박스 활성 여부를 결정한다.
     *
     * PdfPage 항목이 하나라도 있으면 합치기 체크박스를 비활성화한다.
     */
    void UpdateMergeCheckState();

    /** @brief 버튼 상태 재계산을 호스트에 요청하는 WM_TAB_STATE_CHANGED 를 PostMessage 한다. */
    void NotifyHostStateChanged();

    DECLARE_MESSAGE_MAP()
};
