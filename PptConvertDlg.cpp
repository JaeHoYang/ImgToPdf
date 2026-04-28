#include "pch.h"
#include "PptConvertDlg.h"
#include "AppLang.h"
#include <comdef.h>

// ── 엔진 감지 ─────────────────────────────────────────────────

static CString FindPowerPointExe()
{
    const wchar_t* keys[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\POWERPNT.EXE",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\App Paths\\POWERPNT.EXE",
    };
    HKEY roots[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    for (HKEY root : roots)
    {
        for (const wchar_t* key : keys)
        {
            HKEY hk;
            if (RegOpenKeyEx(root, key, 0, KEY_READ, &hk) == ERROR_SUCCESS)
            {
                TCHAR buf[MAX_PATH] = {};
                DWORD sz = sizeof(buf);
                if (RegQueryValueEx(hk, nullptr, nullptr, nullptr, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
                {
                    RegCloseKey(hk);
                    if (GetFileAttributes(buf) != INVALID_FILE_ATTRIBUTES)
                        return buf;
                }
                RegCloseKey(hk);
            }
        }
    }
    return _T("");
}

static CString FindLibreOfficeExe()
{
    const wchar_t* candidates[] = {
        L"C:\\Program Files\\LibreOffice\\program\\soffice.exe",
        L"C:\\Program Files (x86)\\LibreOffice\\program\\soffice.exe",
    };
    for (const wchar_t* p : candidates)
        if (GetFileAttributes(p) != INVALID_FILE_ATTRIBUTES)
            return p;

    const wchar_t* regPaths[] = {
        L"SOFTWARE\\LibreOffice\\UNO\\InstallPath",
        L"SOFTWARE\\WOW6432Node\\LibreOffice\\UNO\\InstallPath",
    };
    for (const wchar_t* key : regPaths)
    {
        HKEY hk;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hk) == ERROR_SUCCESS)
        {
            TCHAR buf[MAX_PATH] = {};
            DWORD sz = sizeof(buf);
            RegQueryValueEx(hk, nullptr, nullptr, nullptr, (LPBYTE)buf, &sz);
            RegCloseKey(hk);
            CString path = buf;
            path.TrimRight(_T("\\"));
            path += _T("\\soffice.exe");
            if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
                return path;
        }
    }
    return _T("");
}

// ── IDispatch 헬퍼 ────────────────────────────────────────────

static HRESULT DispGetProp(IDispatch* p, LPCWSTR name, VARIANT& out)
{
    BSTR b = SysAllocString(name);
    DISPID id;
    HRESULT hr = p->GetIDsOfNames(IID_NULL, &b, 1, LOCALE_SYSTEM_DEFAULT, &id);
    SysFreeString(b);
    if (FAILED(hr)) return hr;
    DISPPARAMS dp = {};
    VariantInit(&out);
    return p->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                     DISPATCH_PROPERTYGET, &dp, &out, nullptr, nullptr);
}

static HRESULT DispCall(IDispatch* p, LPCWSTR name,
                        VARIANT* args, int nArgs, VARIANT* pResult = nullptr)
{
    BSTR b = SysAllocString(name);
    DISPID id;
    HRESULT hr = p->GetIDsOfNames(IID_NULL, &b, 1, LOCALE_SYSTEM_DEFAULT, &id);
    SysFreeString(b);
    if (FAILED(hr)) return hr;

    std::vector<VARIANT> rev(args, args + nArgs);
    std::reverse(rev.begin(), rev.end());

    DISPPARAMS dp = {};
    dp.rgvarg = rev.empty() ? nullptr : rev.data();
    dp.cArgs  = (UINT)rev.size();

    if (pResult) VariantInit(pResult);
    return p->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                     DISPATCH_METHOD, &dp, pResult, nullptr, nullptr);
}

// ── 출력 경로 계산 ────────────────────────────────────────────

