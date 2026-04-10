#pragma once
#include "pch.h"
#include "resource.h"
#include "FileListCtrl.h"
#include "ImagePreviewCtrl.h"
#include "ConvertWorker.h"
#include "ProgressLabel.h"

class CImgToPdfDlg : public CDialogEx
{
public:
    explicit CImgToPdfDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_IMGTOPDF_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override {}    // Enter 키로 닫힘 방지
    virtual void OnCancel() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    // ── 컨트롤 변수 ─────────────────────────────────────
    CStatic           m_lblPath;
    CEdit             m_editPath;
    CButton           m_checkMerge;
    CButton           m_btnBrowse;
    CButton           m_btnConvert;
    CButton           m_btnMoveUp;
    CButton           m_btnMoveDown;
    CButton           m_btnClear;
    CProgressCtrl     m_progress;
    CProgressLabel    m_staticProgressTxt;
    CToolTipCtrl      m_toolTip;
    CFileListCtrl     m_listFiles;
    CImagePreviewCtrl m_preview;

    // ── 초기 레이아웃 기준값 (OnInitDialog에서 캡처) ────────
    int   m_initCx = 0;
    int   m_initCy = 0;
    CRect m_rcEdit0, m_rcMerge0, m_rcBrowse0, m_rcConvert0;
    CRect m_rcProg0, m_rcProgTxt0, m_rcMoveUp0, m_rcMoveDown0, m_rcClear0, m_rcList0, m_rcPreview0;

    // ── 변환 카운트 ──────────────────────────────────────
    int               m_cntSuccess = 0;
    int               m_cntFail    = 0;
    int               m_cntTotal   = 0;

    // ── 상태 ────────────────────────────────────────────
    bool              m_bConverting      = false;
    bool              m_bConversionDone  = false;  // 변환 완료 후 전체 초기화 모드
    std::atomic<bool> m_bStopRequested{ false };
    ConvertWorker     m_worker;

    // ── 메시지 핸들러 ────────────────────────────────────
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnDropFiles(HDROP hDropInfo);
    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedConvert();
    afx_msg void OnBnClickedCheckMerge();
    afx_msg void OnBnClickedMoveUp();
    afx_msg void OnBnClickedMoveDown();
    afx_msg void OnBnClickedClear();
    afx_msg void OnLvnItemChangedListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMClickListFiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnConvertProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnConvertDone(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnListEntriesChanged(WPARAM wParam, LPARAM lParam);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnDestroy();

    // ── 헬퍼 ────────────────────────────────────────────
    void AddFiles(const std::vector<CString>& paths);
    void AddFolder(const CString& folderPath);
    bool IsSupportedExt(const CString& ext) const;
    void ResizeControls(int cx, int cy);
    void SetConvertingState(bool bConverting);
    void UpdateMergeCheckState();
    void UpdateMoveButtonState();
    void UpdateClearButtonState();
    void ShowHelpDialog();

    DECLARE_MESSAGE_MAP()
};
