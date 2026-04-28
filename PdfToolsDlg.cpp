#include "pch.h"
#include "PdfToolsDlg.h"
#include "AppLang.h"
#include "PdfConverter.h"
#include "PdfWriter.h"
#include <sstream>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// ── ParsePageRanges ──────────────────────────────────────────
// "1,3,5-7,10" -> {0,2,4,5,6,9}  (0-based, sorted, deduplicated)
std::vector<int> ParsePageRanges(const CString& s, int pageCount)
{
    std::vector<int> result;
    if (s.IsEmpty() || pageCount <= 0) return result;

    std::wstring ws(s.GetString());
    std::wstringstream ss(ws);
    std::wstring token;

    while (std::getline(ss, token, L','))
    {
        auto trim = [](std::wstring& t)
        {
            auto l = t.find_first_not_of(L" \t");
            auto r = t.find_last_not_of(L" \t");
            t = (l == std::wstring::npos) ? L"" : t.substr(l, r - l + 1);
        };
        trim(token);
        if (token.empty()) continue;

        auto dash = token.find(L'-');
        if (dash != std::wstring::npos)
        {
            try
            {
                int from = std::stoi(token.substr(0, dash));
                int to   = std::stoi(token.substr(dash + 1));
                if (from < 1 || to < from || to > pageCount) return {};
                for (int p = from; p <= to; ++p)
                    result.push_back(p - 1);
            } catch (...) { return {}; }
        }
        else
        {
            try
            {
                int p = std::stoi(token);
                if (p < 1 || p > pageCount) return {};
                result.push_back(p - 1);
            } catch (...) { return {}; }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

// ── temp file helper ─────────────────────────────────────────
static CString TempJpgPath(int idx)
{
    TCHAR tmp[MAX_PATH];
    GetTempPath(MAX_PATH, tmp);
    CString path;
    path.Format(_T("%simgtopdf_tab2_%04d.jpg"), tmp, idx);
    return path;
}

// ── message map ───────────────────────────────────────────────
BEGIN_MESSAGE_MAP(CPdfToolsDlg, CTabDlgBase)
    ON_WM_DROPFILES()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_PDF_PAGES_LOADED, &CPdfToolsDlg::OnPdfPagesLoaded)
    ON_BN_CLICKED(IDC_BTN_PDF_SUMMARIZE,    &CPdfToolsDlg::OnBnClickedSummarize)
    ON_BN_CLICKED(IDC_BTN_PDF_OPEN_NOTEPAD, &CPdfToolsDlg::OnBnClickedOpenNotepad)
    ON_BN_CLICKED(IDC_RADIO_SPLIT,       &CPdfToolsDlg::OnBnClickedRadioSplit)
    ON_BN_CLICKED(IDC_RADIO_MERGE,       &CPdfToolsDlg::OnBnClickedRadioMerge)
    ON_BN_CLICKED(IDC_RADIO_EXTRACT,     &CPdfToolsDlg::OnBnClickedRadioExtract)
    ON_BN_CLICKED(IDC_BTN_OUTPUT_BROWSE, &CPdfToolsDlg::OnBnClickedOutputBrowse)
    ON_BN_CLICKED(IDC_RADIO_SPLIT_RANGE, &CPdfToolsDlg::OnBnClickedRadioSplitRange)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PDF, &CPdfToolsDlg::OnLvnItemChangedListPdf)
    ON_NOTIFY(NM_RCLICK,      IDC_LIST_PDF, &CPdfToolsDlg::OnNMRClickListPdf)
    ON_NOTIFY(NM_DBLCLK,     IDC_LIST_PDF, &CPdfToolsDlg::OnNMDblclkListPdf)
END_MESSAGE_MAP()

CPdfToolsDlg::CPdfToolsDlg(CWnd* pParent)
    : CTabDlgBase(IDD_TAB2, pParent)
{
}

void CPdfToolsDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlgBase::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RADIO_SPLIT,          m_radSplit);
    DDX_Control(pDX, IDC_RADIO_MERGE,          m_radMerge);
    DDX_Control(pDX, IDC_RADIO_EXTRACT,        m_radExtract);
    DDX_Control(pDX, IDC_LIST_PDF,             m_listPdf);
    DDX_Control(pDX, IDC_RADIO_SPLIT_EACH,     m_radSplitEach);
    DDX_Control(pDX, IDC_RADIO_SPLIT_RANGE,    m_radSplitRange);
    DDX_Control(pDX, IDC_EDIT_SPLIT_RANGE,     m_editSplitRange);
    DDX_Control(pDX, IDC_LBL_MERGE_NAME,        m_lblMergeName);
    DDX_Control(pDX, IDC_EDIT_MERGE_OUTNAME,   m_editMergeName);
    DDX_Control(pDX, IDC_LBL_EXTRACT_PAGES,    m_lblExtractPages);
    DDX_Control(pDX, IDC_LBL_EXTRACT_HINT,     m_lblExtractHint);
    DDX_Control(pDX, IDC_EDIT_EXTRACT_PAGES,   m_editExtractPages);
    DDX_Control(pDX, IDC_RADIO_EXTRACT_SINGLE, m_radExtSingle);
    DDX_Control(pDX, IDC_RADIO_EXTRACT_EACH,   m_radExtEach);
    DDX_Control(pDX, IDC_LBL_PDF_OUTPUT,       m_lblOutputPath);
    DDX_Control(pDX, IDC_EDIT_OUTPUT,          m_editOutput);
    DDX_Control(pDX, IDC_BTN_OUTPUT_BROWSE,    m_btnOutputBrowse);
    DDX_Control(pDX, IDC_STATIC_STATUS,        m_lblStatus);
    DDX_Control(pDX, IDC_RADIO_SUMM_KO,         m_radioSummKo);
    DDX_Control(pDX, IDC_RADIO_SUMM_EN,         m_radioSummEn);
    DDX_Control(pDX, IDC_BTN_PDF_SUMMARIZE,    m_btnSummarize);
    DDX_Control(pDX, IDC_BTN_PDF_OPEN_NOTEPAD, m_btnOpenNotepad);
    DDX_Control(pDX, IDC_EDIT_PDF_SUMMARY,     m_editSummary);
}