static CString BuildOutputPath(const CString& input, const CString& outDir)
{
    TCHAR stem[MAX_PATH];
    _tcscpy_s(stem, PathFindFileName(input));
    PathRemoveExtension(stem);

    CString dir = outDir;
    if (dir.IsEmpty())
    {
        TCHAR tmp[MAX_PATH];
        _tcscpy_s(tmp, input);
        PathRemoveFileSpec(tmp);
        dir = tmp;
    }
    dir.TrimRight(_T("\\"));
    return dir + _T("\\") + stem + _T(".pdf");
}

// ── message map ───────────────────────────────────────────────

BEGIN_MESSAGE_MAP(CPptConvertDlg, CTabDlgBase)
    ON_WM_DROPFILES()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_PPT_BROWSE,   &CPptConvertDlg::OnBnClickedOutputBrowse)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PPT, &CPptConvertDlg::OnLvnItemChangedListPpt)
    ON_NOTIFY(NM_CLICK,        IDC_LIST_PPT, &CPptConvertDlg::OnNMClickListPpt)
    ON_NOTIFY(NM_RCLICK,       IDC_LIST_PPT, &CPptConvertDlg::OnNMRClickListPpt)
    ON_NOTIFY(NM_DBLCLK,      IDC_LIST_PPT, &CPptConvertDlg::OnNMDblclkListPpt)
    ON_MESSAGE(WM_PPT_ITEM_DONE, &CPptConvertDlg::OnPptItemDone)
END_MESSAGE_MAP()

CPptConvertDlg::CPptConvertDlg(CWnd* pParent)
    : CTabDlgBase(IDD_TAB5, pParent)
{
}

// ── DoDataExchange ────────────────────────────────────────────

void CPptConvertDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlgBase::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PPT,           m_listPpt);
    DDX_Control(pDX, IDC_STATIC_PPT_PREVIEW, m_preview);
    DDX_Control(pDX, IDC_LBL_PPT_OUTPUT,    m_lblOutputPath);
    DDX_Control(pDX, IDC_EDIT_PPT_OUTPUT,   m_editOutput);
    DDX_Control(pDX, IDC_BTN_PPT_BROWSE,    m_btnOutputBrowse);
    DDX_Control(pDX, IDC_STATIC_PPT_STATUS, m_lblStatus);
}

// ── OnInitDialog ──────────────────────────────────────────────

BOOL CPptConvertDlg::OnInitDialog()
{
    CTabDlgBase::OnInitDialog();

    DragAcceptFiles(TRUE);
    SetBackgroundColor(RGB(248, 249, 252));

    m_btnOutputBrowse.SetColors(RGB(255, 255, 255), RGB(235, 243, 255), RGB(37, 99, 235));

    m_listPpt.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listPpt.InsertColumn(0, LS(IDS_PPT_COL0), LVCFMT_LEFT,   220);
    m_listPpt.InsertColumn(1, LS(IDS_PPT_COL1), LVCFMT_CENTER,  90);
    m_listPpt.InsertColumn(2, LS(IDS_PPT_COL2), LVCFMT_CENTER,  70);

    CRect rcClient;
    GetClientRect(&rcClient);
    m_initCx = rcClient.Width();
    m_initCy = rcClient.Height();

    auto cap = [&](CWnd& w, CRect& rc) { w.GetWindowRect(&rc); ScreenToClient(&rc); };
    cap(m_listPpt,        m_rcList0);
    cap(m_preview,        m_rcPreview0);
    cap(m_lblOutputPath,  m_rcOutputLbl0);
    cap(m_editOutput,     m_rcOutput0);
    cap(m_btnOutputBrowse,m_rcBrowse0);
    cap(m_lblStatus,      m_rcStatus0);

    AdjustListColumns();

    m_toolTip.Create(this, TTS_ALWAYSTIP);
    m_toolTip.SetMaxTipWidth(260);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 400);
    m_toolTip.AddTool(&m_listPpt,         LS(IDS_PPT_TIP_LIST));
    m_toolTip.AddTool(&m_preview,         LS(IDS_PPT_TIP_PREVIEW));
    m_toolTip.AddTool(&m_editOutput,      LS(IDS_PPT_TIP_OUTPUT));
    m_toolTip.AddTool(&m_btnOutputBrowse, LS(IDS_PPT_TIP_BROWSE));
    m_toolTip.Activate(TRUE);

    return TRUE;
}

