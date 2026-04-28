#include "pch.h"
#include "ImgToPdfDlg.h"
#include "AppLang.h"

BEGIN_MESSAGE_MAP(CImgToPdfDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_HELPINFO()
    ON_WM_DROPFILES()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_DESTROY()
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CTRL, &CImgToPdfDlg::OnTcnSelchangeTabCtrl)
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_BTN_TAB_PREV,  &CImgToPdfDlg::OnBnClickedTabPrev)
    ON_BN_CLICKED(IDC_BTN_TAB_NEXT,  &CImgToPdfDlg::OnBnClickedTabNext)
    ON_BN_CLICKED(IDC_BTN_BROWSE,    &CImgToPdfDlg::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BTN_CONVERT,   &CImgToPdfDlg::OnBnClickedConvert)
    ON_BN_CLICKED(IDC_BTN_MOVE_UP,   &CImgToPdfDlg::OnBnClickedMoveUp)
    ON_BN_CLICKED(IDC_BTN_MOVE_DOWN, &CImgToPdfDlg::OnBnClickedMoveDown)
    ON_BN_CLICKED(IDC_BTN_CLEAR,     &CImgToPdfDlg::OnBnClickedClear)
    ON_BN_CLICKED(IDC_CHECK_MERGE,   &CImgToPdfDlg::OnBnClickedCheckMerge)
    ON_MESSAGE(WM_CONVERT_PROGRESS,  &CImgToPdfDlg::OnConvertProgress)
    ON_MESSAGE(WM_CONVERT_DONE,      &CImgToPdfDlg::OnConvertDone)
    ON_MESSAGE(WM_PDF_TOOL_DONE,     &CImgToPdfDlg::OnPdfToolDone)
    ON_MESSAGE(WM_TAB_STATE_CHANGED, &CImgToPdfDlg::OnTabStateChanged)
    ON_MESSAGE(WM_ROUTE_DROP,        &CImgToPdfDlg::OnRouteDrop)
    ON_MESSAGE(WM_MD_CONVERT_DONE,   &CImgToPdfDlg::OnMdConvertDone)
    ON_MESSAGE(WM_WORD_CONVERT_DONE, &CImgToPdfDlg::OnWordConvertDone)
    ON_MESSAGE(WM_PPT_CONVERT_DONE,  &CImgToPdfDlg::OnPptConvertDone)
END_MESSAGE_MAP()

CImgToPdfDlg::CImgToPdfDlg(CWnd* pParent)
    : CDialogEx(IDD_IMGTOPDF_DIALOG, pParent)
    , m_dlgTab1(this)
    , m_dlgTab2(this)
    , m_dlgTab3(this)
    , m_dlgTab4(this)
    , m_dlgTab5(this)
{
}

void CImgToPdfDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LBL_PATH,            m_lblPath);
    DDX_Control(pDX, IDC_EDIT_PATH,           m_editPath);
    DDX_Control(pDX, IDC_CHECK_MERGE,         m_checkMerge);
    DDX_Control(pDX, IDC_BTN_BROWSE,          m_btnBrowse);
    DDX_Control(pDX, IDC_BTN_CONVERT,         m_btnConvert);
    DDX_Control(pDX, IDC_PROGRESS,            m_progress);
    DDX_Control(pDX, IDC_STATIC_PROGRESS_TXT, m_staticProgressTxt);
    DDX_Control(pDX, IDC_BTN_MOVE_UP,         m_btnMoveUp);
    DDX_Control(pDX, IDC_BTN_MOVE_DOWN,       m_btnMoveDown);
    DDX_Control(pDX, IDC_BTN_CLEAR,           m_btnClear);
    DDX_Control(pDX, IDC_TAB_CTRL,            m_tabCtrl);
}