BOOL CPdfToolsDlg::OnInitDialog()
{
    CTabDlgBase::OnInitDialog();

    DragAcceptFiles(TRUE);
    SetBackgroundColor(RGB(248, 249, 252));

    m_btnOutputBrowse.SetColors(RGB(255, 255, 255), RGB(235, 243, 255), RGB(37, 99, 235));

    m_listPdf.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listPdf.InsertColumn(0, LS(IDS_PDF_COL0), LVCFMT_LEFT,   150);
    m_listPdf.InsertColumn(1, LS(IDS_PDF_COL1), LVCFMT_CENTER,  95);
    m_listPdf.InsertColumn(2, LS(IDS_PDF_COL2), LVCFMT_LEFT,   135);
    m_listPdf.InsertColumn(3, LS(IDS_PDF_COL3), LVCFMT_LEFT,    46);

    m_radSplit.SetCheck(BST_CHECKED);
    m_radSplitEach.SetCheck(BST_CHECKED);
    m_radExtSingle.SetCheck(BST_CHECKED);
    ApplyMode(Mode::Split);

    CRect rcClient;
    GetClientRect(&rcClient);
    m_initCx = rcClient.Width();
    m_initCy = rcClient.Height();

    auto cap = [&](CWnd& w, CRect& rc) { w.GetWindowRect(&rc); ScreenToClient(&rc); };
    cap(m_listPdf,         m_rcList0);
    cap(m_lblOutputPath,   m_rcOutputLbl0);
    cap(m_editOutput,      m_rcOutput0);
    cap(m_btnOutputBrowse, m_rcBrowse0);
    cap(m_lblStatus,       m_rcStatus0);
    cap(m_radioSummKo,     m_rcSummKo0);
    cap(m_radioSummEn,     m_rcSummEn0);
    cap(m_btnSummarize,    m_rcSummarize0);
    cap(m_btnOpenNotepad,  m_rcOpenNotepad0);
    cap(m_editSummary,     m_rcSummaryEdit0);

    m_radioSummKo.SetCheck(BST_CHECKED);

    AdjustListColumns();

    m_toolTip.Create(this, TTS_ALWAYSTIP);
    m_toolTip.SetMaxTipWidth(260);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 400);
    m_toolTip.AddTool(&m_listPdf,         LS(IDS_PDF_TIP_LIST));
    m_toolTip.AddTool(&m_radSplit,        LS(IDS_PDF_TIP_SPLIT));
    m_toolTip.AddTool(&m_radMerge,        LS(IDS_PDF_TIP_MERGE_OP));
    m_toolTip.AddTool(&m_radExtract,      LS(IDS_PDF_TIP_EXTRACT));
    m_toolTip.AddTool(&m_radSplitEach,    LS(IDS_PDF_TIP_SPLIT_EACH));
    m_toolTip.AddTool(&m_radSplitRange,   LS(IDS_PDF_TIP_SPLIT_RANGE));
    m_toolTip.AddTool(&m_editSplitRange,  LS(IDS_PDF_TIP_RANGE_EDIT));
    m_toolTip.AddTool(&m_editMergeName,   LS(IDS_PDF_TIP_MERGENAME));
    m_toolTip.AddTool(&m_editExtractPages, LS(IDS_PDF_TIP_EXTRACT_PAGES));
    m_toolTip.AddTool(&m_radExtSingle,    LS(IDS_PDF_TIP_EXT_SINGLE));
    m_toolTip.AddTool(&m_radExtEach,      LS(IDS_PDF_TIP_EXT_EACH));
    m_toolTip.AddTool(&m_editOutput,      LS(IDS_PDF_TIP_OUTPUT));
    m_toolTip.AddTool(&m_btnOutputBrowse, LS(IDS_PDF_TIP_BROWSE));
    m_toolTip.AddTool(&m_radioSummKo,     LS(IDS_PDF_SUMMARIZE_TIP));
    m_toolTip.AddTool(&m_radioSummEn,     LS(IDS_PDF_SUMMARIZE_TIP));
    m_toolTip.AddTool(&m_btnSummarize,    LS(IDS_PDF_SUMMARIZE_TIP));
    m_toolTip.AddTool(&m_btnOpenNotepad,  LS(IDS_PDF_OPEN_NOTEPAD_TIP));
    m_toolTip.AddTool(&m_editSummary,     LS(IDS_PDF_SUMMARY_TIP));
    m_toolTip.Activate(TRUE);

    m_editSummary.SetWindowText(LS(IDS_PDF_SUMMARY_GUIDE));

    return TRUE;
}

BOOL CPdfToolsDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd())
        m_toolTip.RelayEvent(pMsg);
    return CTabDlgBase::PreTranslateMessage(pMsg);
}

// ── CTabDlgBase interface ─────────────────────────────────────

void CPdfToolsDlg::OnTabActivated(CEdit& editPath, CButton& checkMerge)
{
    { CString cue = LS(IDS_PDF_CUE_PATH); editPath.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue); }
    checkMerge.ShowWindow(SW_HIDE);
    m_listPdf.SetFocus();
}

void CPdfToolsDlg::OnCommonBrowse(CEdit& editPath)
{
    auto filterVec = BuildFilter(IDS_PDF_FILTER_LABEL, _T("*.pdf"));
    TCHAR szFile[32768] = {};
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = GetSafeHwnd();
    ofn.lpstrFilter = filterVec.data();
    ofn.lpstrFile   = szFile;
    ofn.nMaxFile    = _countof(szFile);
    ofn.Flags       = OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileName(&ofn)) return;

    std::vector<CString> paths;
    TCHAR* p = szFile;
    CString dir(p); p += dir.GetLength() + 1;
    if (*p == _T('\0'))
    {
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
    AddPdfFiles(paths, &editPath);
}

void CPdfToolsDlg::OnCommonRun(bool /*bMerge*/)
{
    if (m_bRunning) return;
    if (m_entries.empty()) return;

    CString outDir;
    m_editOutput.GetWindowText(outDir);
    if (outDir.IsEmpty())
    {
        outDir = m_entries[0].path.Left(m_entries[0].path.ReverseFind(_T('\\')));
        m_editOutput.SetWindowText(outDir);
    }

    m_bRunning = true;
    if (m_btnSummarize.GetSafeHwnd())   m_btnSummarize.EnableWindow(FALSE);
    if (m_btnOpenNotepad.GetSafeHwnd()) m_btnOpenNotepad.EnableWindow(FALSE);
    SetStatus(LS(IDS_PDF_STATUS_WORKING));
    NotifyHostStateChanged();

    switch (m_mode)
    {
    case Mode::Split:   RunSplit();   break;
    case Mode::Merge:   RunMerge();   break;
    case Mode::Extract: RunExtract(); break;
    }
}

