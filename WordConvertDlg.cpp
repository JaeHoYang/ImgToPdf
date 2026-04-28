#include "pch.h"
#include "WordConvertDlg.h"
#include "AppLang.h"
#include <comdef.h>

// ── 엔진 감지 ─────────────────────────────────────────────────

static CString FindWordExe()
{
    // HKCU / HKLM: SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\WINWORD.EXE
    const wchar_t* keys[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\WINWORD.EXE",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\App Paths\\WINWORD.EXE",
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

    // 레지스트리에서도 확인
    const wchar_t* regPaths[] = {
        L"SOFTWARE\\LibreOffice\\UNO\\InstallPath",
        L"SOFTWARE\\WOW6432Node\\LibreOffice\\UNO\\InstallPath",
    };
    HKEY roots[] = { HKEY_LOCAL_MACHINE };
    for (HKEY root : roots)
    {
        for (const wchar_t* key : regPaths)
        {
            HKEY hk;
            if (RegOpenKeyEx(root, key, 0, KEY_READ, &hk) == ERROR_SUCCESS)
            {
                TCHAR buf[MAX_PATH] = {};
                DWORD sz = sizeof(buf);
                RegQueryValueEx(hk, nullptr, nullptr, nullptr, (LPBYTE)buf, &sz);
                RegCloseKey(hk);
                CString path = buf;
                // InstallPath 는 "program" 디렉토리
                path.TrimRight(_T("\\"));
                path += _T("\\soffice.exe");
                if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
                    return path;
            }
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

static HRESULT DispSetPropBool(IDispatch* p, LPCWSTR name, BOOL val)
{
    BSTR b = SysAllocString(name);
    DISPID id;
    HRESULT hr = p->GetIDsOfNames(IID_NULL, &b, 1, LOCALE_SYSTEM_DEFAULT, &id);
    SysFreeString(b);
    if (FAILED(hr)) return hr;

    VARIANT v; VariantInit(&v);
    v.vt = VT_BOOL; v.boolVal = val ? VARIANT_TRUE : VARIANT_FALSE;
    DISPID putId = DISPID_PROPERTYPUT;
    DISPPARAMS dp = { &v, &putId, 1, 1 };
    return p->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                     DISPATCH_PROPERTYPUT, &dp, nullptr, nullptr, nullptr);
}

static HRESULT DispCall(IDispatch* p, LPCWSTR name,
                        VARIANT* args, int nArgs, VARIANT* pResult = nullptr)
{
    BSTR b = SysAllocString(name);
    DISPID id;
    HRESULT hr = p->GetIDsOfNames(IID_NULL, &b, 1, LOCALE_SYSTEM_DEFAULT, &id);
    SysFreeString(b);
    if (FAILED(hr)) return hr;

    // IDispatch 인수는 역순
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
    // 파일명(확장자 제외) 추출
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

BEGIN_MESSAGE_MAP(CWordConvertDlg, CTabDlgBase)
    ON_WM_DROPFILES()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_WORD_BROWSE,   &CWordConvertDlg::OnBnClickedOutputBrowse)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_WORD, &CWordConvertDlg::OnLvnItemChangedListWord)
    ON_NOTIFY(NM_CLICK,        IDC_LIST_WORD, &CWordConvertDlg::OnNMClickListWord)
    ON_NOTIFY(NM_RCLICK,       IDC_LIST_WORD, &CWordConvertDlg::OnNMRClickListWord)
    ON_NOTIFY(NM_DBLCLK,      IDC_LIST_WORD, &CWordConvertDlg::OnNMDblclkListWord)
    ON_MESSAGE(WM_WORD_ITEM_DONE, &CWordConvertDlg::OnWordItemDone)
END_MESSAGE_MAP()

CWordConvertDlg::CWordConvertDlg(CWnd* pParent)
    : CTabDlgBase(IDD_TAB4, pParent)
{
}

// ── DoDataExchange ────────────────────────────────────────────

void CWordConvertDlg::DoDataExchange(CDataExchange* pDX)
{
    CTabDlgBase::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_WORD,           m_listWord);
    DDX_Control(pDX, IDC_STATIC_WORD_PREVIEW, m_preview);
    DDX_Control(pDX, IDC_LBL_WORD_OUTPUT,    m_lblOutputPath);
    DDX_Control(pDX, IDC_EDIT_WORD_OUTPUT,   m_editOutput);
    DDX_Control(pDX, IDC_BTN_WORD_BROWSE,    m_btnOutputBrowse);
    DDX_Control(pDX, IDC_STATIC_WORD_STATUS, m_lblStatus);
}

// ── OnInitDialog ─────────────────────────────────────────────

BOOL CWordConvertDlg::OnInitDialog()
{
    CTabDlgBase::OnInitDialog();

    DragAcceptFiles(TRUE);
    SetBackgroundColor(RGB(248, 249, 252));

    m_btnOutputBrowse.SetColors(RGB(255, 255, 255), RGB(235, 243, 255), RGB(37, 99, 235));

    m_listWord.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listWord.InsertColumn(0, LS(IDS_WORD_COL0), LVCFMT_LEFT,   220);
    m_listWord.InsertColumn(1, LS(IDS_WORD_COL1), LVCFMT_CENTER,  90);
    m_listWord.InsertColumn(2, LS(IDS_WORD_COL2), LVCFMT_CENTER,  70);

    CRect rcClient;
    GetClientRect(&rcClient);
    m_initCx = rcClient.Width();
    m_initCy = rcClient.Height();

    auto cap = [&](CWnd& w, CRect& rc) { w.GetWindowRect(&rc); ScreenToClient(&rc); };
    cap(m_listWord,       m_rcList0);
    cap(m_preview,        m_rcPreview0);
    cap(m_lblOutputPath,  m_rcOutputLbl0);
    cap(m_editOutput,     m_rcOutput0);
    cap(m_btnOutputBrowse,m_rcBrowse0);
    cap(m_lblStatus,      m_rcStatus0);

    AdjustListColumns();

    m_toolTip.Create(this, TTS_ALWAYSTIP);
    m_toolTip.SetMaxTipWidth(260);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 400);
    m_toolTip.AddTool(&m_listWord,         LS(IDS_WORD_TIP_LIST));
    m_toolTip.AddTool(&m_preview,          LS(IDS_WORD_TIP_PREVIEW));
    m_toolTip.AddTool(&m_editOutput,       LS(IDS_WORD_TIP_OUTPUT));
    m_toolTip.AddTool(&m_btnOutputBrowse,  LS(IDS_WORD_TIP_BROWSE));
    m_toolTip.Activate(TRUE);

    return TRUE;
}

