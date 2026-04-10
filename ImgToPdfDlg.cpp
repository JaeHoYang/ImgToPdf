#include "pch.h"
#include "ImgToPdfDlg.h"
#include "PdfWriter.h"
#include "PdfConverter.h"

// 지원 확장자 목록
static const TCHAR* kSupportedExts[] = {
    _T(".jpg"), _T(".jpeg"), _T(".png"),
    _T(".bmp"), _T(".tiff"), _T(".tif"), _T(".gif")
};

BEGIN_MESSAGE_MAP(CImgToPdfDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_HELPINFO()
    ON_WM_DROPFILES()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_BROWSE,    &CImgToPdfDlg::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BTN_CONVERT,   &CImgToPdfDlg::OnBnClickedConvert)
    ON_BN_CLICKED(IDC_CHECK_MERGE,   &CImgToPdfDlg::OnBnClickedCheckMerge)
    ON_BN_CLICKED(IDC_BTN_MOVE_UP,   &CImgToPdfDlg::OnBnClickedMoveUp)
    ON_BN_CLICKED(IDC_BTN_MOVE_DOWN, &CImgToPdfDlg::OnBnClickedMoveDown)
    ON_BN_CLICKED(IDC_BTN_CLEAR,     &CImgToPdfDlg::OnBnClickedClear)
    ON_NOTIFY(NM_CLICK, IDC_LIST_FILES, &CImgToPdfDlg::OnNMClickListFiles)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, &CImgToPdfDlg::OnLvnItemChangedListFiles)
    ON_MESSAGE(WM_CONVERT_PROGRESS,      &CImgToPdfDlg::OnConvertProgress)
    ON_MESSAGE(WM_CONVERT_DONE,          &CImgToPdfDlg::OnConvertDone)
    ON_MESSAGE(WM_LIST_ENTRIES_CHANGED,  &CImgToPdfDlg::OnListEntriesChanged)
END_MESSAGE_MAP()

CImgToPdfDlg::CImgToPdfDlg(CWnd* pParent)
    : CDialogEx(IDD_IMGTOPDF_DIALOG, pParent)
{
}

void CImgToPdfDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LBL_PATH,             m_lblPath);
    DDX_Control(pDX, IDC_EDIT_PATH,            m_editPath);
    DDX_Control(pDX, IDC_CHECK_MERGE,          m_checkMerge);
    DDX_Control(pDX, IDC_BTN_BROWSE,           m_btnBrowse);
    DDX_Control(pDX, IDC_BTN_CONVERT,          m_btnConvert);
    DDX_Control(pDX, IDC_BTN_MOVE_UP,          m_btnMoveUp);
    DDX_Control(pDX, IDC_BTN_MOVE_DOWN,        m_btnMoveDown);
    DDX_Control(pDX, IDC_BTN_CLEAR,            m_btnClear);
    DDX_Control(pDX, IDC_PROGRESS,             m_progress);
    DDX_Control(pDX, IDC_STATIC_PROGRESS_TXT,  m_staticProgressTxt);
    DDX_Control(pDX, IDC_LIST_FILES,           m_listFiles);
    DDX_Control(pDX, IDC_STATIC_PREVIEW,       m_preview);
}