void CPdfToolsDlg::OnCommonMoveUp()
{
    if (m_mode != Mode::Merge) return;
    POSITION pos = m_listPdf.GetFirstSelectedItemPosition();
    if (!pos) return;
    int sel = m_listPdf.GetNextSelectedItem(pos);
    if (sel <= 0) return;
    std::swap(m_entries[sel], m_entries[sel - 1]);
    RefreshList();
    m_listPdf.SetItemState(sel - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    NotifyHostStateChanged();
}

void CPdfToolsDlg::OnCommonMoveDown()
{
    if (m_mode != Mode::Merge) return;
    POSITION pos = m_listPdf.GetFirstSelectedItemPosition();
    if (!pos) return;
    int sel = m_listPdf.GetNextSelectedItem(pos);
    if (sel >= (int)m_entries.size() - 1) return;
    std::swap(m_entries[sel], m_entries[sel + 1]);
    RefreshList();
    m_listPdf.SetItemState(sel + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    NotifyHostStateChanged();
}

void CPdfToolsDlg::OnCommonClear(bool /*bConvDone*/)
{
    POSITION pos = m_listPdf.GetFirstSelectedItemPosition();
    if (!pos) return;
    int sel = m_listPdf.GetNextSelectedItem(pos);
    m_entries.erase(m_entries.begin() + sel);
    RefreshList();
    NotifyHostStateChanged();
}

bool CPdfToolsDlg::IsRunning()   { return m_bRunning; }
bool CPdfToolsDlg::CanRun()
{
    if (m_bRunning || m_entries.empty()) return false;
    for (const auto& e : m_entries)
        if (e.pages < 0) return false; // 페이지 수 로딩 중
    return true;
}
bool CPdfToolsDlg::CanDelete()
{
    if (m_bRunning) return false;
    POSITION pos = m_listPdf.GetFirstSelectedItemPosition();
    return (pos != nullptr);
}

bool CPdfToolsDlg::CanMoveUp()
{
    if (m_bRunning || m_mode != Mode::Merge) return false;
    POSITION pos = m_listPdf.GetFirstSelectedItemPosition();
    if (!pos) return false;
    int sel = m_listPdf.GetNextSelectedItem(pos);
    return sel > 0;
}

bool CPdfToolsDlg::CanMoveDown()
{
    if (m_bRunning || m_mode != Mode::Merge) return false;
    POSITION pos = m_listPdf.GetFirstSelectedItemPosition();
    if (!pos) return false;
    int sel = m_listPdf.GetNextSelectedItem(pos);
    return sel < (int)m_entries.size() - 1;
}

CString CPdfToolsDlg::RunLabel()
{
    return m_bRunning ? LS(IDS_PDF_BTN_RUNNING) : LS(IDS_PDF_BTN_RUN);
}

// ── called by host when WM_PDF_TOOL_DONE arrives ─────────────

void CPdfToolsDlg::NotifyRunDone(bool bSuccess)
{
    if (m_worker.joinable()) m_worker.join();
    m_bRunning = false;
    CString remark = bSuccess ? LS(IDS_PDF_REMARK_DONE) : LS(IDS_PDF_REMARK_FAIL);
    for (auto& e : m_entries) e.remark = remark;
    RefreshList();
    SetStatus(bSuccess ? LS(IDS_PDF_STATUS_DONE) : LS(IDS_PDF_STATUS_ERROR));
    NotifyHostStateChanged();
}

// ── mode ──────────────────────────────────────────────────────

void CPdfToolsDlg::ApplyMode(Mode mode)
{
    m_mode = mode;

    int showSplit = (mode == Mode::Split)   ? SW_SHOW : SW_HIDE;
    int showMerge = (mode == Mode::Merge)   ? SW_SHOW : SW_HIDE;
    int showExtr  = (mode == Mode::Extract) ? SW_SHOW : SW_HIDE;

    m_radSplitEach.ShowWindow(showSplit);
    m_radSplitRange.ShowWindow(showSplit);
    m_editSplitRange.ShowWindow(showSplit);

    m_lblMergeName.ShowWindow(showMerge);
    m_editMergeName.ShowWindow(showMerge);

    m_lblExtractPages.ShowWindow(showExtr);
    m_lblExtractHint.ShowWindow(showExtr);
    m_editExtractPages.ShowWindow(showExtr);
    m_radExtSingle.ShowWindow(showExtr);
    m_radExtEach.ShowWindow(showExtr);

    RefreshList();
    NotifyHostStateChanged();
    Invalidate();
}

void CPdfToolsDlg::OnBnClickedRadioSplit()   { ApplyMode(Mode::Split);   }
void CPdfToolsDlg::OnBnClickedRadioMerge()   { ApplyMode(Mode::Merge);   AutoFillMergeName(); }
void CPdfToolsDlg::OnBnClickedRadioExtract() { ApplyMode(Mode::Extract); }

void CPdfToolsDlg::OnBnClickedRadioSplitRange()
{
    bool bRange = (m_radSplitRange.GetCheck() == BST_CHECKED);
    m_editSplitRange.EnableWindow(bRange);
}

// ── PDF list ──────────────────────────────────────────────────

void CPdfToolsDlg::AddPdfFiles(const std::vector<CString>& paths, CEdit* pEditPath)
{
    // 중복·확장자 필터링 후 새 경로만 추출
    std::vector<CString> newPaths;
    for (const auto& path : paths)
    {
        bool dup = false;
        for (auto& e : m_entries)
            if (e.path.CompareNoCase(path) == 0) { dup = true; break; }
        if (dup) continue;

        CString ext = PathFindExtension(path);
        ext.MakeLower();
        if (ext != _T(".pdf")) continue;

        newPaths.push_back(path);
    }
    if (newPaths.empty()) return;

    // editPath에 첫 항목 디렉터리 표시
    if (pEditPath)
    {
        CString dir = newPaths[0].Left(newPaths[0].ReverseFind(_T('\\')));
        pEditPath->SetWindowText(dir);
    }

    // pages = -1(로딩 중)로 즉시 목록에 추가 → UI 반응성 유지
    for (const auto& path : newPaths)
    {
        PdfEntry entry;
        entry.path  = path;
        entry.name  = PathFindFileName(path);
        entry.pages = -1;
        m_entries.push_back(entry);
    }

    if (m_editOutput.GetSafeHwnd() && !m_entries.empty())
    {
        CString cur;
        m_editOutput.GetWindowText(cur);
        if (cur.IsEmpty())
        {
            CString dir = m_entries[0].path.Left(m_entries[0].path.ReverseFind(_T('\\')));
            m_editOutput.SetWindowText(dir);
        }
    }

    if (m_mode == Mode::Merge) AutoFillMergeName();
    RefreshList();
    NotifyHostStateChanged();

    // 페이지 수는 백그라운드 스레드에서 획득 후 WM_PDF_PAGES_LOADED로 반환
    HWND hSelf = GetSafeHwnd();
    std::thread([newPaths, hSelf]()
    {
        using Result = std::pair<CString, int>;
        auto* results = new std::vector<Result>();
        for (const auto& path : newPaths)
            results->push_back({ path, PdfConverter::GetPageCount(path) });
        if (!::PostMessage(hSelf, WM_PDF_PAGES_LOADED, 0, reinterpret_cast<LPARAM>(results)))
            delete results;
    }).detach();
}

void CPdfToolsDlg::AutoFillMergeName()
{
    if (!m_editMergeName.GetSafeHwnd() || m_entries.empty()) return;
    CString cur;
    m_editMergeName.GetWindowText(cur);
    if (!cur.IsEmpty()) return;
    CString stem = m_entries[0].name.Left(m_entries[0].name.ReverseFind(_T('.')));
    m_editMergeName.SetWindowText(stem + _T("_merge"));
}

void CPdfToolsDlg::RefreshList()
{
    // 합치기 모드 출력 파일명
    CString mergeOut;
    if (m_editMergeName.GetSafeHwnd())
    {
        m_editMergeName.GetWindowText(mergeOut);
        if (mergeOut.IsEmpty())
        {
            if (!m_entries.empty())
            {
                CString stem = m_entries[0].name.Left(m_entries[0].name.ReverseFind(_T('.')));
                mergeOut = stem + _T("_merge");
            }
            else
            {
                mergeOut = _T("merge");
            }
        }
        CString ext = PathFindExtension(mergeOut); ext.MakeLower();
        if (ext != _T(".pdf")) mergeOut += _T(".pdf");
    }

    bool bSplitRange  = m_radSplitRange.GetSafeHwnd()  && (m_radSplitRange.GetCheck()  == BST_CHECKED);
    bool bExtSingle   = m_radExtSingle.GetSafeHwnd()   && (m_radExtSingle.GetCheck()   == BST_CHECKED);

    m_listPdf.SetRedraw(FALSE);
    m_listPdf.DeleteAllItems();
    for (int i = 0; i < (int)m_entries.size(); ++i)
    {
        const auto& e = m_entries[i];

        // 파일명 (합치기 모드에서는 순서 번호 붙임)
        CString displayName;
        if (m_mode == Mode::Merge)
            displayName.Format(_T("%d. %s"), i + 1, (LPCTSTR)e.name);
        else
            displayName = e.name;
        m_listPdf.InsertItem(i, displayName);

        // 페이지수 (pages == -1 이면 백그라운드 로딩 중)
        CString pg;
        if (e.pages < 0) pg = _T("...");
        else             pg.Format(_T("%d"), e.pages);
        m_listPdf.SetItemText(i, 1, pg);

        // 변환될파일명
        CString stem = e.name.Left(e.name.ReverseFind(_T('.')));
        CString outName;
        switch (m_mode)
        {
        case Mode::Split:
            outName = bSplitRange
                ? stem + _T("_range*.pdf")
                : stem + _T("_p*.pdf");
            break;
        case Mode::Merge:
            outName = mergeOut;
            break;
        case Mode::Extract:
            outName = bExtSingle
                ? stem + _T("_extracted.pdf")
                : stem + _T("_p*.pdf");
            break;
        }
        m_listPdf.SetItemText(i, 2, outName);

        // 비고
        m_listPdf.SetItemText(i, 3, e.remark);
    }
    m_listPdf.SetRedraw(TRUE);
    AdjustListColumns();
}

void CPdfToolsDlg::AdjustListColumns()
{
    CRect rc;
    m_listPdf.GetClientRect(&rc);
    // GetClientRect는 이미 스크롤바를 제외한 너비를 반환하므로 직접 사용
    int avail = rc.Width();
    if (avail <= 100) return;

    // 페이지수·비고는 고정, 나머지를 파일명(45%)·변환될파일명(55%)으로 분배
    const int PG_W = 95, REM_W = 90;
    int flex  = avail - PG_W - REM_W;
    int col0w = flex * 45 / 100;
    int col2w = flex - col0w;

    m_listPdf.SetColumnWidth(0, col0w);
    m_listPdf.SetColumnWidth(1, PG_W);
    m_listPdf.SetColumnWidth(2, col2w);
    m_listPdf.SetColumnWidth(3, REM_W);
}

void CPdfToolsDlg::UpdateButtonState()
{
    NotifyHostStateChanged();
}

void CPdfToolsDlg::SetStatus(const CString& msg)
{
    m_lblStatus.SetWindowText(msg);
}

void CPdfToolsDlg::NotifyHostStateChanged()
{
    if (m_hHostNotify)
        ::PostMessage(m_hHostNotify, WM_TAB_STATE_CHANGED, 0, 0);
}

void CPdfToolsDlg::OnDropFiles(HDROP hDropInfo)
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
    if (m_hHostNotify)
        ::SendMessage(m_hHostNotify, WM_ROUTE_DROP, 0, (LPARAM)&paths);
}

void CPdfToolsDlg::OnLvnItemChangedListPdf(NMHDR*, LRESULT* pResult)
{
    NotifyHostStateChanged();

    int sel = m_listPdf.GetNextItem(-1, LVNI_SELECTED);
    bool hasSel = (sel >= 0 && sel < (int)m_entries.size());
    if (m_btnSummarize.GetSafeHwnd())
        m_btnSummarize.EnableWindow(hasSel && !m_bRunning);

    // 선택이 없으면 메모장 버튼도 비활성화
    if (!hasSel && m_btnOpenNotepad.GetSafeHwnd())
        m_btnOpenNotepad.EnableWindow(FALSE);

    *pResult = 0;
}

void CPdfToolsDlg::OnNMRClickListPdf(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    bool hasItem = (!m_bRunning && idx >= 0 && idx < (int)m_entries.size());

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 1, LS(IDS_PDF_MENU_REMOVE));
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 2, LS(IDS_PDF_MENU_OPEN_LOC));
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 4, LS(IDS_PDF_MENU_OPEN));
    menu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)nullptr);
    menu.AppendMenu((m_bRunning || m_entries.empty()) ? MF_STRING | MF_GRAYED : MF_STRING, 3, LS(IDS_PDF_MENU_CLEAR_ALL));

    CPoint pt;
    GetCursorPos(&pt);
    int cmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, this);

    if (cmd == 1)
    {
        m_entries.erase(m_entries.begin() + idx);
        RefreshList();
        NotifyHostStateChanged();
    }
    else if (cmd == 2)
    {
        CString dir = m_entries[idx].path.Left(m_entries[idx].path.ReverseFind(_T('\\')));
        ShellExecute(nullptr, _T("explore"), dir, nullptr, nullptr, SW_SHOWNORMAL);
    }
    else if (cmd == 4)
    {
        ShellExecute(nullptr, _T("open"), m_entries[idx].path,
                     nullptr, nullptr, SW_SHOWNORMAL);
    }
    else if (cmd == 3)
    {
        m_entries.clear();
        m_editOutput.SetWindowText(_T(""));
        RefreshList();
        NotifyHostStateChanged();
    }
    *pResult = 0;
}