BOOL CImgToPdfDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    LoadSavedLang();
    DragAcceptFiles(TRUE);

    // icon
    HICON hIcon = AfxGetApp()->LoadIcon(IDI_IMGTOPDF);
    SetIcon(hIcon, TRUE);
    SetIcon(hIcon, FALSE);

    // system menu entries
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu)
    {
        pSysMenu->AppendMenu(MF_SEPARATOR);
        pSysMenu->AppendMenu(MF_STRING, IDM_HELP_USAGE, LS(IDS_MENU_HELP));
        pSysMenu->AppendMenu(MF_STRING, IDM_LANG_TOGGLE,
            (g_lang == Lang::KO) ? L"Switch to English" : L"\ud55c\uad6d\uc5b4\ub85c \uc804\ud658");
    }

    // background
    SetBackgroundColor(RGB(248, 249, 252));

    // progress bar — classic mode + edit-box style border
    SetWindowTheme(m_progress.GetSafeHwnd(), L"", L"");
    m_progress.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
    m_progress.SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    m_progress.SetRange32(0, 100);
    m_progress.SetPos(0);
    m_progress.SendMessage(PBM_SETBARCOLOR, 0, (LPARAM)RGB(37, 99, 235));
    m_progress.SendMessage(PBM_SETBKCOLOR,  0, (LPARAM)RGB(248, 249, 252));
    m_staticProgressTxt.SetIdle();

    // CColorButton: DrawItem 직접 구현 — 한글 깨짐 없이 색상 적용
    m_btnConvert.SetColors(RGB(37, 99, 235), RGB(29, 78, 216), RGB(255, 255, 255));
    m_btnBrowse.SetColors(RGB(255, 255, 255), RGB(235, 243, 255), RGB(37, 99, 235));

    // CMFCButton 소형 버튼: flat 호버 효과
    m_btnMoveUp.m_nFlatStyle   = CMFCButton::BUTTONSTYLE_SEMIFLAT;
    m_btnMoveDown.m_nFlatStyle = CMFCButton::BUTTONSTYLE_SEMIFLAT;
    m_btnClear.m_nFlatStyle    = CMFCButton::BUTTONSTYLE_SEMIFLAT;

    // editPath placeholder
    {
        CString cue = LS(IDS_CUE_PATH);
        m_editPath.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue);
    }

    // tooltips
    m_toolTip.Create(this, TTS_ALWAYSTIP);
    m_toolTip.SetMaxTipWidth(260);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 400);

    m_toolTip.AddTool(&m_editPath,        LS(IDS_TIP_EDITPATH));
    m_toolTip.AddTool(&m_checkMerge,      LS(IDS_TIP_MERGE));
    m_toolTip.AddTool(&m_btnBrowse,       LS(IDS_TIP_BROWSE));
    m_toolTip.AddTool(&m_btnConvert,      LS(IDS_TIP_CONVERT));
    m_toolTip.AddTool(&m_staticProgressTxt, LS(IDS_TIP_PROGRESS));
    m_toolTip.AddTool(&m_btnMoveUp,       LS(IDS_TIP_MOVEUP));
    m_toolTip.AddTool(&m_btnMoveDown,     LS(IDS_TIP_MOVEDOWN));
    m_toolTip.AddTool(&m_btnClear,        LS(IDS_TIP_CLEAR_BASE));
    m_toolTip.Activate(TRUE);

    // capture layout baseline
    CRect rcClient;
    GetClientRect(&rcClient);
    m_initCx = rcClient.Width();
    m_initCy = rcClient.Height();

    auto cap = [&](CWnd& w, CRect& rc) { w.GetWindowRect(&rc); ScreenToClient(&rc); };
    cap(m_editPath,          m_rcEdit0);
    cap(m_checkMerge,        m_rcMerge0);
    cap(m_btnBrowse,         m_rcBrowse0);
    cap(m_btnConvert,        m_rcConvert0);
    cap(m_progress,          m_rcProg0);
    cap(m_staticProgressTxt, m_rcProgTxt0);
    cap(m_btnMoveUp,         m_rcMoveUp0);
    cap(m_btnMoveDown,       m_rcMoveDown0);
    cap(m_btnClear,          m_rcClear0);
    cap(m_tabCtrl,           m_rcTab0);

    // register tab labels
    m_tabCtrl.InsertItem(0, LS(IDS_TAB1_LABEL));
    m_tabCtrl.InsertItem(1, LS(IDS_TAB2_LABEL));
    m_tabCtrl.InsertItem(2, LS(IDS_TAB3_LABEL));
    m_tabCtrl.InsertItem(3, LS(IDS_TAB4_LABEL));
    m_tabCtrl.InsertItem(4, LS(IDS_TAB5_LABEL));

    // pointer array (default order)
    m_tabDlgs[0] = &m_dlgTab1;
    m_tabDlgs[1] = &m_dlgTab2;
    m_tabDlgs[2] = &m_dlgTab3;
    m_tabDlgs[3] = &m_dlgTab4;
    m_tabDlgs[4] = &m_dlgTab5;

    // inject host HWND into each tab before Create()
    m_dlgTab1.m_hHostNotify = GetSafeHwnd();
    m_dlgTab2.m_hHostNotify = GetSafeHwnd();
    m_dlgTab3.m_hHostNotify = GetSafeHwnd();
    m_dlgTab4.m_hHostNotify = GetSafeHwnd();
    m_dlgTab5.m_hHostNotify = GetSafeHwnd();

    // create child dialogs as non-modal WS_CHILD children of the main dialog
    m_dlgTab1.Create(IDD_TAB1, this);
    m_dlgTab2.Create(IDD_TAB2, this);
    m_dlgTab3.Create(IDD_TAB3, this);
    m_dlgTab4.Create(IDD_TAB4, this);
    m_dlgTab5.Create(IDD_TAB5, this);

    // ◀▶ buttons (positioned over tab header row)
    CreateTabNavButtons();

    // restore saved tab order from registry
    LoadTabOrder();

    // size and show first tab
    RepositionTabDialogs();
    ShowTab(0);

    return TRUE;
}