BOOL CImgToPdfDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 아이콘 설정
    HICON hIcon = AfxGetApp()->LoadIcon(IDI_IMGTOPDF);
    SetIcon(hIcon, TRUE);   // 큰 아이콘 (Alt+Tab, 작업 표시줄)
    SetIcon(hIcon, FALSE);  // 작은 아이콘 (타이틀바)

    // 드래그앤드롭 허용
    DragAcceptFiles(TRUE);

    // 리스트뷰 컬럼 설정
    m_listFiles.SetupColumns();

    // 프로그레스바 초기화
    m_progress.SetRange32(0, 100);
    m_progress.SetPos(0);
    m_staticProgressTxt.SetIdle();

    // ── 시스템 메뉴에 "사용 방법" 항목 추가 ──────────────────
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu)
    {
        pSysMenu->AppendMenu(MF_SEPARATOR);
        pSysMenu->AppendMenu(MF_STRING, IDM_HELP_USAGE, _T("사용 방법(&H)..."));
    }

    // ── 에디트박스 플레이스홀더 ──────────────────────────────────
    m_editPath.SendMessage(EM_SETCUEBANNER, FALSE,
        (LPARAM)L"F1을 누르면 사용 방법이 나옵니다.");

    // ── 툴팁 설정 ────────────────────────────────────────────
    m_toolTip.Create(this, TTS_ALWAYSTIP);
    m_toolTip.SetMaxTipWidth(260);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 400);

    struct TipDef { CWnd* w; LPCTSTR tip; };
    TipDef tips[] = {
        { &m_editPath,          _T("변환할 파일의 경로가 표시됩니다.") },
        { &m_checkMerge,        _T("체크: 모든 이미지 → 하나의 다중 페이지 PDF\n미체크: 각 이미지 → 개별 PDF 파일\n(PDF 항목 포함 시 자동 비활성)") },
        { &m_btnBrowse,         _T("이미지 또는 PDF 파일을 선택합니다.\n다중 선택 지원 | 창에 드래그앤드롭도 가능") },
        { &m_btnConvert,        _T("변환 시작\n변환 중 클릭 시 현재 파일 완료 후 중단") },
        { &m_staticProgressTxt, _T("(완료 / 실패 / 총계)\n완료: 초록  실패: 빨강  총계: 기본색") },
        { &m_btnMoveUp,         _T("선택한 이미지 항목을 위로 이동\n(PDF 페이지 항목은 이동 불가)") },
        { &m_btnMoveDown,       _T("선택한 이미지 항목을 아래로 이동\n(PDF 페이지 항목은 이동 불가)") },
        { &m_btnClear,          _T("항목을 선택하면 제거할 수 있습니다.\n변환 완료 후에는 전체 초기화합니다.") },
        { &m_listFiles,         _T("파일 목록\n• 드래그앤드롭 또는 [찾기]로 추가\n• 클릭: 미리보기 표시\n• Delete 키: 선택 항목 제거\n• 우클릭: 제거 / 파일 위치 열기") },
        { &m_preview,           _T("목록에서 이미지 항목을 클릭하면 미리보기가 표시됩니다.") },
    };
    for (auto& t : tips)
        m_toolTip.AddTool(t.w, t.tip);
    m_toolTip.Activate(TRUE);

    // 초기 클라이언트 크기 및 컨트롤 위치 캡처
    CRect rcClient;
    GetClientRect(&rcClient);
    m_initCx = rcClient.Width();
    m_initCy = rcClient.Height();

    auto capture = [&](CWnd& w, CRect& rc) {
        w.GetWindowRect(&rc); ScreenToClient(&rc);
    };
    capture(m_editPath,        m_rcEdit0);
    capture(m_checkMerge,      m_rcMerge0);
    capture(m_btnBrowse,       m_rcBrowse0);
    capture(m_btnConvert,      m_rcConvert0);
    capture(m_progress,          m_rcProg0);
    capture(m_staticProgressTxt, m_rcProgTxt0);
    capture(m_btnMoveUp,         m_rcMoveUp0);
    capture(m_btnMoveDown,       m_rcMoveDown0);
    capture(m_btnClear,          m_rcClear0);
    capture(m_listFiles,       m_rcList0);
    capture(m_preview,         m_rcPreview0);

    // 초기 포커스를 리스트뷰로 이동 — 에디트박스 포커스 시 플레이스홀더가 숨겨지는 것 방지
    m_listFiles.SetFocus();
    return FALSE;  // FALSE = 포커스를 직접 설정했음을 MFC에 알림
}

// ── PreTranslateMessage — 툴팁 릴레이 ──────────────────────

BOOL CImgToPdfDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd())
        m_toolTip.RelayEvent(pMsg);
    return CDialogEx::PreTranslateMessage(pMsg);
}

// ── 시스템 메뉴 / F1 → 사용 방법 다이얼로그 ─────────────────

void CImgToPdfDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if (nID == IDM_HELP_USAGE)
        ShowHelpDialog();
    else
        CDialogEx::OnSysCommand(nID, lParam);
}

BOOL CImgToPdfDlg::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    ShowHelpDialog();
    return TRUE;
}

void CImgToPdfDlg::ShowHelpDialog()
{
    const wchar_t* msg =
        L"ImgToPdf — 이미지 \u2194 PDF 양방향 변환\n"
        L"\n"
        L"이미지를 PDF로 변환하거나 PDF를 페이지별 JPG로 추출합니다.\n"
        L"파일은 드래그앤드롭 또는 [찾기] 버튼으로 추가하세요.\n"
        L"\n"
        L"[ 이미지 \u2192 PDF ]\n"
        L"1. JPG/PNG/BMP/TIFF/GIF 파일을 드래그하거나 [찾기]로 추가\n"
        L"2. (선택) 합치기 체크 \u2192 모든 이미지를 하나의 다중 페이지 PDF로 합침\n"
        L"3. (선택) \u25b2\u25bc 버튼으로 변환 순서 조정\n"
        L"4. [변환] 클릭 \u2192 원본 파일과 같은 폴더에 PDF 저장\n"
        L"\n"
        L"[ PDF \u2192 JPG ]\n"
        L"1. PDF 파일을 드래그하거나 [찾기]로 추가\n"
        L"2. 페이지 수가 자동 스캔되어 페이지별로 목록에 표시됨\n"
        L"3. [변환] 클릭 \u2192 원본명_p001.jpg, _p002.jpg ... 형태로 저장\n"
        L"\n"
        L"[ 공통 ]\n"
        L"- Delete 키 또는 우클릭으로 선택 항목 제거\n"
        L"- 변환 중 [중단] \u2192 현재 파일 완료 후 중단\n"
        L"- 변환 완료 후 [삭제] \u2192 목록 전체 초기화\n"
        L"- 합치기는 이미지 항목만 지원 (PDF 항목 포함 시 자동 비활성)\n"
        L"- 각 버튼/컨트롤에 마우스를 올리면 툴팁 설명이 표시됩니다\n"
        L"\n"
        L"(F1 또는 타이틀바 우클릭 \u2192 사용 방법으로 이 창을 열 수 있습니다.)\n"
        L"\n"
        L"────────────────────────────────────\n"
        L"제작자  jaeho\n"
        L"문의    jaeho9697@gmail.com";

    ::MessageBoxW(GetSafeHwnd(), msg, L"사용 방법", MB_OK | MB_ICONINFORMATION);
}

void CImgToPdfDlg::OnCancel()
{
    if (m_bConverting)
    {
        m_bStopRequested = true;
        m_worker.Stop();
    }
    CDialogEx::OnCancel();
}

void CImgToPdfDlg::OnDestroy()
{
    m_bStopRequested = true;
    m_worker.Stop();
    CDialogEx::OnDestroy();
}

// ── 창 크기 조절 ──────────────────────────────────────────────

void CImgToPdfDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    lpMMI->ptMinTrackSize.x = 800;
    lpMMI->ptMinTrackSize.y = 600;
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CImgToPdfDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED) return;
    if (!m_listFiles.GetSafeHwnd()) return;
    ResizeControls(cx, cy);
}

