#include "pch.h"
#include "ImgConvertDlg.h"
#include "PdfWriter.h"
#include "AppLang.h"
// #include "PdfConverter.h"  // PDF 기능 비활성화

static const TCHAR* kSupportedExts[] = {
    _T(".jpg"), _T(".jpeg"), _T(".png"),
    _T(".bmp"), _T(".tiff"), _T(".tif"), _T(".gif")
};

BEGIN_MESSAGE_MAP(CImgConvertDlg, CTabDlgBase)
    ON_WM_HSCROLL()
    ON_WM_DROPFILES()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_IMG_OUTPUT_BROWSE, &CImgConvertDlg::OnBnClickedOutputBrowse)
    ON_NOTIFY(NM_CLICK,        IDC_LIST_FILES, &CImgConvertDlg::OnNMClickListFiles)
    ON_NOTIFY(NM_DBLCLK,      IDC_LIST_FILES, &CImgConvertDlg::OnNMDblclkListFiles)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, &CImgConvertDlg::OnLvnItemChangedListFiles)
    ON_MESSAGE(WM_LIST_ENTRIES_CHANGED, &CImgConvertDlg::OnListEntriesChanged)
END_MESSAGE_MAP()

CImgConvertDlg::CImgConvertDlg(CWnd* pParent)
    : CTabDlgBase(IDD_TAB1, pParent)
{
}

void CImgConvertDlg::Stop()
{
    m_bStopRequested = true;
    m_worker.Stop();
}

void CImgConvertDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlgBase::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_FILES,             m_listFiles);
    DDX_Control(pDX, IDC_STATIC_PREVIEW,         m_preview);
    DDX_Control(pDX, IDC_SLIDER_QUALITY,         m_sliderQuality);
    DDX_Control(pDX, IDC_LBL_QUALITY,            m_lblQuality);
    DDX_Control(pDX, IDC_LBL_IMG_OUTPUT,         m_lblOutputPath);
    DDX_Control(pDX, IDC_EDIT_IMG_OUTPUT,        m_editOutput);
    DDX_Control(pDX, IDC_BTN_IMG_OUTPUT_BROWSE,  m_btnOutputBrowse);
}

BOOL CImgConvertDlg::OnInitDialog()
{
    CTabDlgBase::OnInitDialog();

    DragAcceptFiles(TRUE);
    SetBackgroundColor(RGB(248, 249, 252));

    m_btnOutputBrowse.SetColors(RGB(255, 255, 255), RGB(235, 243, 255), RGB(37, 99, 235));

    m_listFiles.SetupColumns();

    m_toolTip.Create(this, TTS_ALWAYSTIP);
    m_toolTip.SetMaxTipWidth(260);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 400);
    m_toolTip.AddTool(&m_listFiles,  LS(IDS_IMG_TIP_LIST));
    m_toolTip.AddTool(&m_preview,    LS(IDS_IMG_TIP_PREVIEW));
    m_toolTip.AddTool(&m_editOutput, LS(IDS_IMG_TIP_OUTPUT));
    m_toolTip.Activate(TRUE);

    {
        CString cue = LS(IDS_IMG_CUE_OUTPUT);
        m_editOutput.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue);
    }

    CRect rcClient;
    GetClientRect(&rcClient);
    m_initCx = rcClient.Width();
    m_initCy = rcClient.Height();

    m_sliderQuality.SetRange(10, 100);
    m_sliderQuality.SetPos(90);
    m_sliderQuality.SetPageSize(10);
    m_toolTip.AddTool(&m_sliderQuality, LS(IDS_IMG_TIP_QUALITY));

    auto capture = [&](CWnd& w, CRect& rc) { w.GetWindowRect(&rc); ScreenToClient(&rc); };
    capture(m_listFiles,       m_rcList0);
    capture(m_preview,         m_rcPreview0);
    capture(m_sliderQuality,   m_rcSlider0);
    capture(m_lblQuality,      m_rcQualLbl0);
    capture(m_lblOutputPath,   m_rcOutputLbl0);
    capture(m_editOutput,      m_rcOutput0);
    capture(m_btnOutputBrowse, m_rcOutputBrowse0);

    m_listFiles.SetFocus();
    return FALSE;
}

