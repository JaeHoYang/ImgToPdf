#pragma once
#include "pch.h"

class CImgToPdfApp : public CWinApp
{
public:
    CImgToPdfApp();

    virtual BOOL InitInstance() override;

    DECLARE_MESSAGE_MAP()

private:
    ULONG_PTR m_gdiplusToken = 0;
};

extern CImgToPdfApp theApp;