void CPdfToolsDlg::OnNMDblclkListPdf(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx >= 0 && idx < (int)m_entries.size())
        ShellExecute(nullptr, _T("open"), m_entries[idx].path,
                     nullptr, nullptr, SW_SHOWNORMAL);
    *pResult = 0;
}

void CPdfToolsDlg::OnBnClickedOutputBrowse()
{
    BROWSEINFO bi = {};
    bi.hwndOwner = GetSafeHwnd();
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    CString browseTitle = LS(IDS_FOLDER_SELECT); bi.lpszTitle = browseTitle;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl) return;
    TCHAR path[MAX_PATH] = {};
    SHGetPathFromIDList(pidl, path);
    CoTaskMemFree(pidl);
    m_editOutput.SetWindowText(path);
}

// ── run workers ───────────────────────────────────────────────

void CPdfToolsDlg::RunSplit()
{
    if (m_entries.empty()) return;
    PdfEntry entry = m_entries[0];

    CString outDir;
    m_editOutput.GetWindowText(outDir);
    if (!outDir.IsEmpty() && outDir[outDir.GetLength()-1] != _T('\\'))
        outDir += _T('\\');

    bool bRange = (m_radSplitRange.GetCheck() == BST_CHECKED);
    CString ranges;
    m_editSplitRange.GetWindowText(ranges);

    HWND hNotify = m_hHostNotify;

    m_worker = std::thread([entry, outDir, bRange, ranges, hNotify]()
    {
        bool ok = true;
        CString stem = entry.name.Left(entry.name.ReverseFind(_T('.')));

        if (!bRange)
        {
            for (int i = 0; i < entry.pages && ok; ++i)
            {
                CString tmpJpg = TempJpgPath(i);
                CString err;
                if (!PdfConverter::RenderPageToJpg(entry.path, i, tmpJpg, err))
                { ok = false; break; }

                CString outPdf;
                outPdf.Format(_T("%s%s_p%03d.pdf"), (LPCTSTR)outDir, (LPCTSTR)stem, i + 1);
                if (!PdfWriter::WriteSingle(tmpJpg, outPdf, err))
                    ok = false;
                DeleteFile(tmpJpg);
            }
        }
        else
        {
            int groupIdx = 1;
            std::wstring ws(ranges.GetString());
            std::wstringstream ss(ws);
            std::wstring token;
            while (std::getline(ss, token, L',') && ok)
            {
                auto trim = [](std::wstring& t)
                {
                    auto l = t.find_first_not_of(L" \t");
                    auto r = t.find_last_not_of(L" \t");
                    t = (l == std::wstring::npos) ? L"" : t.substr(l, r - l + 1);
                };
                trim(token);
                if (token.empty()) continue;

                CString groupStr(token.c_str());
                auto pages = ParsePageRanges(groupStr, entry.pages);
                if (pages.empty()) { ok = false; break; }

                std::vector<CString> jpgPaths;
                for (int p : pages)
                {
                    CString tmpJpg = TempJpgPath(p);
                    CString err;
                    if (!PdfConverter::RenderPageToJpg(entry.path, p, tmpJpg, err))
                    { ok = false; break; }
                    jpgPaths.push_back(tmpJpg);
                }
                if (!ok) break;

                CString outPdf;
                outPdf.Format(_T("%s%s_range%d.pdf"), (LPCTSTR)outDir, (LPCTSTR)stem, groupIdx++);
                CString err;
                ok = PdfWriter::WriteMerged(jpgPaths, outPdf, err);
                for (auto& j : jpgPaths) DeleteFile(j);
            }
        }

        ::PostMessage(hNotify, WM_PDF_TOOL_DONE, ok ? 1 : 0, 0);
    });
    m_worker.detach();
}