BOOL CWordConvertDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_toolTip.GetSafeHwnd()) m_toolTip.RelayEvent(pMsg);
    return CTabDlgBase::PreTranslateMessage(pMsg);
}

// ── CTabDlgBase interface ─────────────────────────────────────

void CWordConvertDlg::OnTabActivated(CEdit& editPath, CButton& checkMerge)
{
    CString cue = LS(IDS_WORD_CUE_PATH);
    editPath.SendMessage(EM_SETCUEBANNER, FALSE, (LPARAM)(LPCWSTR)cue);
    checkMerge.ShowWindow(SW_HIDE);
}

void CWordConvertDlg::OnCommonBrowse(CEdit& editPath)
{
    CString folder;
    editPath.GetWindowText(folder);

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

void CWordConvertDlg::OnCommonRun(bool)
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
                          LS(IDS_WORD_ERR_OUTDIR),
                          LS(IDS_WORD_ERR_TITLE),
                          MB_OK | MB_ICONWARNING);
            return;
        }
    }

    CString wordExe  = FindWordExe();
    CString libreExe = FindLibreOfficeExe();

    if (wordExe.IsEmpty() && libreExe.IsEmpty())
    {
        ::MessageBoxW(GetSafeHwnd(),
                      LS(IDS_WORD_ERR_NO_ENGINE),
                      LS(IDS_WORD_ERR_TITLE),
                      MB_OK | MB_ICONWARNING);
        return;
    }

    m_bRunning = true;
    m_bDone    = false;
    SetStatus(LS(IDS_WORD_STATUS_WORKING));
    NotifyHostStateChanged();

    HWND hWnd    = GetSafeHwnd();
    HWND hNotify = m_hHostNotify;

    // entries 복사 (스레드 캡처)
    struct Item { CString path; CString outDir; };
    std::vector<Item> items;
    items.reserve(m_entries.size());
    for (auto& e : m_entries)
        items.push_back({ e.path, outDir });

    m_worker = std::thread([items, wordExe, libreExe, hWnd, hNotify]()
    {
        const int N = (int)items.size();
        // lParam: 0=실패, 1=Word성공, 2=LibreOffice성공
        std::vector<bool> done(N, false);

        // ── Phase 1: Word COM (인스턴스 1개 재사용) ────────────
        if (!wordExe.IsEmpty())
        {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            CLSID clsid = {};
            IDispatch* pApp = nullptr;

            if (SUCCEEDED(CLSIDFromProgID(L"Word.Application", &clsid)) &&
                SUCCEEDED(CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER,
                                           IID_IDispatch, (void**)&pApp)))
            {
                DispSetPropBool(pApp, L"Visible", FALSE);

                for (int i = 0; i < N; ++i)
                {
                    CString out = BuildOutputPath(items[i].path, items[i].outDir);

                    VARIANT varDocs; VariantInit(&varDocs);
                    if (FAILED(DispGetProp(pApp, L"Documents", varDocs)) ||
                        varDocs.vt != VT_DISPATCH)
                        continue;
                    IDispatch* pDocs = varDocs.pdispVal;

                    VARIANT argOpen[1]; VariantInit(&argOpen[0]);
                    argOpen[0].vt      = VT_BSTR;
                    argOpen[0].bstrVal = SysAllocString(items[i].path);
                    VARIANT varDoc; VariantInit(&varDoc);
                    HRESULT hr = DispCall(pDocs, L"Open", argOpen, 1, &varDoc);
                    SysFreeString(argOpen[0].bstrVal);
                    pDocs->Release();

                    if (FAILED(hr) || varDoc.vt != VT_DISPATCH)
                        continue;
                    IDispatch* pDoc = varDoc.pdispVal;

                    VARIANT argExp[2];
                    VariantInit(&argExp[0]); VariantInit(&argExp[1]);
                    argExp[0].vt      = VT_BSTR;
                    argExp[0].bstrVal = SysAllocString(out);
                    argExp[1].vt      = VT_INT;
                    argExp[1].intVal  = 17; // wdExportFormatPDF
                    hr = DispCall(pDoc, L"ExportAsFixedFormat", argExp, 2);
                    SysFreeString(argExp[0].bstrVal);

                    VARIANT argClose[1]; VariantInit(&argClose[0]);
                    argClose[0].vt = VT_INT; argClose[0].intVal = 0;
                    DispCall(pDoc, L"Close", argClose, 1);
                    pDoc->Release();

                    if (SUCCEEDED(hr))
                    {
                        done[i] = true;
                        ::PostMessage(hWnd, WM_WORD_ITEM_DONE, (WPARAM)i, 1);
                    }
                }

                VARIANT argQuit[1]; VariantInit(&argQuit[0]);
                argQuit[0].vt = VT_INT; argQuit[0].intVal = 0;
                DispCall(pApp, L"Quit", argQuit, 1);
                pApp->Release();
            }
            CoUninitialize();
        }

        // ── Phase 2: LibreOffice (미완료 파일 병렬 실행) ───────
        if (!libreExe.IsEmpty())
        {
            std::vector<int>    idx2;
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
                    ::PostMessage(hWnd, WM_WORD_ITEM_DONE, (WPARAM)i, ok ? 2 : 0);
                }
            }
        }

        // 두 엔진 모두 실패한 항목 통보
        int nOk = 0;
        for (int i = 0; i < N; ++i)
        {
            if (!done[i])
                ::PostMessage(hWnd, WM_WORD_ITEM_DONE, (WPARAM)i, 0);
            if (done[i]) ++nOk;
        }
        ::PostMessage(hNotify, WM_WORD_CONVERT_DONE, nOk > 0 ? 1 : 0, 0);
    });
    m_worker.detach();
}