BOOL CImgToPdfDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd())
        m_toolTip.RelayEvent(pMsg);
    return CDialogEx::PreTranslateMessage(pMsg);
}

// ── drag-and-drop routing ────────────────────────────────────

static bool IsImageExt(const CString& lower)
{
    return lower == _T(".jpg")  || lower == _T(".jpeg") ||
           lower == _T(".png")  || lower == _T(".bmp")  ||
           lower == _T(".tiff") || lower == _T(".tif")  || lower == _T(".gif");
}

void CImgToPdfDlg::ScanFolder(const CString& folder, std::vector<CString>& out)
{
    CString pattern = folder + _T("\\*");
    WIN32_FIND_DATA fd = {};
    HANDLE hFind = FindFirstFile(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        out.push_back(folder + _T("\\") + fd.cFileName);
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
}

void CImgToPdfDlg::RouteDroppedFiles(const std::vector<CString>& paths)
{
    std::vector<CString> imgFiles, pdfFiles, mdFiles, wordFiles, pptFiles;

    for (const auto& p : paths)
    {
        DWORD attr = GetFileAttributes(p);
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::vector<CString> found;
            ScanFolder(p, found);
            for (const auto& f : found)
            {
                CString ext = PathFindExtension(f);
                ext.MakeLower();
                if (IsImageExt(ext))                               imgFiles.push_back(f);
                else if (ext == _T(".pdf"))                        pdfFiles.push_back(f);
                else if (ext == _T(".md"))                         mdFiles.push_back(f);
                else if (ext == _T(".doc") || ext == _T(".docx"))  wordFiles.push_back(f);
                else if (ext == _T(".ppt") || ext == _T(".pptx"))  pptFiles.push_back(f);
            }
        }
        else
        {
            CString ext = PathFindExtension(p);
            ext.MakeLower();
            if (IsImageExt(ext))                               imgFiles.push_back(p);
            else if (ext == _T(".pdf"))                        pdfFiles.push_back(p);
            else if (ext == _T(".md"))                         mdFiles.push_back(p);
            else if (ext == _T(".doc") || ext == _T(".docx"))  wordFiles.push_back(p);
            else if (ext == _T(".ppt") || ext == _T(".pptx"))  pptFiles.push_back(p);
        }
    }

    if (!imgFiles.empty()) m_dlgTab1.AddFiles(imgFiles);
    if (!pdfFiles.empty())
    {
        m_dlgTab2.AddPdfFiles(pdfFiles);
        for (int i = 0; i < 5; ++i)
            if (m_tabDlgs[i] == &m_dlgTab2) { ShowTab(i); break; }
    }
    if (!mdFiles.empty())  m_dlgTab3.AddMdFiles(mdFiles);
    if (!wordFiles.empty())
    {
        m_dlgTab4.AddWordFiles(wordFiles);
        for (int i = 0; i < 5; ++i)
            if (m_tabDlgs[i] == &m_dlgTab4) { ShowTab(i); break; }
    }
    if (!pptFiles.empty())
    {
        m_dlgTab5.AddPptFiles(pptFiles);
        for (int i = 0; i < 5; ++i)
            if (m_tabDlgs[i] == &m_dlgTab5) { ShowTab(i); break; }
    }
}