BOOL CPptConvertDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd()) m_toolTip.RelayEvent(pMsg);
    return CTabDlgBase::PreTranslateMessage(pMsg);
}

// ── CTabDlgBase interface ─────────────────────────────────────

void CPptConvertDlg::OnTabActivated(CEdit& editPath, CButton& checkMerge)
{
    CString cue = LS(IDS_PPT_CUE_PATH);
    editPath.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue);
    checkMerge.ShowWindow(SW_HIDE);
}

void CPptConvertDlg::OnCommonBrowse(CEdit& editPath)
{
    BROWSEINFO bi = {};
    bi.hwndOwner = GetSafeHwnd();
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    CString title = LS(IDS_FOLDER_SELECT);
    bi.lpszTitle = title;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
    if (!pidl) return;

    TCHAR buf[MAX_PATH] = {};
    SHGetPathFromIDList(pidl, buf);
    CoTaskMemFree(pidl);

    editPath.SetWindowText(buf);
    m_editOutput.SetWindowText(buf);
}

void CPptConvertDlg::OnCommonRun(bool)
{
    if (m_bRunning) return;
    if (m_entries.empty()) return;

    CString outDir;
    m_editOutput.GetWindowText(outDir);
    outDir.Trim();

    if (!outDir.IsEmpty())
    {
        DWORD attr = GetFileAttributes(outDir);
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            ::MessageBoxW(GetSafeHwnd(),
                          LS(IDS_PPT_ERR_OUTDIR),
                          LS(IDS_PPT_ERR_TITLE),
                          MB_OK | MB_ICONWARNING);
            return;
        }
    }

    CString pptExe  = FindPowerPointExe();
    CString libreExe = FindLibreOfficeExe();

    if (pptExe.IsEmpty() && libreExe.IsEmpty())
    {
        ::MessageBoxW(GetSafeHwnd(),
                      LS(IDS_PPT_ERR_NO_ENGINE),
                      LS(IDS_PPT_ERR_TITLE),
                      MB_OK | MB_ICONWARNING);
        return;
    }

    m_bRunning = true;
    m_bDone    = false;
    SetStatus(LS(IDS_PPT_STATUS_WORKING));
    NotifyHostStateChanged();

    HWND hWnd    = GetSafeHwnd();
    HWND hNotify = m_hHostNotify;

    struct Item { CString path; CString outDir; };
    std::vector<Item> items;
    items.reserve(m_entries.size());
    for (auto& e : m_entries)
        items.push_back({ e.path, outDir });

    m_worker = std::thread([items, pptExe, libreExe, hWnd, hNotify]()
    {
        const int N = (int)items.size();
        // lParam: 0=실패, 1=PowerPoint성공, 2=LibreOffice성공
        std::vector<bool> done(N, false);

        // ── Phase 1: PowerPoint COM (인스턴스 1개 재사용) ──────
        if (!pptExe.IsEmpty())
        {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            CLSID clsid = {};
            IDispatch* pApp = nullptr;

            if (SUCCEEDED(CLSIDFromProgID(L"PowerPoint.Application", &clsid)) &&
                SUCCEEDED(CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER,
                                           IID_IDispatch, (void**)&pApp)))
            {
                for (int i = 0; i < N; ++i)
                {
                    CString out = BuildOutputPath(items[i].path, items[i].outDir);

                    // Presentations 컬렉션 취득
                    VARIANT varPres; VariantInit(&varPres);
                    if (FAILED(DispGetProp(pApp, L"Presentations", varPres)) ||
                        varPres.vt != VT_DISPATCH)
                        continue;
                    IDispatch* pPres = varPres.pdispVal;

                    // Presentations.Open(FileName, ReadOnly, Untitled, WithWindow)
                    VARIANT argOpen[4];
                    VariantInit(&argOpen[0]); VariantInit(&argOpen[1]);
                    VariantInit(&argOpen[2]); VariantInit(&argOpen[3]);
                    argOpen[0].vt      = VT_BSTR;
                    argOpen[0].bstrVal = SysAllocString(items[i].path);
                    argOpen[1].vt      = VT_BOOL;
                    argOpen[1].boolVal = VARIANT_TRUE;  // ReadOnly
                    argOpen[2].vt      = VT_BOOL;
                    argOpen[2].boolVal = VARIANT_FALSE; // Untitled
                    argOpen[3].vt      = VT_BOOL;
                    argOpen[3].boolVal = VARIANT_FALSE; // WithWindow

                    VARIANT varDoc; VariantInit(&varDoc);
                    HRESULT hr = DispCall(pPres, L"Open", argOpen, 4, &varDoc);
                    SysFreeString(argOpen[0].bstrVal);
                    pPres->Release();

                    if (FAILED(hr) || varDoc.vt != VT_DISPATCH)
                        continue;
                    IDispatch* pDoc = varDoc.pdispVal;

                    // Presentation.SaveAs(FileName, FileFormat=32=ppSaveAsPDF)
                    VARIANT argSave[2];
                    VariantInit(&argSave[0]); VariantInit(&argSave[1]);
                    argSave[0].vt      = VT_BSTR;
                    argSave[0].bstrVal = SysAllocString(out);
                    argSave[1].vt      = VT_INT;
                    argSave[1].intVal  = 32; // ppSaveAsPDF
                    hr = DispCall(pDoc, L"SaveAs", argSave, 2);
                    SysFreeString(argSave[0].bstrVal);

                    // Presentation.Close()
                    DispCall(pDoc, L"Close", nullptr, 0);
                    pDoc->Release();

                    if (SUCCEEDED(hr))
                    {
                        done[i] = true;
                        ::PostMessage(hWnd, WM_PPT_ITEM_DONE, (WPARAM)i, 1);
                    }
                }

                // Application.Quit()
                DispCall(pApp, L"Quit", nullptr, 0);
                pApp->Release();
            }
            CoUninitialize();
        }

        // ── Phase 2: LibreOffice (미완료 파일 병렬 실행) ───────
        if (!libreExe.IsEmpty())
        {
            std::vector<int> idx2;
            for (int i = 0; i < N; ++i)
                if (!done[i]) idx2.push_back(i);

            if (!idx2.empty())
            {
                std::vector<HANDLE> procs(idx2.size(), INVALID_HANDLE_VALUE);

                for (int j = 0; j < (int)idx2.size(); ++j)
                {
                    int i = idx2[j];
                    CString dir = items[i].outDir;
                    if (dir.IsEmpty())
                    {
                        TCHAR tmp[MAX_PATH];
                        _tcscpy_s(tmp, items[i].path);
                        PathRemoveFileSpec(tmp);
                        dir = tmp;
                    }
                    CString cmd;
                    cmd.Format(L"\"%s\" --headless --convert-to pdf \"%s\" --outdir \"%s\"",
                               (LPCTSTR)libreExe, (LPCTSTR)(items[i].path), (LPCTSTR)dir);
                    STARTUPINFO si = { sizeof(si) };
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;
                    PROCESS_INFORMATION pi = {};
                    if (CreateProcess(nullptr, cmd.GetBuffer(), nullptr, nullptr,
                                      FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
                    {
                        procs[j] = pi.hProcess;
                        CloseHandle(pi.hThread);
                    }
                    cmd.ReleaseBuffer();
                }

                // 유효 핸들만 모아 MAXIMUM_WAIT_OBJECTS 단위로 일괄 대기
                std::vector<HANDLE> valid;
                for (HANDLE h : procs)
                    if (h != INVALID_HANDLE_VALUE) valid.push_back(h);

                for (size_t s = 0; s < valid.size(); s += MAXIMUM_WAIT_OBJECTS)
                {
                    DWORD cnt = (DWORD)min(valid.size() - s, (size_t)MAXIMUM_WAIT_OBJECTS);
                    WaitForMultipleObjects(cnt, valid.data() + s, TRUE, 120000);
                }

                for (int j = 0; j < (int)idx2.size(); ++j)
                {
                    int i = idx2[j];
                    bool ok = false;
                    if (procs[j] != INVALID_HANDLE_VALUE)
                    {
                        DWORD exitCode = 1;
                        GetExitCodeProcess(procs[j], &exitCode);
                        CloseHandle(procs[j]);
                        ok = (exitCode == 0);
                    }
                    done[i] = ok;
                    ::PostMessage(hWnd, WM_PPT_ITEM_DONE, (WPARAM)i, ok ? 2 : 0);
                }
            }
        }

        // 두 엔진 모두 실패한 항목 통보
        int nOk = 0;
        for (int i = 0; i < N; ++i)
        {
            if (!done[i])
                ::PostMessage(hWnd, WM_PPT_ITEM_DONE, (WPARAM)i, 0);
            if (done[i]) ++nOk;
        }
        ::PostMessage(hNotify, WM_PPT_CONVERT_DONE, nOk > 0 ? 1 : 0, 0);
    });
    m_worker.detach();
}