BOOL CImgConvertDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd())
        m_toolTip.RelayEvent(pMsg);
    return CTabDlgBase::PreTranslateMessage(pMsg);
}

// ── CTabDlgBase interface ─────────────────────────────────────

void CImgConvertDlg::OnTabActivated(CEdit& editPath, CButton& checkMerge)
{
    {
        CString cue = LS(IDS_CUE_PATH);
        editPath.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue);
    }
    checkMerge.ShowWindow(SW_SHOW);
    m_listFiles.SetFocus();
}

void CImgConvertDlg::OnCommonBrowse(CEdit& editPath)
{
    auto filterVec = BuildFilter(IDS_IMG_FILTER_LABEL,
        _T("*.jpg;*.jpeg;*.png;*.bmp;*.tiff;*.tif;*.gif"));

    TCHAR szFile[32768] = {};
    OPENFILENAME ofn   = {};
    ofn.lStructSize    = sizeof(ofn);
    ofn.hwndOwner      = GetSafeHwnd();
    ofn.lpstrFilter    = filterVec.data();
    ofn.lpstrFile      = szFile;
    ofn.nMaxFile       = _countof(szFile);
    ofn.Flags          = OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if (!GetOpenFileName(&ofn)) return;

    std::vector<CString> paths;
    TCHAR* p = szFile;
    CString dir(p);
    p += dir.GetLength() + 1;

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

    if (!paths.empty())
    {
        CString firstDir = paths[0].Left(paths[0].ReverseFind(_T('\\')));
        editPath.SetWindowText(firstDir);
    }
    AddFiles(paths);
}

void CImgConvertDlg::OnCommonRun(bool bMerge)
{
    if (m_bConverting)
    {
        m_bStopRequested = true;
        return;
    }

    const auto& entries = m_listFiles.GetEntries();
    if (entries.empty()) return;

    m_bConverting     = true;
    m_bConversionDone = false;
    m_bStopRequested  = false;
    m_listFiles.SetRowColorsEnabled(true);

    int total = (int)entries.size();
    for (int i = 0; i < total; ++i)
        m_listFiles.SetStatus(i, FileEntry::Status::Wait);

    CString outDir;
    m_editOutput.GetWindowText(outDir);
    outDir.Trim();

    if (!outDir.IsEmpty())
    {
        DWORD attr = GetFileAttributes(outDir);
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            ::MessageBoxW(GetSafeHwnd(),
                LS(IDS_IMG_ERR_OUTDIR),
                LS(IDS_IMG_ERR_TITLE), MB_OK | MB_ICONWARNING);
            return;
        }
    }

    ConvertTask task;
    task.bMerge      = bMerge;
    task.hNotify     = m_hHostNotify;
    task.pStop       = &m_bStopRequested;
    task.outDir      = outDir;
    task.jpegQuality = m_sliderQuality.GetPos();
    for (auto& e : const_cast<std::vector<FileEntry>&>(entries))
        task.entries.push_back(&e);

    m_worker.Start(std::move(task));

    DragAcceptFiles(false);
    NotifyHostStateChanged();
}

void CImgConvertDlg::OnCommonMoveUp()
{
    int idx = m_listFiles.GetNextItem(-1, LVNI_SELECTED);
    if (m_listFiles.MoveUp(idx))
    {
        if (m_listFiles.GetMergeMode())
            m_listFiles.SetMergeMode(true);
        NotifyHostStateChanged();
    }
}

void CImgConvertDlg::OnCommonMoveDown()
{
    int idx = m_listFiles.GetNextItem(-1, LVNI_SELECTED);
    if (m_listFiles.MoveDown(idx))
    {
        if (m_listFiles.GetMergeMode())
            m_listFiles.SetMergeMode(true);
        NotifyHostStateChanged();
    }
}