void CWordConvertDlg::OnCommonClear(bool bConvDone)
{
    if (bConvDone)
    {
        if (::MessageBoxW(GetSafeHwnd(),
                          LS(IDS_WORD_CONFIRM_RESET),
                          LS(IDS_WORD_CONFIRM_TITLE),
                          MB_YESNO | MB_ICONQUESTION) != IDYES)
            return;
        m_entries.clear();
        m_listWord.DeleteAllItems();
        m_bDone = false;
        SetStatus(_T(""));
    }
    else
    {
        int sel = m_listWord.GetNextItem(-1, LVNI_SELECTED);
        if (sel < 0) return;
        m_entries.erase(m_entries.begin() + sel);
        m_listWord.DeleteItem(sel);
    }
    NotifyHostStateChanged();
}

CString CWordConvertDlg::RunLabel()
{
    return m_bRunning ? LS(IDS_WORD_STATUS_WORKING) : LS(IDS_WORD_BTN_CONVERT);
}

bool CWordConvertDlg::CanRun()
{
    return !m_bRunning && !m_entries.empty();
}

bool CWordConvertDlg::CanDelete()
{
    if (m_bRunning) return false;
    if (m_bDone)    return true;
    return m_listWord.GetNextItem(-1, LVNI_SELECTED) >= 0;
}