void CPptConvertDlg::OnCommonClear(bool bConvDone)
{
    if (bConvDone)
    {
        if (::MessageBoxW(GetSafeHwnd(),
                          LS(IDS_PPT_CONFIRM_RESET),
                          LS(IDS_PPT_CONFIRM_TITLE),
                          MB_YESNO | MB_ICONQUESTION) != IDYES)
            return;
        m_entries.clear();
        m_listPpt.DeleteAllItems();
        m_bDone = false;
        SetStatus(_T(""));
    }
    else
    {
        int sel = m_listPpt.GetNextItem(-1, LVNI_SELECTED);
        if (sel < 0) return;
        m_entries.erase(m_entries.begin() + sel);
        m_listPpt.DeleteItem(sel);
    }
    NotifyHostStateChanged();
}

CString CPptConvertDlg::RunLabel()
{
    return m_bRunning ? LS(IDS_PPT_STATUS_WORKING) : LS(IDS_PPT_BTN_CONVERT);
}

bool CPptConvertDlg::CanRun()
{
    return !m_bRunning && !m_entries.empty();
}

bool CPptConvertDlg::CanDelete()
{
    if (m_bRunning) return false;
    if (m_bDone)    return true;
    return m_listPpt.GetNextItem(-1, LVNI_SELECTED) >= 0;
}