void CImgToPdfDlg::ResizeControls(int cx, int cy)
{
    if (m_initCx == 0) return;  // 초기값 미캡처 시 무시

    int dw = cx - m_initCx;  // 가로 증가분
    int dh = cy - m_initCy;  // 세로 증가분

    // 오른쪽 고정(우측 앵커): 원래 right + dw
    // 가로 늘어나는 컨트롤: 원래 width + dw
    // 세로 늘어나는 컨트롤: 원래 height + dh (리스트/미리보기)

    auto move = [&](CWnd& w, CRect r) {
        w.MoveWindow(r.left, r.top, r.Width(), r.Height());
    };

    // 에디트박스: 오른쪽으로 늘어남
    CRect r = m_rcEdit0;
    r.right += dw;
    move(m_editPath, r);

    // 합치기 체크박스: 우측 고정
    r = m_rcMerge0;
    r.OffsetRect(dw, 0);
    move(m_checkMerge, r);

    // 찾기 버튼: 우측 고정
    r = m_rcBrowse0;
    r.OffsetRect(dw, 0);
    move(m_btnBrowse, r);

    // 변환 버튼: 우측 고정
    r = m_rcConvert0;
    r.OffsetRect(dw, 0);
    move(m_btnConvert, r);

    // 프로그레스바: 오른쪽으로 늘어남
    r = m_rcProg0;
    r.right += dw;
    move(m_progress, r);

    // 상태 텍스트: 우측 고정
    r = m_rcProgTxt0;
    r.OffsetRect(dw, 0);
    move(m_staticProgressTxt, r);

    // 이동/삭제 버튼: 우측 고정
    r = m_rcMoveUp0;
    r.OffsetRect(dw, 0);
    move(m_btnMoveUp, r);

    r = m_rcMoveDown0;
    r.OffsetRect(dw, 0);
    move(m_btnMoveDown, r);

    r = m_rcClear0;
    r.OffsetRect(dw, 0);
    move(m_btnClear, r);

    // 리스트뷰: 가로+세로 늘어남 (세로는 절반씩 배분)
    r = m_rcList0;
    r.right  += dw;
    r.bottom += dh / 2;
    move(m_listFiles, r);

    // 미리보기: 리스트 아래 붙어서 가로+세로 늘어남
    r = m_rcPreview0;
    r.left   = m_rcList0.left;
    r.top    = m_rcList0.bottom + (dh / 2) + 4;
    r.right  = m_rcList0.right + dw;
    r.bottom = m_initCy + dh - 4;
    move(m_preview, r);

    Invalidate();
}

// ── 드래그앤드롭 ─────────────────────────────────────────────

void CImgToPdfDlg::OnDropFiles(HDROP hDropInfo)
{
    UINT count = DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0);
    std::vector<CString> filePaths;
    for (UINT i = 0; i < count; ++i)
    {
        TCHAR buf[MAX_PATH] = {};
        DragQueryFile(hDropInfo, i, buf, MAX_PATH);
        CString path(buf);
        DWORD attr = GetFileAttributes(path);
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
            AddFolder(path);
        else
            filePaths.push_back(path);
    }
    DragFinish(hDropInfo);
    AddFiles(filePaths);
}

// ── 찾기 버튼 ────────────────────────────────────────────────

void CImgToPdfDlg::OnBnClickedBrowse()
{
    // 파일 다중 선택 다이얼로그
    TCHAR filter[] =
        _T("이미지/PDF 파일\0*.jpg;*.jpeg;*.png;*.bmp;*.tiff;*.tif;*.gif;*.pdf\0")
        _T("이미지 파일\0*.jpg;*.jpeg;*.png;*.bmp;*.tiff;*.tif;*.gif\0")
        _T("PDF 파일\0*.pdf\0")
        _T("모든 파일\0*.*\0\0");

    TCHAR szFile[32768] = {};
    OPENFILENAME ofn   = {};
    ofn.lStructSize    = sizeof(ofn);
    ofn.hwndOwner      = GetSafeHwnd();
    ofn.lpstrFilter    = filter;
    ofn.lpstrFile      = szFile;
    ofn.nMaxFile       = _countof(szFile);
    ofn.Flags          = OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if (!GetOpenFileName(&ofn)) return;

    // 다중 선택 파싱 (디렉토리 + NULL + 파일들 + NULL NULL)
    std::vector<CString> paths;
    TCHAR* p = szFile;
    CString dir(p);
    p += dir.GetLength() + 1;

    if (*p == _T('\0'))
    {
        // 단일 파일 선택
        paths.push_back(dir);
    }
    else
    {
        while (*p != _T('\0'))
        {
            CString name(p);
            paths.push_back(dir + _T("\\") + name);
            p += name.GetLength() + 1;
        }
    }

    if (!paths.empty())
    {
        CString firstDir = paths[0].Left(paths[0].ReverseFind(_T('\\')));
        m_editPath.SetWindowText(firstDir);
    }
    AddFiles(paths);
}