void CImgToPdfDlg::OnDropFiles(HDROP hDropInfo)
{
    UINT count = DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0);
    std::vector<CString> paths;
    for (UINT i = 0; i < count; ++i)
    {
        TCHAR buf[MAX_PATH] = {};
        DragQueryFile(hDropInfo, i, buf, MAX_PATH);
        paths.emplace_back(buf);
    }
    DragFinish(hDropInfo);
    RouteDroppedFiles(paths);
}

LRESULT CImgToPdfDlg::OnRouteDrop(WPARAM, LPARAM lParam)
{
    auto* paths = reinterpret_cast<std::vector<CString>*>(lParam);
    if (paths) RouteDroppedFiles(*paths);
    return 0;
}

// ── common control button handlers ───────────────────────────

void CImgToPdfDlg::OnBnClickedBrowse()
{
    ActiveTab()->OnCommonBrowse(m_editPath);
}

void CImgToPdfDlg::OnBnClickedConvert()
{
    // init progress counters when starting tab1 conversion (not stopping)
    if (ActiveTab() == &m_dlgTab1 && !m_dlgTab1.IsRunning())
    {
        m_cntSuccess = 0;
        m_cntFail    = 0;
        m_cntTotal   = m_dlgTab1.GetEntryCount();
        m_progress.SetRange32(0, m_cntTotal > 0 ? m_cntTotal : 1);
        m_progress.SetPos(0);
        m_staticProgressTxt.SetCounts(0, 0, m_cntTotal);
    }
    ActiveTab()->OnCommonRun(m_checkMerge.GetCheck() == BST_CHECKED);
}

void CImgToPdfDlg::OnBnClickedMoveUp()
{
    ActiveTab()->OnCommonMoveUp();
}

void CImgToPdfDlg::OnBnClickedMoveDown()
{
    ActiveTab()->OnCommonMoveDown();
}

void CImgToPdfDlg::OnBnClickedClear()
{
    bool wasDone = ActiveTab()->IsDone();
    ActiveTab()->OnCommonClear(wasDone);

    // if a full reset just happened (wasDone is now false), clear progress display
    if (wasDone && !ActiveTab()->IsDone())
    {
        m_cntSuccess = 0; m_cntFail = 0; m_cntTotal = 0;
        m_progress.SetPos(0);
        m_staticProgressTxt.SetIdle();
        m_editPath.SetWindowText(_T(""));
    }
    UpdateCommonState();
}

void CImgToPdfDlg::OnBnClickedCheckMerge()
{
    ActiveTab()->OnCommonMergeChanged(m_checkMerge.GetCheck() == BST_CHECKED);
}

// ── common state update ───────────────────────────────────────

void CImgToPdfDlg::UpdateCommonState()
{
    if (!m_tabCtrl.GetSafeHwnd()) return;
    CTabDlgBase* active = ActiveTab();
    if (!active) return;

    m_btnConvert.SetWindowText(active->RunLabel());
    m_btnConvert.EnableWindow(active->CanRun() || active->IsRunning());
    m_btnMoveUp.EnableWindow(active->CanMoveUp());
    m_btnMoveDown.EnableWindow(active->CanMoveDown());
    m_btnClear.EnableWindow(active->CanDelete());
    m_toolTip.UpdateTipText(active->ClearTooltip(), &m_btnClear);

    bool canMerge = active->CanMerge();
    m_checkMerge.EnableWindow(canMerge);
    if (!canMerge && m_checkMerge.GetCheck() == BST_CHECKED)
        m_checkMerge.SetCheck(BST_UNCHECKED);
}

// ── WM_CONVERT_PROGRESS (from ImgConvertDlg worker) ──────────