void CPdfToolsDlg::RunMerge()
{
    CString outDir;
    m_editOutput.GetWindowText(outDir);
    if (!outDir.IsEmpty() && outDir[outDir.GetLength()-1] != _T('\\'))
        outDir += _T('\\');

    CString outName;
    m_editMergeName.GetWindowText(outName);
    if (outName.IsEmpty())
    {
        if (!m_entries.empty())
        {
            CString stem = m_entries[0].name.Left(m_entries[0].name.ReverseFind(_T('.')));
            outName = stem + _T("_merge");
        }
        else
        {
            outName = _T("merge");
        }
    }
    CString ext = PathFindExtension(outName);
    ext.MakeLower();
    if (ext != _T(".pdf")) outName += _T(".pdf");

    CString outPath = outDir + outName;
    std::vector<PdfEntry> entries = m_entries;
    HWND hNotify = m_hHostNotify;

    m_worker = std::thread([entries, outPath, hNotify]()
    {
        std::vector<CString> jpgPaths;
        bool ok = true;
        int idx = 0;

        for (const auto& entry : entries)
        {
            for (int i = 0; i < entry.pages && ok; ++i)
            {
                CString tmpJpg = TempJpgPath(idx++);
                CString err;
                if (!PdfConverter::RenderPageToJpg(entry.path, i, tmpJpg, err))
                { ok = false; break; }
                jpgPaths.push_back(tmpJpg);
            }
            if (!ok) break;
        }

        if (ok)
        {
            CString err;
            ok = PdfWriter::WriteMerged(jpgPaths, outPath, err);
        }

        for (auto& j : jpgPaths) DeleteFile(j);
        ::PostMessage(hNotify, WM_PDF_TOOL_DONE, ok ? 1 : 0, 0);
    });
    m_worker.detach();
}

void CPdfToolsDlg::RunExtract()
{
    if (m_entries.empty()) return;
    PdfEntry entry = m_entries[0];

    CString rangeStr;
    m_editExtractPages.GetWindowText(rangeStr);

    auto pages = ParsePageRanges(rangeStr, entry.pages);
    if (pages.empty())
    {
        SetStatus(LS(IDS_PDF_STATUS_RANGE_ERR));
        m_bRunning = false;
        NotifyHostStateChanged();
        return;
    }

    CString outDir;
    m_editOutput.GetWindowText(outDir);
    if (!outDir.IsEmpty() && outDir[outDir.GetLength()-1] != _T('\\'))
        outDir += _T('\\');

    bool bSingle = (m_radExtSingle.GetCheck() == BST_CHECKED);
    CString stem = entry.name.Left(entry.name.ReverseFind(_T('.')));
    HWND hNotify = m_hHostNotify;

    m_worker = std::thread([entry, pages, outDir, stem, bSingle, hNotify]()
    {
        bool ok = true;

        if (bSingle)
        {
            std::vector<CString> jpgPaths;
            for (int p : pages)
            {
                CString tmpJpg = TempJpgPath(p);
                CString err;
                if (!PdfConverter::RenderPageToJpg(entry.path, p, tmpJpg, err))
                { ok = false; break; }
                jpgPaths.push_back(tmpJpg);
            }
            if (ok)
            {
                CString outPdf = outDir + stem + _T("_extracted.pdf");
                CString err;
                ok = PdfWriter::WriteMerged(jpgPaths, outPdf, err);
            }
            for (auto& j : jpgPaths) DeleteFile(j);
        }
        else
        {
            for (int p : pages)
            {
                CString tmpJpg = TempJpgPath(p);
                CString err;
                if (!PdfConverter::RenderPageToJpg(entry.path, p, tmpJpg, err))
                { ok = false; break; }

                CString outPdf;
                outPdf.Format(_T("%s%s_p%03d.pdf"), (LPCTSTR)outDir, (LPCTSTR)stem, p + 1);
                if (!PdfWriter::WriteSingle(tmpJpg, outPdf, err))
                    ok = false;
                DeleteFile(tmpJpg);
                if (!ok) break;
            }
        }

        ::PostMessage(hNotify, WM_PDF_TOOL_DONE, ok ? 1 : 0, 0);
    });
    m_worker.detach();
}