// ── 파일 추가 ─────────────────────────────────────────────────

void CWordConvertDlg::AddWordFiles(const std::vector<CString>& paths)
{
    for (const auto& p : paths)
    {
        CString ext = PathFindExtension(p);
        ext.MakeLower();
        if (ext != _T(".doc") && ext != _T(".docx")) continue;

        bool dup = false;
        for (const auto& e : m_entries)
            if (e.path.CompareNoCase(p) == 0) { dup = true; break; }
        if (dup) continue;

        WordEntry e;
        e.path = p;
        e.name = PathFindFileName(p);
        m_entries.push_back(std::move(e));
    }
    RefreshList();
    NotifyHostStateChanged();
}

// ── 리스트 갱신 ───────────────────────────────────────────────

void CWordConvertDlg::RefreshList()
{
    m_listWord.DeleteAllItems();
    for (int i = 0; i < (int)m_entries.size(); ++i)
    {
        m_listWord.InsertItem(i, m_entries[i].name);
        m_listWord.SetItemText(i, 1, m_entries[i].engine);
        m_listWord.SetItemText(i, 2, m_entries[i].status);
    }
    AdjustListColumns();
}

void CWordConvertDlg::UpdateEntryRow(int idx)
{
    if (idx < 0 || idx >= (int)m_entries.size()) return;
    m_listWord.SetItemText(idx, 1, m_entries[idx].engine);
    m_listWord.SetItemText(idx, 2, m_entries[idx].status);
}

void CWordConvertDlg::AdjustListColumns()
{
    if (!m_listWord.GetSafeHwnd()) return;
    CRect rc;
    m_listWord.GetClientRect(&rc);
    int w = rc.Width() - GetSystemMetrics(SM_CXVSCROLL) - 2;
    const int ENG_W = 90, STA_W = 70;
    int nameW = max(60, w - ENG_W - STA_W);
    m_listWord.SetColumnWidth(0, nameW);
    m_listWord.SetColumnWidth(1, ENG_W);
    m_listWord.SetColumnWidth(2, STA_W);
}

void CWordConvertDlg::SetStatus(const CString& msg)
{
    m_lblStatus.SetWindowText(msg);
}

void CWordConvertDlg::NotifyHostStateChanged()
{
    if (m_hHostNotify)
        ::PostMessage(m_hHostNotify, WM_TAB_STATE_CHANGED, 0, 0);
}

// ── 변환 완료 콜백 (호스트 → 여기) ──────────────────────────

void CWordConvertDlg::NotifyItemDone(int idx, bool ok)
{
    if (idx < 0 || idx >= (int)m_entries.size()) return;
    m_entries[idx].done   = true;
    m_entries[idx].failed = !ok;
    m_entries[idx].status = LS(ok ? IDS_WORD_STATUS_DONE : IDS_WORD_STATUS_FAIL);
    UpdateEntryRow(idx);
}

void CWordConvertDlg::NotifyConvertDone(bool ok)
{
    m_bRunning = false;
    m_bDone    = true;
    SetStatus(LS(ok ? IDS_WORD_STATUS_DONE : IDS_WORD_STATUS_FAIL));
    NotifyHostStateChanged();
}

// ── WM_WORD_ITEM_DONE ─────────────────────────────────────────

LRESULT CWordConvertDlg::OnWordItemDone(WPARAM wParam, LPARAM lParam)
{
    int idx = (int)wParam;
    // lParam: 0=실패, 1=Word성공, 2=LibreOffice성공
    if (idx >= 0 && idx < (int)m_entries.size())
    {
        if (lParam == 1)      m_entries[idx].engine = LS(IDS_WORD_ENGINE_WORD);
        else if (lParam == 2) m_entries[idx].engine = LS(IDS_WORD_ENGINE_LIBRE);
        NotifyItemDone(idx, lParam != 0);
    }
    return 0;
}

// ── 드래그앤드롭 ──────────────────────────────────────────────

void CWordConvertDlg::OnDropFiles(HDROP hDropInfo)
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

void CWordConvertDlg::OnLvnItemChangedListWord(NMHDR*, LRESULT* pResult)
{
    NotifyHostStateChanged();
    *pResult = 0;
}