LRESULT CImgToPdfDlg::OnConvertProgress(WPARAM wParam, LPARAM lParam)
{
    int idx         = (int)wParam;
    auto st         = (ConvertStatus)lParam;
    bool isTerminal = (st == ConvertStatus::Success || st == ConvertStatus::Fail);

    m_dlgTab1.SetFileStatus(idx, st);

    if (isTerminal)
    {
        if (st == ConvertStatus::Success) ++m_cntSuccess;
        else                              ++m_cntFail;
        m_progress.SetPos(m_cntSuccess + m_cntFail);
        m_staticProgressTxt.SetCounts(m_cntSuccess, m_cntFail, m_cntTotal);
    }
    return 0;
}

LRESULT CImgToPdfDlg::OnConvertDone(WPARAM, LPARAM)
{
    // m_cntSuccess/m_cntFail already accumulated via OnConvertProgress
    m_staticProgressTxt.SetCounts(m_cntSuccess, m_cntFail, m_cntTotal);
    m_dlgTab1.NotifyConvertDone();
    UpdateCommonState();
    return 0;
}

// ── WM_PDF_TOOL_DONE (from PdfToolsDlg worker) ───────────────

LRESULT CImgToPdfDlg::OnPdfToolDone(WPARAM wParam, LPARAM)
{
    bool ok = (wParam != 0);
    m_dlgTab2.NotifyRunDone(ok);
    m_staticProgressTxt.SetWindowText(ok ? LS(IDS_PROGRESS_DONE) : LS(IDS_PROGRESS_ERROR));
    UpdateCommonState();
    return 0;
}

// ── WM_MD_CONVERT_DONE (from MdConvertDlg worker) ────────────

LRESULT CImgToPdfDlg::OnMdConvertDone(WPARAM wParam, LPARAM)
{
    bool ok = (wParam != 0);
    m_dlgTab3.NotifyConvertDone(ok);
    UpdateCommonState();
    return 0;
}

LRESULT CImgToPdfDlg::OnWordConvertDone(WPARAM wParam, LPARAM)
{
    bool ok = (wParam != 0);
    m_dlgTab4.NotifyConvertDone(ok);
    UpdateCommonState();
    return 0;
}

LRESULT CImgToPdfDlg::OnPptConvertDone(WPARAM wParam, LPARAM)
{
    bool ok = (wParam != 0);
    m_dlgTab5.NotifyConvertDone(ok);
    UpdateCommonState();
    return 0;
}

// ── WM_TAB_STATE_CHANGED (from any tab) ──────────────────────

LRESULT CImgToPdfDlg::OnTabStateChanged(WPARAM, LPARAM)
{
    UpdateCommonState();
    return 0;
}

// ── ◀▶ buttons ───────────────────────────────────────────────

void CImgToPdfDlg::CreateTabNavButtons()
{
    CFont* pFont = GetFont();

    m_btnTabPrev.Create(L"◀", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        CRect(0, 0, 28, 20), this, IDC_BTN_TAB_PREV);
    m_btnTabNext.Create(L"▶", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        CRect(0, 0, 28, 20), this, IDC_BTN_TAB_NEXT);

    if (pFont)
    {
        m_btnTabPrev.SetFont(pFont);
        m_btnTabNext.SetFont(pFont);
    }

    m_btnTabPrev.m_nFlatStyle = CMFCButton::BUTTONSTYLE_SEMIFLAT;
    m_btnTabNext.m_nFlatStyle = CMFCButton::BUTTONSTYLE_SEMIFLAT;

    PositionTabNavButtons();
}

void CImgToPdfDlg::PositionTabNavButtons()
{
    if (!m_tabCtrl.GetSafeHwnd()) return;
    if (!m_btnTabPrev.GetSafeHwnd()) return;

    CRect rcItem;
    if (!m_tabCtrl.GetItemRect(0, &rcItem)) return;

    CRect rcTabCtrl;
    m_tabCtrl.GetWindowRect(&rcTabCtrl);
    ScreenToClient(&rcTabCtrl);

    const int btnW   = 26;
    const int btnH   = rcItem.Height();
    const int gap    = 2;
    const int right  = rcTabCtrl.right - 4;
    const int top    = rcTabCtrl.top + rcItem.top;

    CRect rcNext(right - btnW,          top, right,               top + btnH);
    CRect rcPrev(right - btnW*2 - gap,  top, right - btnW - gap,  top + btnH);

    m_btnTabPrev.MoveWindow(rcPrev);
    m_btnTabNext.MoveWindow(rcNext);
}