void CPdfToolsDlg::ApplyLanguage()
{
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_LIST),          &m_listPdf);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_SPLIT),         &m_radSplit);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_MERGE_OP),      &m_radMerge);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_EXTRACT),       &m_radExtract);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_SPLIT_EACH),    &m_radSplitEach);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_SPLIT_RANGE),   &m_radSplitRange);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_RANGE_EDIT),    &m_editSplitRange);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_MERGENAME),     &m_editMergeName);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_EXTRACT_PAGES), &m_editExtractPages);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_EXT_SINGLE),    &m_radExtSingle);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_EXT_EACH),      &m_radExtEach);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_OUTPUT),        &m_editOutput);
    m_toolTip.UpdateTipText(LS(IDS_PDF_TIP_BROWSE),        &m_btnOutputBrowse);
    m_toolTip.UpdateTipText(LS(IDS_PDF_SUMMARIZE_TIP),     &m_btnSummarize);
    m_toolTip.UpdateTipText(LS(IDS_PDF_OPEN_NOTEPAD_TIP),  &m_btnOpenNotepad);
    m_toolTip.UpdateTipText(LS(IDS_PDF_SUMMARY_TIP),       &m_editSummary);
    m_btnSummarize.SetWindowText(LS(IDS_PDF_SUMMARIZE_BTN));

    // 안내 텍스트가 표시 중이면 새 언어로 교체 (요약 결과는 유지)
    {
        CString cur;
        m_editSummary.GetWindowText(cur);
        Lang other = (g_lang == Lang::KO) ? Lang::EN : Lang::KO;
        Lang saved = g_lang; g_lang = other;
        CString otherGuide = LS(IDS_PDF_SUMMARY_GUIDE);
        g_lang = saved;
        if (cur == otherGuide)
            m_editSummary.SetWindowText(LS(IDS_PDF_SUMMARY_GUIDE));
    }

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT;
    CString col0 = LS(IDS_PDF_COL0); lvc.pszText = col0.GetBuffer(); m_listPdf.SetColumn(0, &lvc); col0.ReleaseBuffer();
    CString col1 = LS(IDS_PDF_COL1); lvc.pszText = col1.GetBuffer(); m_listPdf.SetColumn(1, &lvc); col1.ReleaseBuffer();
    CString col2 = LS(IDS_PDF_COL2); lvc.pszText = col2.GetBuffer(); m_listPdf.SetColumn(2, &lvc); col2.ReleaseBuffer();
    CString col3 = LS(IDS_PDF_COL3); lvc.pszText = col3.GetBuffer(); m_listPdf.SetColumn(3, &lvc); col3.ReleaseBuffer();
}

// ── resize ────────────────────────────────────────────────────

void CPdfToolsDlg::OnSize(UINT nType, int cx, int cy)
{
    CTabDlgBase::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED || !m_listPdf.GetSafeHwnd()) return;
    ResizeControls(cx, cy);
}

LRESULT CPdfToolsDlg::OnPdfPagesLoaded(WPARAM, LPARAM lParam)
{
    using Result = std::pair<CString, int>;
    auto* results = reinterpret_cast<std::vector<Result>*>(lParam);

    bool changed = false;
    for (auto& [path, pages] : *results)
    {
        for (auto it = m_entries.begin(); it != m_entries.end(); )
        {
            if (it->path.CompareNoCase(path) == 0)
            {
                if (pages > 0)
                {
                    it->pages = pages;
                    ++it;
                }
                else
                {
                    // 페이지 수 획득 실패 → 잘못된 PDF이므로 목록에서 제거
                    it = m_entries.erase(it);
                }
                changed = true;
                break;
            }
            else ++it;
        }
    }
    delete results;

    if (changed)
    {
        RefreshList();
        NotifyHostStateChanged();
    }
    return 0;
}

void CPdfToolsDlg::OnDestroy()
{
    if (m_summaryWorker.joinable()) m_summaryWorker.join();
    CTabDlgBase::OnDestroy();
}