void CImgConvertDlg::OnCommonClear(bool bConvDone)
{
    if (bConvDone)
    {
        if (::MessageBoxW(GetSafeHwnd(),
                LS(IDS_IMG_CONFIRM_RESET),
                LS(IDS_IMG_CONFIRM_TITLE),
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
            return;

        m_listFiles.Clear();
        m_bConversionDone = false;
        m_editOutput.SetWindowText(_T(""));
        UpdateMergeCheckState();
        NotifyHostStateChanged();
    }
    else
    {
        m_listFiles.RemoveSelected();
    }
}

void CImgConvertDlg::OnCommonMergeChanged(bool bMerge)
{
    m_listFiles.SetMergeMode(bMerge);
}

bool CImgConvertDlg::IsRunning()
{
    return m_bConverting;
}

bool CImgConvertDlg::IsDone()
{
    return m_bConversionDone;
}

bool CImgConvertDlg::CanMoveUp()
{
    if (m_bConverting) return false;
    int idx = m_listFiles.GetNextItem(-1, LVNI_SELECTED);
    const auto& entries = m_listFiles.GetEntries();
    if (idx <= 0 || idx >= (int)entries.size()) return false;
    // if (entries[idx].type == FileEntry::Type::PdfPage) return false;  // PDF 기능 비활성화
    return true;
}

bool CImgConvertDlg::CanMoveDown()
{
    if (m_bConverting) return false;
    int idx = m_listFiles.GetNextItem(-1, LVNI_SELECTED);
    const auto& entries = m_listFiles.GetEntries();
    int n = (int)entries.size();
    if (idx < 0 || idx >= n - 1) return false;
    // if (entries[idx].type == FileEntry::Type::PdfPage) return false;  // PDF 기능 비활성화
    return true;
}

bool CImgConvertDlg::CanDelete()
{
    if (m_bConverting) return false;
    if (m_bConversionDone) return true;
    return (m_listFiles.GetNextItem(-1, LVNI_SELECTED) >= 0);
}

bool CImgConvertDlg::CanRun()
{
    return !m_bConverting && !m_listFiles.GetEntries().empty();
}

bool CImgConvertDlg::CanMerge()
{
    if (m_bConverting) return false;
    // PDF 기능 비활성화: PdfPage 항목 체크 제거
    // const auto& entries = m_listFiles.GetEntries();
    // return !std::any_of(entries.begin(), entries.end(),
    //     [](const FileEntry& e){ return e.type == FileEntry::Type::PdfPage; });
    return !m_listFiles.GetEntries().empty();
}

CString CImgConvertDlg::RunLabel()
{
    return m_bConverting ? LS(IDS_IMG_BTN_STOP) : LS(IDS_IMG_BTN_CONVERT);
}

CString CImgConvertDlg::ClearTooltip()
{
    if (m_bConversionDone)
        return LS(IDS_IMG_CLEAR_DONE);
    if (m_listFiles.GetNextItem(-1, LVNI_SELECTED) >= 0)
        return LS(IDS_IMG_CLEAR_SEL);
    return LS(IDS_IMG_CLEAR_NONE);
}

void CImgConvertDlg::ApplyLanguage()
{
    m_toolTip.UpdateTipText(LS(IDS_IMG_TIP_LIST),    &m_listFiles);
    m_toolTip.UpdateTipText(LS(IDS_IMG_TIP_PREVIEW), &m_preview);
    m_toolTip.UpdateTipText(LS(IDS_IMG_TIP_OUTPUT),  &m_editOutput);
    {
        CString cue = LS(IDS_IMG_CUE_OUTPUT);
        m_editOutput.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue);
    }
    m_listFiles.ApplyLanguage();
}

// ── called by host ────────────────────────────────────────────

void CImgConvertDlg::SetFileStatus(int idx, ConvertStatus st)
{
    switch (st)
    {
    case ConvertStatus::Success: m_listFiles.SetStatus(idx, FileEntry::Status::Success); break;
    case ConvertStatus::Fail:    m_listFiles.SetStatus(idx, FileEntry::Status::Fail);    break;
    case ConvertStatus::Running: m_listFiles.SetStatus(idx, FileEntry::Status::Running); break;
    }
}

void CImgConvertDlg::NotifyConvertDone()
{
    m_bConverting     = false;
    m_bConversionDone = true;
    m_bStopRequested  = false;
    m_listFiles.SetRowColorsEnabled(false);
    DragAcceptFiles(TRUE);
    UpdateMergeCheckState();
}

// ── 창 크기 조절 ──────────────────────────────────────────────

void CImgConvertDlg::OnSize(UINT nType, int cx, int cy)
{
    CTabDlgBase::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED) return;
    if (!m_listFiles.GetSafeHwnd()) return;
    ResizeControls(cx, cy);
}