void CImgToPdfDlg::UpdateTabNavButtons()
{
    if (!m_btnTabPrev.GetSafeHwnd()) return;
    m_btnTabPrev.EnableWindow(m_activeTab > 0);
    m_btnTabNext.EnableWindow(m_activeTab < m_tabCtrl.GetItemCount() - 1);
}

// ── tab layout ───────────────────────────────────────────────

void CImgToPdfDlg::RepositionTabDialogs()
{
    if (!m_tabCtrl.GetSafeHwnd()) return;

    CRect rcTab;
    m_tabCtrl.GetWindowRect(&rcTab);
    ScreenToClient(&rcTab);
    m_tabCtrl.AdjustRect(FALSE, &rcTab);

    for (int i = 0; i < 5; ++i)
        if (m_tabDlgs[i] && m_tabDlgs[i]->GetSafeHwnd())
            m_tabDlgs[i]->MoveWindow(rcTab);
}

void CImgToPdfDlg::ShowTab(int idx)
{
    for (int i = 0; i < 5; ++i)
        if (m_tabDlgs[i] && m_tabDlgs[i]->GetSafeHwnd())
            m_tabDlgs[i]->ShowWindow(i == idx ? SW_SHOW : SW_HIDE);
    m_activeTab = idx;
    m_tabCtrl.SetCurSel(idx);

    if (m_tabDlgs[idx] && m_tabDlgs[idx]->GetSafeHwnd())
        m_tabDlgs[idx]->BringWindowToTop();

    UpdateTabNavButtons();

    // notify tab and refresh common controls
    if (m_editPath.GetSafeHwnd())
        ActiveTab()->OnTabActivated(m_editPath, m_checkMerge);

    // reset progress display when switching to a non-conversion tab
    if (m_tabDlgs[idx] != &m_dlgTab1)
    {
        m_progress.SetPos(0);
        m_staticProgressTxt.SetIdle();
    }

    UpdateCommonState();
}

void CImgToPdfDlg::OnTcnSelchangeTabCtrl(NMHDR*, LRESULT* pResult)
{
    ShowTab(m_tabCtrl.GetCurSel());
    *pResult = 0;
}

void CImgToPdfDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    if (nIDCtl != IDC_TAB_CTRL) { CDialogEx::OnDrawItem(nIDCtl, lpDIS); return; }

    constexpr COLORREF clrBg     = RGB(248, 249, 252);
    constexpr COLORREF clrBorder = RGB(180, 185, 200);
    constexpr COLORREF clrText   = RGB(30,  30,  30);

    CDC dc; dc.Attach(lpDIS->hDC);
    CRect rc(lpDIS->rcItem);
    bool bSel = (lpDIS->itemState & ODS_SELECTED) != 0;

    dc.FillSolidRect(rc, clrBg);

    CPen pen(PS_SOLID, 1, clrBorder);
    CPen* pOldPen = dc.SelectObject(&pen);
    if (bSel) {
        dc.MoveTo(rc.left,      rc.bottom);
        dc.LineTo(rc.left,      rc.top);
        dc.LineTo(rc.right - 1, rc.top);
        dc.LineTo(rc.right - 1, rc.bottom);
    } else {
        dc.Draw3dRect(rc, clrBorder, clrBorder);
    }
    dc.SelectObject(pOldPen);

    TCHAR szText[64] = {};
    TCITEM ti = {}; ti.mask = TCIF_TEXT; ti.pszText = szText; ti.cchTextMax = 63;
    m_tabCtrl.GetItem(lpDIS->itemID, &ti);

    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(clrText);
    CFont* pOldFont = dc.SelectObject(m_tabCtrl.GetFont());
    dc.DrawText(szText, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (pOldFont) dc.SelectObject(pOldFont);

    dc.Detach();
}

// ── ◀▶ click → swap ──────────────────────────────────────────

void CImgToPdfDlg::OnBnClickedTabPrev()
{
    if (m_activeTab > 0) SwapTabs(m_activeTab, m_activeTab - 1);
}

void CImgToPdfDlg::OnBnClickedTabNext()
{
    if (m_activeTab < m_tabCtrl.GetItemCount() - 1)
        SwapTabs(m_activeTab, m_activeTab + 1);
}