// ── WinRT OCR 기반 PDF 텍스트 추출 ───────────────────────────────
// 백그라운드 스레드에서 호출. RoInitialize(MTA) 완료 후 호출해야 함.
static std::wstring ExtractPdfTextOcr(const CString& pdfPath, int maxPages)
{
    using Microsoft::WRL::ComPtr;
    using Microsoft::WRL::Callback;
    using Microsoft::WRL::Wrappers::HStringReference;
    using Microsoft::WRL::Wrappers::HString;
    using namespace ABI::Windows::Foundation;
    using namespace ABI::Windows::Storage;
    using namespace ABI::Windows::Storage::Streams;
    using namespace ABI::Windows::Data::Pdf;
    using namespace ABI::Windows::Graphics::Imaging;
    using namespace ABI::Windows::Media::Ocr;

    // ── OCR 엔진 ─────────────────────────────────────────────
    ComPtr<IOcrEngineStatics> ocrStatics;
    if (FAILED(RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Media_Ocr_OcrEngine).Get(),
        IID_PPV_ARGS(&ocrStatics)))) return L"";

    ComPtr<IOcrEngine> ocrEngine;
    ocrStatics->TryCreateFromUserProfileLanguages(&ocrEngine);
    if (!ocrEngine) return L"";

    // ── PDF 문서 로딩 ─────────────────────────────────────────
    ComPtr<IStorageFileStatics> fileStatics;
    if (FAILED(RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Storage_StorageFile).Get(),
        IID_PPV_ARGS(&fileStatics)))) return L"";

    HString hPath;
    hPath.Set(pdfPath.GetString(), pdfPath.GetLength());

    ComPtr<IAsyncOperation<StorageFile*>> asyncFile;
    if (FAILED(fileStatics->GetFileFromPathAsync(hPath.Get(), &asyncFile))) return L"";

    ComPtr<IStorageFile> storageFile;
    {
        HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        asyncFile->put_Completed(Callback<IAsyncOperationCompletedHandler<StorageFile*>>(
            [hEv](IAsyncOperation<StorageFile*>*, AsyncStatus) -> HRESULT {
                SetEvent(hEv); return S_OK; }).Get());
        WaitForSingleObject(hEv, 10000); CloseHandle(hEv);
        if (FAILED(asyncFile->GetResults(&storageFile))) return L"";
    }

    ComPtr<IPdfDocumentStatics> pdfStatics;
    if (FAILED(RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Data_Pdf_PdfDocument).Get(),
        IID_PPV_ARGS(&pdfStatics)))) return L"";

    ComPtr<IAsyncOperation<PdfDocument*>> asyncDoc;
    if (FAILED(pdfStatics->LoadFromFileAsync(storageFile.Get(), &asyncDoc))) return L"";

    ComPtr<IPdfDocument> pdfDoc;
    {
        HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        asyncDoc->put_Completed(Callback<IAsyncOperationCompletedHandler<PdfDocument*>>(
            [hEv](IAsyncOperation<PdfDocument*>*, AsyncStatus) -> HRESULT {
                SetEvent(hEv); return S_OK; }).Get());
        WaitForSingleObject(hEv, 10000); CloseHandle(hEv);
        if (FAILED(asyncDoc->GetResults(&pdfDoc))) return L"";
    }

    UINT32 pageCount = 0;
    pdfDoc->get_PageCount(&pageCount);
    int pages = min((int)pageCount, maxPages);

    // ── BitmapDecoder 팩토리 ─────────────────────────────────
    ComPtr<IBitmapDecoderStatics> decoderStatics;
    if (FAILED(RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Graphics_Imaging_BitmapDecoder).Get(),
        IID_PPV_ARGS(&decoderStatics)))) return L"";

    // ── 페이지별 렌더 → 디코딩 → OCR ────────────────────────
    std::wstring allText;
    allText.reserve(16000);

    for (int i = 0; i < pages && allText.size() < 16000; ++i)
    {
        ComPtr<IPdfPage> page;
        if (FAILED(pdfDoc->GetPage((UINT32)i, &page))) continue;

        // 페이지 → InMemoryRandomAccessStream
        ComPtr<IRandomAccessStream> rasStream;
        if (FAILED(RoActivateInstance(
            HStringReference(RuntimeClass_Windows_Storage_Streams_InMemoryRandomAccessStream).Get(),
            reinterpret_cast<IInspectable**>(rasStream.GetAddressOf())))) continue;

        ComPtr<IAsyncAction> asyncRender;
        if (FAILED(page->RenderToStreamAsync(rasStream.Get(), &asyncRender))) continue;
        {
            HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            asyncRender->put_Completed(Callback<IAsyncActionCompletedHandler>(
                [hEv](IAsyncAction*, AsyncStatus) -> HRESULT {
                    SetEvent(hEv); return S_OK; }).Get());
            WaitForSingleObject(hEv, 30000); CloseHandle(hEv);
        }
        rasStream->Seek(0);

        // BitmapDecoder 생성
        ComPtr<IAsyncOperation<BitmapDecoder*>> asyncDecoder;
        if (FAILED(decoderStatics->CreateAsync(rasStream.Get(), &asyncDecoder))) continue;

        ComPtr<IBitmapDecoder> decoder;
        {
            HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            asyncDecoder->put_Completed(Callback<IAsyncOperationCompletedHandler<BitmapDecoder*>>(
                [hEv](IAsyncOperation<BitmapDecoder*>*, AsyncStatus) -> HRESULT {
                    SetEvent(hEv); return S_OK; }).Get());
            WaitForSingleObject(hEv, 10000); CloseHandle(hEv);
            if (FAILED(asyncDecoder->GetResults(&decoder))) continue;
        }

        // SoftwareBitmap 획득
        ComPtr<IBitmapFrameWithSoftwareBitmap> frame;
        if (FAILED(decoder.As(&frame))) continue;

        ComPtr<IAsyncOperation<SoftwareBitmap*>> asyncBitmap;
        if (FAILED(frame->GetSoftwareBitmapAsync(&asyncBitmap))) continue;

        ComPtr<ISoftwareBitmap> bitmap;
        {
            HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            asyncBitmap->put_Completed(Callback<IAsyncOperationCompletedHandler<SoftwareBitmap*>>(
                [hEv](IAsyncOperation<SoftwareBitmap*>*, AsyncStatus) -> HRESULT {
                    SetEvent(hEv); return S_OK; }).Get());
            WaitForSingleObject(hEv, 10000); CloseHandle(hEv);
            if (FAILED(asyncBitmap->GetResults(&bitmap))) continue;
        }

        // OCR
        ComPtr<IAsyncOperation<OcrResult*>> asyncOcr;
        if (FAILED(ocrEngine->RecognizeAsync(bitmap.Get(), &asyncOcr))) continue;

        ComPtr<IOcrResult> ocrResult;
        {
            HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            asyncOcr->put_Completed(Callback<IAsyncOperationCompletedHandler<OcrResult*>>(
                [hEv](IAsyncOperation<OcrResult*>*, AsyncStatus) -> HRESULT {
                    SetEvent(hEv); return S_OK; }).Get());
            WaitForSingleObject(hEv, 30000); CloseHandle(hEv);
            if (FAILED(asyncOcr->GetResults(&ocrResult))) continue;
        }

        HString text;
        if (FAILED(ocrResult->get_Text(text.GetAddressOf()))) continue;

        UINT32 len = 0;
        const wchar_t* raw = text.GetRawBuffer(&len);
        if (raw && len > 0)
        {
            allText.append(raw, len);
            allText += L'\n';
        }
    }

    return allText;
}