// ── 파일 추가 ─────────────────────────────────────────────────

void CPptConvertDlg::AddPptFiles(const std::vector<CString>& paths)
{
    for (const auto& p : paths)
    {
        CString ext = PathFindExtension(p);
        ext.MakeLower();
        if (ext != _T(".ppt") && ext != _T(".pptx")) continue;

        bool dup = false;
        for (const auto& e : m_entries)
            if (e.path.CompareNoCase(p) == 0) { dup = true; break; }
        if (dup) continue;

        PptEntry e;
        e.path = p;
        e.name = PathFindFileName(p);
        m_entries.push_back(std::move(e));
    }
    RefreshList();
    NotifyHostStateChanged();
}

// ── 리스트 갱신 ───────────────────────────────────────────────

void CPptConvertDlg::RefreshList()
{
    m_listPpt.DeleteAllItems();
    for (int i = 0; i < (int)m_entries.size(); ++i)
    {
        m_listPpt.InsertItem(i, m_entries[i].name);
        m_listPpt.SetItemText(i, 1, m_entries[i].engine);
        m_listPpt.SetItemText(i, 2, m_entries[i].status);
    }
    AdjustListColumns();
}

void CPptConvertDlg::UpdateEntryRow(int idx)
{
    if (idx < 0 || idx >= (int)m_entries.size()) return;
    m_listPpt.SetItemText(idx, 1, m_entries[idx].engine);
    m_listPpt.SetItemText(idx, 2, m_entries[idx].status);
}