void CImgToPdfDlg::SwapTabs(int a, int b)
{
    TCHAR bufA[128] = {}, bufB[128] = {};
    TCITEM tcA = {}; tcA.mask = TCIF_TEXT; tcA.pszText = bufA; tcA.cchTextMax = 128;
    TCITEM tcB = {}; tcB.mask = TCIF_TEXT; tcB.pszText = bufB; tcB.cchTextMax = 128;
    m_tabCtrl.GetItem(a, &tcA);
    m_tabCtrl.GetItem(b, &tcB);
    m_tabCtrl.SetItem(a, &tcB);
    m_tabCtrl.SetItem(b, &tcA);

    std::swap(m_tabDlgs[a], m_tabDlgs[b]);
    ShowTab(b);
    SaveTabOrder();
}

// ── registry: tab order ───────────────────────────────────────

void CImgToPdfDlg::SaveTabOrder()
{
    CString order;
    for (int i = 0; i < 5; ++i)
    {
        if (i > 0) order += L",";
        order.AppendFormat(L"%d", GetDlgIndex(m_tabDlgs[i]));
    }
    AfxGetApp()->WriteProfileString(L"TabOrder", L"Order", order);
}

void CImgToPdfDlg::LoadTabOrder()
{
    CString s = AfxGetApp()->GetProfileString(L"TabOrder", L"Order", L"0,1,2,3,4");

    int ids[5] = { 0, 1, 2, 3, 4 };
    if (_stscanf_s(s, L"%d,%d,%d,%d,%d", &ids[0], &ids[1], &ids[2], &ids[3], &ids[4]) != 5) return;

    bool used[5] = {};
    for (int x : ids)
        if (x >= 0 && x < 5) used[x] = true;
    if (!used[0] || !used[1] || !used[2] || !used[3] || !used[4]) return;

    CTabDlgBase* origDlgs[5] = { &m_dlgTab1, &m_dlgTab2, &m_dlgTab3, &m_dlgTab4, &m_dlgTab5 };
    static const AppStringId labelIds[5] = { IDS_TAB1_LABEL, IDS_TAB2_LABEL, IDS_TAB3_LABEL, IDS_TAB4_LABEL, IDS_TAB5_LABEL };

    for (int i = 0; i < 5; ++i)
    {
        m_tabDlgs[i] = origDlgs[ids[i]];
        CString label = LS(labelIds[ids[i]]);
        TCITEM tc = {}; tc.mask = TCIF_TEXT;
        tc.pszText = label.GetBuffer();
        m_tabCtrl.SetItem(i, &tc);
        label.ReleaseBuffer();
    }
}

int CImgToPdfDlg::GetDlgIndex(CTabDlgBase* pDlg)
{
    if (pDlg == &m_dlgTab1) return 0;
    if (pDlg == &m_dlgTab2) return 1;
    if (pDlg == &m_dlgTab3) return 2;
    if (pDlg == &m_dlgTab4) return 3;
    return 4;
}

// ── size / constraints ────────────────────────────────────────

void CImgToPdfDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    lpMMI->ptMinTrackSize.x = 800;
    lpMMI->ptMinTrackSize.y = 600;
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CImgToPdfDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED || !m_tabCtrl.GetSafeHwnd()) return;

    ResizeCommonControls(cx, cy);
    PositionTabNavButtons();
    RepositionTabDialogs();
}

void CImgToPdfDlg::ResizeCommonControls(int cx, int cy)
{
    if (m_initCx == 0) return;

    int dw = cx - m_initCx;
    int dh = cy - m_initCy;

    auto move = [](CWnd& w, CRect r)
    {
        w.MoveWindow(r.left, r.top, r.Width(), r.Height());
    };

    CRect r;
    r = m_rcEdit0;    r.right += dw;               move(m_editPath,          r);
    r = m_rcMerge0;   r.OffsetRect(dw, 0);          move(m_checkMerge,        r);
    r = m_rcBrowse0;  r.OffsetRect(dw, 0);          move(m_btnBrowse,         r);
    r = m_rcConvert0; r.OffsetRect(dw, 0);          move(m_btnConvert,        r);
    r = m_rcProg0;    r.right += dw;                move(m_progress,          r);
    r = m_rcProgTxt0; r.OffsetRect(dw, 0);          move(m_staticProgressTxt, r);
    r = m_rcMoveUp0;  r.OffsetRect(dw, 0);          move(m_btnMoveUp,         r);
    r = m_rcMoveDown0;r.OffsetRect(dw, 0);          move(m_btnMoveDown,       r);
    r = m_rcClear0;   r.OffsetRect(dw, 0);          move(m_btnClear,          r);
    r = m_rcTab0;     r.right += dw; r.bottom += dh; move(m_tabCtrl,          r);

    Invalidate();
}