void CPdfToolsDlg::OnBnClickedSummarize()
{
    int sel = m_listPdf.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)m_entries.size()) return;

    CString pdfPath = m_entries[sel].path;
    bool bKo = (m_radioSummKo.GetCheck() == BST_CHECKED);
    HWND hEdit        = m_editSummary.GetSafeHwnd();
    HWND hBtn         = m_btnSummarize.GetSafeHwnd();
    HWND hBtnNotepad  = m_btnOpenNotepad.GetSafeHwnd();

    CString working = LS(IDS_PDF_SUMMARIZE_WORKING);
    ::SetWindowText(hEdit, working);
    ::EnableWindow(hBtn, FALSE);
    ::EnableWindow(hBtnNotepad, FALSE);

    if (m_summaryWorker.joinable()) m_summaryWorker.join();

    m_summaryWorker = std::thread([pdfPath, bKo, hEdit, hBtn, hBtnNotepad]()
    {
        auto setResult = [&](const wchar_t* msg, bool bSuccess = false)
        {
            if (!::IsWindow(hEdit)) return;
            ::SetWindowText(hEdit, msg);
            if (::IsWindow(hBtn))        ::EnableWindow(hBtn, TRUE);
            if (::IsWindow(hBtnNotepad)) ::EnableWindow(hBtnNotepad, bSuccess);
        };

        // WinRT 초기화 (RAII)
        struct RoInit {
            RoInit()  { RoInitialize(RO_INIT_MULTITHREADED); }
            ~RoInit() { RoUninitialize(); }
        } roInit;

        // 1. WinRT OCR로 PDF 텍스트 추출 (최대 5페이지)
        std::wstring wtext = ExtractPdfTextOcr(pdfPath, 5);
        if (wtext.empty() || wtext.find_first_not_of(L" \t\r\n") == std::wstring::npos)
        {
            setResult(L"텍스트를 인식할 수 없습니다.\n"
                      L"OCR 언어 팩이 설치되어 있지 않거나 PDF 이미지 품질이 낮을 수 있습니다.\n"
                      L"설정 → 시간 및 언어 → 언어에서 OCR 언어 팩을 추가해보세요.");
            return;
        }

        // wide → UTF-8
        std::string textUtf8;
        {
            int len = WideCharToMultiByte(CP_UTF8, 0,
                wtext.c_str(), (int)wtext.size(), nullptr, 0, nullptr, nullptr);
            if (len > 0)
            {
                textUtf8.resize(len);
                WideCharToMultiByte(CP_UTF8, 0,
                    wtext.c_str(), (int)wtext.size(), &textUtf8[0], len, nullptr, nullptr);
            }
        }
        if (textUtf8.empty())
        {
            setResult(L"텍스트 변환에 실패했습니다.");
            return;
        }

        // 4. JSON escape 및 Ollama 프롬프트 구성
        std::string escaped;
        escaped.reserve(textUtf8.size() + 64);
        for (unsigned char c : textUtf8)
        {
            if      (c == '"')  escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else if (c < 0x20)  {} // 제어문자 제거
            else                escaped += (char)c;
        }

        std::string promptText = bKo
            ? "다음 PDF 내용을 아래 형식으로 한국어로 상세하게 요약해줘:\\n\\n"
              "[개요]\\n(전체 내용을 4~6문장으로 상세히 설명)\\n\\n"
              "[주요 내용]\\n"
              "• 핵심 포인트를 최소 5개 이상 불릿으로 작성\\n"
              "• 각 포인트는 1~2문장으로 충분히 설명\\n\\n"
              "[결론 및 시사점]\\n(핵심 메시지와 의의를 3~4문장으로)\\n\\n"
              "위 형식을 반드시 따르고, 내용을 충분히 풍부하게 작성해줘.\\n\\nPDF 내용:\\n"
            : "Summarize the following PDF content in detail using this exact format:\\n\\n"
              "[Overview]\\n(4-6 sentences describing the full content in detail)\\n\\n"
              "[Key Points]\\n"
              "• At least 5 bullet points\\n"
              "• Each point should be 1-2 sentences with sufficient explanation\\n\\n"
              "[Conclusion]\\n(3-4 sentences on the main message and significance)\\n\\n"
              "Follow this format strictly and write with sufficient detail.\\n\\nPDF content:\\n";

        std::string body =
            "{\"model\":\"llama3.2:3b\","
            "\"prompt\":\"" + promptText + escaped + "\","
            "\"stream\":false}";

        // 5. WinHTTP로 Ollama 호출
        HINTERNET hSession = WinHttpOpen(L"ImgToPdf/1.0",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession)
        {
            setResult(L"Ollama 연결 실패 (WinHTTP 초기화 오류).");
            return;
        }

        HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", 11434, 0);
        HINTERNET hRequest = hConnect
            ? WinHttpOpenRequest(hConnect, L"POST", L"/api/generate",
                                 nullptr, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, 0)
            : nullptr;

        std::string rawResp;
        bool httpOk = false;

        if (hRequest)
        {
            LPCWSTR hdrs = L"Content-Type: application/json";
            BOOL sent = WinHttpSendRequest(hRequest, hdrs, (DWORD)-1,
                (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
            if (sent && WinHttpReceiveResponse(hRequest, nullptr))
            {
                httpOk = true;
                DWORD avail = 0;
                while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0)
                {
                    std::vector<char> chunk(avail + 1, 0);
                    DWORD rd = 0;
                    WinHttpReadData(hRequest, chunk.data(), avail, &rd);
                    rawResp.append(chunk.data(), rd);
                }
            }
        }

        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (!httpOk)
        {
            setResult(L"Ollama에 연결할 수 없습니다.\nOllama가 실행 중인지 확인하세요 (http://localhost:11434).");
            return;
        }

        // 6. JSON에서 "response" 필드 파싱
        std::string result;
        auto pos = rawResp.find("\"response\":\"");
        if (pos != std::string::npos)
        {
            pos += 12;
            for (size_t i = pos; i < rawResp.size(); ++i)
            {
                if (rawResp[i] == '\\' && i + 1 < rawResp.size())
                {
                    ++i;
                    switch (rawResp[i])
                    {
                    case 'n':  result += "\r\n"; break;
                    case 'r':  break; // \r은 무시 (\r\n으로 통일)
                    case 't':  result += '\t'; break;
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    default:   result += rawResp[i]; break;
                    }
                }
                else if (rawResp[i] == '"') break;
                else result += rawResp[i];
            }
        }

        if (result.empty())
        {
            setResult(L"Ollama 응답을 파싱할 수 없습니다. 모델(llama3.2:3b)이 설치되어 있는지 확인하세요.");
            return;
        }

        // 7. UTF-8 → UTF-16 변환 후 표시
        int wlen = MultiByteToWideChar(CP_UTF8, 0, result.c_str(), -1, nullptr, 0);
        std::wstring wresult(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, result.c_str(), -1, wresult.data(), wlen);
        setResult(wresult.c_str(), true);
    });
    m_summaryWorker.detach();
}

void CPdfToolsDlg::OnBnClickedOpenNotepad()
{
    CString text;
    m_editSummary.GetWindowText(text);
    if (text.IsEmpty()) return;

    // 임시 파일 경로 생성 (%TEMP%\ImgToPdf_summary_XXXX.txt)
    wchar_t tmpDir[MAX_PATH], tmpBase[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpDir);
    GetTempFileNameW(tmpDir, L"ImgToPdf_", 0, tmpBase); // 빈 파일 생성됨

    // .txt 확장자로 이름 변경 (실패 시 원본 삭제 후 중단)
    CString txtPath = tmpBase;
    txtPath += L".txt";
    if (!MoveFileW(tmpBase, txtPath))
    {
        DeleteFileW(tmpBase);
        return;
    }

    // UTF-8 BOM으로 저장
    std::ofstream ofs(txtPath.GetString(), std::ios::binary);
    if (!ofs)
    {
        DeleteFileW(txtPath);
        return;
    }

    ofs.write("\xEF\xBB\xBF", 3); // BOM

    // CString(UTF-16) → UTF-8 변환
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data(), utf8Len, nullptr, nullptr);
    if (utf8Len > 1) ofs.write(utf8.data(), utf8Len - 1); // null 제외
    ofs.close();

    // 메모장 실행 (파일은 OS 재시작 시 %TEMP% 정리에 위임)
    ShellExecuteW(nullptr, L"open", L"notepad.exe", txtPath, nullptr, SW_SHOWNORMAL);
}

void CPdfToolsDlg::ResizeControls(int cx, int cy)
{
    if (m_initCx == 0) return;

    int dw = cx - m_initCx;
    int dh = cy - m_initCy;

    auto move = [](CWnd& w, CRect r)
    {
        w.MoveWindow(r.left, r.top, r.Width(), r.Height());
    };

    CRect r = m_rcList0;
    r.right += dw;
    move(m_listPdf, r);
    AdjustListColumns();

    r = m_rcOutputLbl0;
    r.OffsetRect(0, dh);
    move(m_lblOutputPath, r);

    r = m_rcOutput0;
    r.OffsetRect(0, dh);
    r.right += dw;
    move(m_editOutput, r);

    r = m_rcBrowse0;
    r.OffsetRect(dw, dh);
    move(m_btnOutputBrowse, r);

    r = m_rcStatus0;
    r.OffsetRect(0, dh);
    r.right += dw;
    move(m_lblStatus, r);

    r = m_rcSummKo0;
    r.OffsetRect(0, dh);
    move(m_radioSummKo, r);

    r = m_rcSummEn0;
    r.OffsetRect(0, dh);
    move(m_radioSummEn, r);

    r = m_rcSummarize0;
    r.OffsetRect(dw, dh);
    move(m_btnSummarize, r);

    r = m_rcOpenNotepad0;
    r.OffsetRect(dw, dh);
    move(m_btnOpenNotepad, r);

    r = m_rcSummaryEdit0;
    r.OffsetRect(0, dh);
    r.right += dw;
    r.bottom += dh;
    move(m_editSummary, r);

    Invalidate();
}