// ── 변환 버튼 ────────────────────────────────────────────────

void CImgToPdfDlg::OnBnClickedConvert()
{
    if (m_bConverting)
    {
        m_bStopRequested = true;
        return;
    }

    const auto& entries = m_listFiles.GetEntries();
    if (entries.empty()) return;

    SetConvertingState(true);

    bool bMerge = (m_checkMerge.GetCheck() == BST_CHECKED);
    int  total  = (int)entries.size();
    m_cntSuccess = 0;
    m_cntFail    = 0;
    m_cntTotal   = total;
    m_progress.SetRange32(0, total);
    m_progress.SetPos(0);
    m_staticProgressTxt.SetCounts(0, 0, total);

    // 모든 항목 상태를 Wait로 초기화
    m_listFiles.SetRowColorsEnabled(true);
    for (int i = 0; i < total; ++i)
        m_listFiles.SetStatus(i, FileEntry::Status::Wait);

    // ConvertTask 구성
    ConvertTask task;
    task.bMerge  = bMerge;
    task.hNotify = GetSafeHwnd();
    task.pStop   = &m_bStopRequested;
    for (auto& e : const_cast<std::vector<FileEntry>&>(entries))
        task.entries.push_back(&e);

    m_worker.Start(std::move(task));
}

void CImgToPdfDlg::SetConvertingState(bool bConverting)
{
    m_bConverting = bConverting;
    m_btnConvert.SetWindowText(bConverting ? _T("중단") : _T("변환"));
    m_btnBrowse.EnableWindow(!bConverting);
    // 변환 중 드래그앤드롭 차단: entries 벡터 재할당으로 인한 댕글링 포인터 방지
    DragAcceptFiles(!bConverting);
    // 합치기 체크박스: PdfPage 항목 유무에 따라 활성화 결정
    if (!bConverting) UpdateMergeCheckState();
    else              m_checkMerge.EnableWindow(FALSE);
    // 이동/삭제 버튼
    UpdateMoveButtonState();
    UpdateClearButtonState();
}

// ── 상하 이동 버튼 ───────────────────────────────────────────

void CImgToPdfDlg::OnBnClickedMoveUp()
{
    int idx = m_listFiles.GetNextItem(-1, LVNI_SELECTED);
    if (m_listFiles.MoveUp(idx))
    {
        if (m_checkMerge.GetCheck() == BST_CHECKED)
            m_listFiles.SetMergeMode(true);
        UpdateMoveButtonState();
    }
}

void CImgToPdfDlg::OnBnClickedMoveDown()
{
    int idx = m_listFiles.GetNextItem(-1, LVNI_SELECTED);
    if (m_listFiles.MoveDown(idx))
    {
        if (m_checkMerge.GetCheck() == BST_CHECKED)
            m_listFiles.SetMergeMode(true);
        UpdateMoveButtonState();
    }
}

void CImgToPdfDlg::UpdateMoveButtonState()
{
    if (m_bConverting)
    {
        m_btnMoveUp.EnableWindow(FALSE);
        m_btnMoveDown.EnableWindow(FALSE);
        return;
    }

    int idx = m_listFiles.GetNextItem(-1, LVNI_SELECTED);
    const auto& entries = m_listFiles.GetEntries();
    int n = (int)entries.size();

    if (idx < 0 || idx >= n ||
        entries[idx].type == FileEntry::Type::PdfPage)
    {
        m_btnMoveUp.EnableWindow(FALSE);
        m_btnMoveDown.EnableWindow(FALSE);
        return;
    }

    m_btnMoveUp.EnableWindow(idx > 0);
    m_btnMoveDown.EnableWindow(idx < n - 1);
}