// ── system menu / F1 help ─────────────────────────────────────

void CImgToPdfDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if (nID == IDM_HELP_USAGE)
        ShowHelpDialog();
    else if (nID == IDM_LANG_TOGGLE)
    {
        SetLang(g_lang == Lang::KO ? Lang::EN : Lang::KO);
        ApplyLanguage();
    }
    else
        CDialogEx::OnSysCommand(nID, lParam);
}

BOOL CImgToPdfDlg::OnHelpInfo(HELPINFO*)
{
    ShowHelpDialog();
    return TRUE;
}

void CImgToPdfDlg::UpdateLangMenuItem()
{
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (!pSysMenu) return;
    CString menuText = (g_lang == Lang::KO) ? L"Switch to English" : L"\ud55c\uad6d\uc5b4\ub85c \uc804\ud658";
    pSysMenu->ModifyMenu(IDM_LANG_TOGGLE, MF_BYCOMMAND | MF_STRING,
                         IDM_LANG_TOGGLE, menuText);
}

void CImgToPdfDlg::ApplyLanguage()
{
    // common controls
    {
        CString cue = LS(IDS_CUE_PATH);
        m_editPath.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue);
    }
    m_toolTip.UpdateTipText(LS(IDS_TIP_EDITPATH),   &m_editPath);
    m_toolTip.UpdateTipText(LS(IDS_TIP_MERGE),       &m_checkMerge);
    m_toolTip.UpdateTipText(LS(IDS_TIP_BROWSE),      &m_btnBrowse);
    m_toolTip.UpdateTipText(LS(IDS_TIP_CONVERT),     &m_btnConvert);
    m_toolTip.UpdateTipText(LS(IDS_TIP_PROGRESS),    &m_staticProgressTxt);
    m_toolTip.UpdateTipText(LS(IDS_TIP_MOVEUP),      &m_btnMoveUp);
    m_toolTip.UpdateTipText(LS(IDS_TIP_MOVEDOWN),    &m_btnMoveDown);
    m_toolTip.UpdateTipText(LS(IDS_TIP_CLEAR_BASE),  &m_btnClear);

    // tab labels (respecting current order)
    static const AppStringId labelIds[5] = { IDS_TAB1_LABEL, IDS_TAB2_LABEL, IDS_TAB3_LABEL, IDS_TAB4_LABEL, IDS_TAB5_LABEL };
    for (int i = 0; i < 5; ++i)
    {
        int dlgIdx = GetDlgIndex(m_tabDlgs[i]);
        CString label = LS(labelIds[dlgIdx]);
        TCITEM tc = {}; tc.mask = TCIF_TEXT;
        tc.pszText = label.GetBuffer();
        m_tabCtrl.SetItem(i, &tc);
        label.ReleaseBuffer();
    }

    // each tab
    for (int i = 0; i < 5; ++i)
        if (m_tabDlgs[i]) m_tabDlgs[i]->ApplyLanguage();

    // system menu lang item
    UpdateLangMenuItem();

    // refresh button states (RunLabel may have changed)
    UpdateCommonState();
}

void CImgToPdfDlg::ShowHelpDialog()
{
    CString msg = LS(IDS_HELP_TEXT);
    CString cap = LS(IDS_HELP_CAPTION);
    ::MessageBoxW(GetSafeHwnd(), msg, cap, MB_OK | MB_ICONINFORMATION);
}


// ── close / destroy ───────────────────────────────────────────

void CImgToPdfDlg::OnCancel()
{
    m_dlgTab1.Stop();
    CDialogEx::OnCancel();
}

void CImgToPdfDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
}