void CImgConvertDlg::OnHScroll(UINT, UINT, CScrollBar* pBar)
{
    if (pBar && pBar->GetSafeHwnd() == m_sliderQuality.GetSafeHwnd())
    {
        CString lbl;
        lbl.Format(LS(IDS_IMG_QUALITY_FMT), m_sliderQuality.GetPos());
        m_lblQuality.SetWindowText(lbl);
    }
}

void CImgConvertDlg::ResizeControls(int cx, int cy)
{
    if (m_initCx == 0) return;

    int dw = cx - m_initCx;
    int dh = cy - m_initCy;

    auto move = [&](CWnd& w, CRect r)
    {
        w.MoveWindow(r.left, r.top, r.Width(), r.Height());
    };

    CRect r;

    r = m_rcList0; r.right += dw;
    move(m_listFiles, r);

    r = m_rcSlider0; r.right += dw;
    move(m_sliderQuality, r);

    r = m_rcQualLbl0; r.OffsetRect(dw, 0);
    move(m_lblQuality, r);

    r = m_rcOutputLbl0;
    move(m_lblOutputPath, r);
    r = m_rcOutput0; r.right += dw;
    move(m_editOutput, r);
    r = m_rcOutputBrowse0; r.OffsetRect(dw, 0);
    move(m_btnOutputBrowse, r);

    // 미리보기: 너비와 높이 모두 확장
    r = m_rcPreview0; r.right += dw; r.bottom += dh;
    move(m_preview, r);

    Invalidate();
}

// ── 종료 ─────────────────────────────────────────────────────

void CImgConvertDlg::OnDestroy()
{
    m_bStopRequested = true;
    m_worker.Stop();
    // PDF 기능 비활성화: PDF 페이지 임시 미리보기 파일 삭제 불필요
    // if (!m_sTempPreviewJpg.IsEmpty())
    //     DeleteFile(m_sTempPreviewJpg);
    CTabDlgBase::OnDestroy();
}

// ── 출력 폴더 선택 ────────────────────────────────────────────

void CImgConvertDlg::OnBnClickedOutputBrowse()
{
    TCHAR szPath[MAX_PATH] = {};
    BROWSEINFO bi          = {};
    bi.hwndOwner           = GetSafeHwnd();
    bi.pszDisplayName      = szPath;
    CString folderTitle = LS(IDS_FOLDER_SELECT);
    bi.lpszTitle           = folderTitle;
    bi.ulFlags             = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pIdl = SHBrowseForFolder(&bi);
    if (!pIdl) return;
    SHGetPathFromIDList(pIdl, szPath);
    CoTaskMemFree(pIdl);
    m_editOutput.SetWindowText(szPath);
}

// ── 드래그앤드롭 ─────────────────────────────────────────────

void CImgConvertDlg::OnDropFiles(HDROP hDropInfo)
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

// ── 목록 이벤트 ──────────────────────────────────────────────

void CImgConvertDlg::OnLvnItemChangedListFiles(NMHDR*, LRESULT* pResult)
{
    NotifyHostStateChanged();
    *pResult = 0;
}

void CImgConvertDlg::OnNMClickListFiles(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    const auto& entries = m_listFiles.GetEntries();
    if (idx >= 0 && idx < (int)entries.size())
    {
        const FileEntry& e = entries[idx];
        // PDF 기능 비활성화: PdfPage 미리보기 분기 제거
        // if (e.type == FileEntry::Type::PdfPage)
        // {
        //     if (m_sTempPreviewJpg.IsEmpty())
        //     {
        //         TCHAR tmp[MAX_PATH];
        //         GetTempPath(MAX_PATH, tmp);
        //         m_sTempPreviewJpg = CString(tmp) + _T("imgtopdf_preview.jpg");
        //     }
        //     CString err;
        //     if (PdfConverter::RenderPageToJpg(e.srcPath, e.pageIndex, m_sTempPreviewJpg, err))
        //         m_preview.LoadImage(m_sTempPreviewJpg);
        //     else
        //         m_preview.Clear();
        // }
        // else
        {
            m_preview.LoadImage(e.srcPath);
        }
    }
    *pResult = 0;
}