void CWordConvertDlg::OnNMClickListWord(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx >= 0 && idx < (int)m_entries.size())
        m_preview.LoadShellThumbnail(m_entries[idx].path);
    else
        m_preview.Clear();
    *pResult = 0;
}

void CWordConvertDlg::OnNMDblclkListWord(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx >= 0 && idx < (int)m_entries.size())
        ShellExecute(nullptr, _T("open"), m_entries[idx].path, nullptr, nullptr, SW_SHOW);
    *pResult = 0;
}

void CWordConvertDlg::OnNMRClickListWord(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    if (idx < 0 || idx >= (int)m_entries.size()) { *pResult = 0; return; }

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1, LS(IDS_WORD_MENU_OPEN));
    menu.AppendMenu(MF_STRING, 2, LS(IDS_WORD_MENU_OPEN_LOC));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, 3, LS(IDS_WORD_MENU_REMOVE));
    menu.AppendMenu(MF_STRING, 4, LS(IDS_WORD_MENU_CLEAR_ALL));

    CPoint pt;
    GetCursorPos(&pt);
    int cmd = menu.TrackPopupMenu(
        TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, this);

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
            m_listWord.DeleteItem(idx);
            NotifyHostStateChanged();
        }
        break;
    case 4:
        if (!m_bRunning)
        {
            m_entries.clear();
            m_listWord.DeleteAllItems();
            m_bDone = false;
            SetStatus(_T(""));
            NotifyHostStateChanged();
        }
        break;
    }
    *pResult = 0;
}

// ── 출력 폴더 찾기 버튼 ──────────────────────────────────────

void CWordConvertDlg::OnBnClickedOutputBrowse()
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

void CWordConvertDlg::ApplyLanguage()
{
    m_toolTip.UpdateTipText(LS(IDS_WORD_TIP_LIST),   &m_listWord);
    m_toolTip.UpdateTipText(LS(IDS_WORD_TIP_OUTPUT), &m_editOutput);
    m_toolTip.UpdateTipText(LS(IDS_WORD_TIP_BROWSE), &m_btnOutputBrowse);

    LVCOLUMN lvc = {}; lvc.mask = LVCF_TEXT;
    CString c0 = LS(IDS_WORD_COL0); lvc.pszText = c0.GetBuffer(); m_listWord.SetColumn(0, &lvc); c0.ReleaseBuffer();
    CString c1 = LS(IDS_WORD_COL1); lvc.pszText = c1.GetBuffer(); m_listWord.SetColumn(1, &lvc); c1.ReleaseBuffer();
    CString c2 = LS(IDS_WORD_COL2); lvc.pszText = c2.GetBuffer(); m_listWord.SetColumn(2, &lvc); c2.ReleaseBuffer();

    // 상태 문자열 재적용
    for (int i = 0; i < (int)m_entries.size(); ++i)
    {
        if (!m_entries[i].status.IsEmpty())
            m_entries[i].status = LS(m_entries[i].failed
                ? IDS_WORD_STATUS_FAIL : IDS_WORD_STATUS_DONE);
        m_listWord.SetItemText(i, 2, m_entries[i].status);
    }
}

// ── 리사이즈 ─────────────────────────────────────────────────

void CWordConvertDlg::OnSize(UINT nType, int cx, int cy)
{
    CTabDlgBase::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED || !m_listWord.GetSafeHwnd()) return;
    ResizeControls(cx, cy);
}

void CWordConvertDlg::ResizeControls(int cx, int cy)
{
    if (m_initCx == 0) return;
    int dw = cx - m_initCx;
    int dh = cy - m_initCy;

    auto move = [](CWnd& w, CRect r)
    { w.MoveWindow(r.left, r.top, r.Width(), r.Height()); };

    CRect r;
    r = m_rcList0;      r.right += dw;                       move(m_listWord,        r);
    r = m_rcOutputLbl0;                                       move(m_lblOutputPath,   r);
    r = m_rcOutput0;    r.right += dw;                       move(m_editOutput,      r);
    r = m_rcBrowse0;    r.OffsetRect(dw, 0);                 move(m_btnOutputBrowse, r);
    r = m_rcPreview0;   r.right += dw; r.bottom += dh;      move(m_preview,         r);
    r = m_rcStatus0;    r.right += dw; r.OffsetRect(0, dh); move(m_lblStatus,       r);

    AdjustListColumns();
}

void CWordConvertDlg::OnDestroy()
{
    CTabDlgBase::OnDestroy();
}