void CImgToPdfDlg::UpdateClearButtonState()
{
    if (m_bConverting)
    {
        m_btnClear.EnableWindow(FALSE);
        return;
    }

    bool hasSelection = (m_listFiles.GetNextItem(-1, LVNI_SELECTED) >= 0);

    if (m_bConversionDone)
    {
        m_btnClear.EnableWindow(TRUE);
        m_toolTip.UpdateTipText(
            _T("목록·진행 상태를 전체 초기화합니다."), &m_btnClear);
    }
    else if (hasSelection)
    {
        m_btnClear.EnableWindow(TRUE);
        m_toolTip.UpdateTipText(
            _T("선택한 항목을 목록에서 제거합니다."), &m_btnClear);
    }
    else
    {
        m_btnClear.EnableWindow(FALSE);
        m_toolTip.UpdateTipText(
            _T("항목을 선택하면 제거할 수 있습니다.\n변환 완료 후에는 전체 초기화합니다."),
            &m_btnClear);
    }
}

void CImgToPdfDlg::OnBnClickedClear()
{
    if (m_bConversionDone)
    {
        // 변환 완료 후 → 전체 초기화 (확인)
        if (::MessageBoxW(GetSafeHwnd(),
                L"목록과 진행 상태를 모두 초기화합니다.\n계속하시겠습니까?",
                L"초기화 확인",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
            return;

        m_listFiles.Clear();
        m_editPath.SetWindowText(_T(""));
        m_progress.SetPos(0);
        m_staticProgressTxt.SetIdle();
        m_cntSuccess      = 0;
        m_cntFail         = 0;
        m_cntTotal        = 0;
        m_bConversionDone = false;
        UpdateMergeCheckState();
        UpdateMoveButtonState();
        UpdateClearButtonState();
    }
    else
    {
        // 일반 상태 → 선택 항목만 삭제
        // RemoveSelected 내부에서 WM_LIST_ENTRIES_CHANGED를 PostMessage하므로
        // UpdateMergeCheckState / UpdateClearButtonState는 그쪽에서 처리됨
        m_listFiles.RemoveSelected();
    }
}

void CImgToPdfDlg::OnLvnItemChangedListFiles(NMHDR* pNMHDR, LRESULT* pResult)
{
    UpdateMoveButtonState();
    UpdateClearButtonState();
    *pResult = 0;
}

// ── 합치기 체크박스 ──────────────────────────────────────────

void CImgToPdfDlg::OnBnClickedCheckMerge()
{
    bool bMerge = (m_checkMerge.GetCheck() == BST_CHECKED);
    m_listFiles.SetMergeMode(bMerge);
}

// ── 리스트 클릭 → 미리보기 ──────────────────────────────────

void CImgToPdfDlg::OnNMClickListFiles(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    const auto& entries = m_listFiles.GetEntries();
    if (idx >= 0 && idx < (int)entries.size())
        m_preview.LoadImage(entries[idx].srcPath);
    *pResult = 0;
}

// ── 변환 진행 메시지 ─────────────────────────────────────────

LRESULT CImgToPdfDlg::OnConvertProgress(WPARAM wParam, LPARAM lParam)
{
    int idx         = (int)wParam;
    auto st         = (ConvertStatus)lParam;
    bool isTerminal = (st == ConvertStatus::Success || st == ConvertStatus::Fail);

    switch (st)
    {
    case ConvertStatus::Success: m_listFiles.SetStatus(idx, FileEntry::Status::Success); break;
    case ConvertStatus::Fail:    m_listFiles.SetStatus(idx, FileEntry::Status::Fail);    break;
    case ConvertStatus::Running: m_listFiles.SetStatus(idx, FileEntry::Status::Running); break;
    }

    if (isTerminal)
    {
        if (st == ConvertStatus::Success) ++m_cntSuccess;
        else                              ++m_cntFail;
        m_progress.SetPos(m_cntSuccess + m_cntFail);
        m_staticProgressTxt.SetCounts(m_cntSuccess, m_cntFail, m_cntTotal);
    }
    return 0;
}

LRESULT CImgToPdfDlg::OnConvertDone(WPARAM wParam, LPARAM lParam)
{
    SetConvertingState(false);
    m_listFiles.SetRowColorsEnabled(false);
    m_staticProgressTxt.SetCounts(m_cntSuccess, m_cntFail, m_cntTotal);
    m_bStopRequested  = false;
    m_bConversionDone = true;
    UpdateClearButtonState();
    return 0;
}

LRESULT CImgToPdfDlg::OnListEntriesChanged(WPARAM, LPARAM)
{
    // RemoveSelected 완료 후 m_entries가 확정된 시점에 호출됨
    UpdateMergeCheckState();
    UpdateMoveButtonState();
    UpdateClearButtonState();
    return 0;
}

// ── 헬퍼 ─────────────────────────────────────────────────────

bool CImgToPdfDlg::IsSupportedExt(const CString& ext) const
{
    CString lower(ext);
    lower.MakeLower();
    for (auto* e : kSupportedExts)
        if (lower == e) return true;
    return false;
}

void CImgToPdfDlg::AddFiles(const std::vector<CString>& paths)
{
    for (const auto& path : paths)
    {
        CString ext = PathFindExtension(path);
        CString lower(ext);
        lower.MakeLower();

        // PDF 파일: 페이지별 행 추가
        if (lower == _T(".pdf"))
        {
            int pageCount = PdfConverter::GetPageCount(path);
            if (pageCount <= 0) continue;

            CString baseName = PathFindFileName(path);
            int dotPos = baseName.ReverseFind(_T('.'));
            CString stem = (dotPos >= 0) ? baseName.Left(dotPos) : baseName;

            for (int i = 0; i < pageCount; ++i)
            {
                if (m_listFiles.HasEntry(path, i)) continue;

                FileEntry entry;
                entry.srcPath   = path;
                entry.type      = FileEntry::Type::PdfPage;
                entry.pageIndex = i;
                entry.pageTotal = pageCount;

                CString pg;
                pg.Format(_T(" (p.%d/%d)"), i + 1, pageCount);
                entry.srcName = baseName + pg;

                CString pageNum;
                pageNum.Format(_T("%03d"), i + 1);
                entry.pdfName = stem + _T("_p") + pageNum + _T(".jpg");

                m_listFiles.AddEntry(entry);
            }
            continue;
        }

        // 이미지 파일
        if (!IsSupportedExt(ext)) continue;
        if (m_listFiles.HasEntry(path)) continue;

        FileEntry entry;
        entry.srcPath = path;
        entry.srcName = PathFindFileName(path);
        CString srcExt = PathFindExtension(path);
        entry.pdfName = entry.srcName.Left(
            entry.srcName.GetLength() - srcExt.GetLength())
            + _T(".pdf");
        m_listFiles.AddEntry(entry);
    }

    UpdateMergeCheckState();
}

void CImgToPdfDlg::UpdateMergeCheckState()
{
    const auto& entries = m_listFiles.GetEntries();
    bool hasPdfPage = std::any_of(entries.begin(), entries.end(),
        [](const FileEntry& e){ return e.type == FileEntry::Type::PdfPage; });

    m_checkMerge.EnableWindow(!hasPdfPage && !m_bConverting);
    if (hasPdfPage)
    {
        m_checkMerge.SetCheck(BST_UNCHECKED);
        m_listFiles.SetMergeMode(false);
    }
    else
    {
        m_listFiles.SetMergeMode(m_checkMerge.GetCheck() == BST_CHECKED);
    }
}

void CImgToPdfDlg::AddFolder(const CString& folderPath)
{
    CString pattern = folderPath + _T("\\*");
    WIN32_FIND_DATA fd = {};
    HANDLE hFind = FindFirstFile(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    // RAII: 어떤 경로로 빠져나가도 FindClose 보장
    struct FindGuard {
        HANDLE h;
        ~FindGuard() { FindClose(h); }
    } guard{ hFind };

    std::vector<CString> found;
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        CString fullPath = folderPath + _T("\\") + fd.cFileName;
        CString ext      = PathFindExtension(fd.cFileName);
        if (IsSupportedExt(ext))
            found.push_back(fullPath);
    }
    while (FindNextFile(hFind, &fd));

    AddFiles(found);
    if (!found.empty()) m_editPath.SetWindowText(folderPath);
}