void CImgConvertDlg::OnNMDblclkListFiles(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    const auto& entries = m_listFiles.GetEntries();
    if (idx >= 0 && idx < (int)entries.size())
        ShellExecute(nullptr, _T("open"), entries[idx].srcPath,
                     nullptr, nullptr, SW_SHOWNORMAL);
    *pResult = 0;
}

LRESULT CImgConvertDlg::OnListEntriesChanged(WPARAM, LPARAM)
{
    if (m_listFiles.GetEntries().empty())
        m_editOutput.SetWindowText(_T(""));
    UpdateMergeCheckState();
    NotifyHostStateChanged();
    return 0;
}

// ── 헬퍼 ─────────────────────────────────────────────────────

void CImgConvertDlg::NotifyHostStateChanged()
{
    if (m_hHostNotify)
        ::PostMessage(m_hHostNotify, WM_TAB_STATE_CHANGED, 0, 0);
}

void CImgConvertDlg::UpdateMergeCheckState()
{
    // PDF 기능 비활성화: PdfPage 존재 시 합치기 해제 로직 불필요
    // const auto& entries = m_listFiles.GetEntries();
    // bool hasPdfPage = std::any_of(entries.begin(), entries.end(),
    //     [](const FileEntry& e){ return e.type == FileEntry::Type::PdfPage; });
    // if (hasPdfPage)
    //     m_listFiles.SetMergeMode(false);
}

bool CImgConvertDlg::IsSupportedExt(const CString& ext) const
{
    CString lower(ext);
    lower.MakeLower();
    for (auto* e : kSupportedExts)
        if (lower == e) return true;
    return false;
}

void CImgConvertDlg::AddFiles(const std::vector<CString>& paths)
{
    for (const auto& path : paths)
    {
        CString ext = PathFindExtension(path);
        CString lower(ext);
        lower.MakeLower();

        // PDF 기능 비활성화: PDF 파일 페이지 분해 추가 로직 제거
        // if (lower == _T(".pdf"))
        // {
        //     int pageCount = PdfConverter::GetPageCount(path);
        //     if (pageCount <= 0) continue;
        //     CString baseName = PathFindFileName(path);
        //     int dotPos = baseName.ReverseFind(_T('.'));
        //     CString stem = (dotPos >= 0) ? baseName.Left(dotPos) : baseName;
        //     for (int i = 0; i < pageCount; ++i)
        //     {
        //         if (m_listFiles.HasEntry(path, i)) continue;
        //         FileEntry entry;
        //         entry.srcPath   = path;
        //         entry.type      = FileEntry::Type::PdfPage;
        //         entry.pageIndex = i;
        //         entry.pageTotal = pageCount;
        //         CString pg;
        //         pg.Format(_T(" (p.%d/%d)"), i + 1, pageCount);
        //         entry.srcName = baseName + pg;
        //         CString pageNum;
        //         pageNum.Format(_T("%03d"), i + 1);
        //         entry.pdfName = stem + _T("_p") + pageNum + _T(".jpg");
        //         m_listFiles.AddEntry(entry);
        //     }
        //     continue;
        // }

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

    // 출력경로가 비어있으면 첫 유효 파일의 부모 폴더로 자동 설정
    if (m_editOutput.GetSafeHwnd())
    {
        CString cur;
        m_editOutput.GetWindowText(cur);
        if (cur.IsEmpty())
        {
            for (const auto& p : paths)
            {
                if (IsSupportedExt(PathFindExtension(p)))
                {
                    m_editOutput.SetWindowText(p.Left(p.ReverseFind(_T('\\'))));
                    break;
                }
            }
        }
    }

    UpdateMergeCheckState();
    NotifyHostStateChanged();
}

void CImgConvertDlg::AddFolder(const CString& folderPath)
{
    CString pattern = folderPath + _T("\\*");
    WIN32_FIND_DATA fd = {};
    HANDLE hFind = FindFirstFile(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    struct FindGuard
    {
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
}
