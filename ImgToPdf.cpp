#include "pch.h"
#include "ImgToPdf.h"
#include "ImgToPdfDlg.h"

// ComCtl32 v6 활성화 — EM_SETCUEBANNER 등 v6 전용 기능 사용에 필요
#pragma comment(linker, "/manifestdependency:\"type='win32' " \
    "name='Microsoft.Windows.Common-Controls' version='6.0.0.0' " \
    "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

CImgToPdfApp theApp;

BEGIN_MESSAGE_MAP(CImgToPdfApp, CWinApp)
END_MESSAGE_MAP()

CImgToPdfApp::CImgToPdfApp()
{
}

BOOL CImgToPdfApp::InitInstance()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    CWinApp::InitInstance();
    SetRegistryKey(_T("jaeho"));

    // GDI+ 초기화
    Gdiplus::GdiplusStartupInput gdiplusInput;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusInput, nullptr);

    AfxInitRichEdit2(); // MD 미리보기용 RichEdit 5.0 (MSFTEDIT.DLL)

    {
        CImgToPdfDlg dlg;
        m_pMainWnd = &dlg;
        dlg.DoModal();
    }  // dlg (및 m_bitmap) 소멸 후 GDI+ 종료

    Gdiplus::GdiplusShutdown(m_gdiplusToken);

    CoUninitialize();
    return FALSE;
}