void CPptConvertDlg::AdjustListColumns()
{
    if (!m_listPpt.GetSafeHwnd()) return;
    CRect rc;
    m_listPpt.GetClientRect(&rc);
    int w = rc.Width() - GetSystemMetrics(SM_CXVSCROLL) - 2;
    const int ENG_W = 90, STA_W = 70;
    int nameW = max(60, w - ENG_W - STA_W);
    m_listPpt.SetColumnWidth(0, nameW);
    m_listPpt.SetColumnWidth(1, ENG_W);
    m_listPpt.SetColumnWidth(2, STA_W);
}

void CPptConvertDlg::SetStatus(const CString& msg)
{
    if (m_lblStatus.GetSafeHwnd())
        m_lblStatus.SetWindowText(msg);
}

void CPptConvertDlg::NotifyHostStateChanged()
{
    if (m_hHostNotify)
        ::PostMessage(m_hHostNotify, WM_TAB_STATE_CHANGED, 0, 0);
}

// ── 변환 완료 콜백 ────────────────────────────────────────────

void CPptConvertDlg::NotifyItemDone(int idx, bool ok)
{
    if (idx < 0 || idx >= (int)m_entries.size()) return;
    m_entries[idx].done   = true;
    m_entries[idx].failed = !ok;
    m_entries[idx].status = LS(ok ? IDS_PPT_STATUS_DONE : IDS_PPT_STATUS_FAIL);
    UpdateEntryRow(idx);
}

void CPptConvertDlg::NotifyConvertDone(bool ok)
{
    m_bRunning = false;
    m_bDone    = true;
    SetStatus(LS(ok ? IDS_PPT_STATUS_DONE : IDS_PPT_STATUS_FAIL));
    NotifyHostStateChanged();
}

// ── WM_PPT_ITEM_DONE ──────────────────────────────────────────

LRESULT CPptConvertDlg::OnPptItemDone(WPARAM wParam, LPARAM lParam)
{
    int idx = (int)wParam;
    // lParam: 0=실패, 1=PowerPoint성공, 2=LibreOffice성공
    if (idx >= 0 && idx < (int)m_entries.size())
    {
        if (lParam == 1)      m_entries[idx].engine = LS(IDS_PPT_ENGINE_PPT);
        else if (lParam == 2) m_entries[idx].engine = LS(IDS_PPT_ENGINE_LIBRE);
        NotifyItemDone(idx, lParam != 0);
    }
    return 0;
}

// ── 드래그앤드롭 ──────────────────────────────────────────────

void CPptConvertDlg::OnDropFiles(HDROP hDropInfo)
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

// ── 리스트 이벤트 ─────────────────────────────────────────────

void CPptConvertDlg::OnLvnItemChangedListPpt(NMHDR*, LRESULT* pResult)
{
    NotifyHostStateChanged();
    *pResult = 0;
}

void CPptConvertDlg::OnNMClickListPpt(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx >= 0 && idx < (int)m_entries.size())
        m_preview.LoadShellThumbnail(m_entries[idx].path);
    else
        m_preview.Clear();
    *pResult = 0;
}

void CPptConvertDlg::OnNMDblclkListPpt(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx >= 0 && idx < (int)m_entries.size())
        ShellExecute(nullptr, _T("open"), m_entries[idx].path, nullptr, nullptr, SW_SHOW);
    *pResult = 0;
}

void CPptConvertDlg::OnNMRClickListPpt(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx < 0 || idx >= (int)m_entries.size()) { *pResult = 0; return; }

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1, LS(IDS_PPT_MENU_OPEN));
    menu.AppendMenu(MF_STRING, 2, LS(IDS_PPT_MENU_OPEN_LOC));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, 3, LS(IDS_PPT_MENU_REMOVE));
    menu.AppendMenu(MF_STRING, 4, LS(IDS_PPT_MENU_CLEAR_ALL));

    CPoint pt;
    GetCursorPos(&pt);
    int cmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, this);

    switch (cmd)
    {
    case 1:
        ShellExecute(nullptr, _T("open"), m_entries[idx].path, nullptr, nullptr, SW_SHOW);
        break;
    case 2:
    {
        CString dir = m_entries[idx].path;
        PathRemoveFileSpec(dir.GetBuffer(MAX_PATH)); dir.ReleaseBuffer();
        ShellExecute(nullptr, _T("explore"), dir, nullptr, nullptr, SW_SHOW);
        break;
    }
    case 3:
        if (!m_bRunning)
        {
            m_entries.erase(m_entries.begin() + idx);
            m_listPpt.DeleteItem(idx);
            NotifyHostStateChanged();
        }
        break;
    case 4:
        if (!m_bRunning)
        {
            m_entries.clear();
            m_listPpt.DeleteAllItems();
            m_bDone = false;
            SetStatus(_T(""));
            NotifyHostStateChanged();
        }
        break;
    }
    *pResult = 0;
}

// ── 출력 폴더 찾기 버튼 ──────────────────────────────────────

void CPptConvertDlg::OnBnClickedOutputBrowse()
{
    BROWSEINFO bi = {};
    bi.hwndOwner = GetSafeHwnd();
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    CString title = LS(IDS_FOLDER_SELECT);
    bi.lpszTitle = title;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
    if (!pidl) return;

    TCHAR buf[MAX_PATH] = {};
    SHGetPathFromIDList(pidl, buf);
    CoTaskMemFree(pidl);
    m_editOutput.SetWindowText(buf);
}

// ── 언어 전환 ────────────────────────────────────────────────

void CPptConvertDlg::ApplyLanguage()
{
    m_toolTip.UpdateTipText(LS(IDS_PPT_TIP_LIST),   &m_listPpt);
    m_toolTip.UpdateTipText(LS(IDS_PPT_TIP_OUTPUT), &m_editOutput);
    m_toolTip.UpdateTipText(LS(IDS_PPT_TIP_BROWSE), &m_btnOutputBrowse);

    LVCOLUMN lvc = {}; lvc.mask = LVCF_TEXT;
    CString c0 = LS(IDS_PPT_COL0); lvc.pszText = c0.GetBuffer(); m_listPpt.SetColumn(0, &lvc); c0.ReleaseBuffer();
    CString c1 = LS(IDS_PPT_COL1); lvc.pszText = c1.GetBuffer(); m_listPpt.SetColumn(1, &lvc); c1.ReleaseBuffer();
    CString c2 = LS(IDS_PPT_COL2); lvc.pszText = c2.GetBuffer(); m_listPpt.SetColumn(2, &lvc); c2.ReleaseBuffer();

    for (int i = 0; i < (int)m_entries.size(); ++i)
    {
        if (!m_entries[i].status.IsEmpty())
            m_entries[i].status = LS(m_entries[i].failed
                ? IDS_PPT_STATUS_FAIL : IDS_PPT_STATUS_DONE);
        m_listPpt.SetItemText(i, 2, m_entries[i].status);
    }
}

// ── 리사이즈 ─────────────────────────────────────────────────

void CPptConvertDlg::OnSize(UINT nType, int cx, int cy)
{
    CTabDlgBase::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED || !m_listPpt.GetSafeHwnd()) return;
    ResizeControls(cx, cy);
}

void CPptConvertDlg::ResizeControls(int cx, int cy)
{
    if (m_initCx == 0) return;
    int dw = cx - m_initCx;
    int dh = cy - m_initCy;

    auto move = [](CWnd& w, CRect r)
    { w.MoveWindow(r.left, r.top, r.Width(), r.Height()); };

    CRect r;
    r = m_rcList0;      r.right += dw;                       move(m_listPpt,         r);
    r = m_rcOutputLbl0;                                       move(m_lblOutputPath,   r);
    r = m_rcOutput0;    r.right += dw;                       move(m_editOutput,      r);
    r = m_rcBrowse0;    r.OffsetRect(dw, 0);                 move(m_btnOutputBrowse, r);
    r = m_rcPreview0;   r.right += dw; r.bottom += dh;      move(m_preview,         r);
    r = m_rcStatus0;    r.right += dw; r.OffsetRect(0, dh); move(m_lblStatus,       r);

    AdjustListColumns();
}

void CPptConvertDlg::OnDestroy()
{
    CTabDlgBase::OnDestroy();
}
