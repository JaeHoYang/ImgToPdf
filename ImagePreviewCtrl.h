#pragma once
#include "pch.h"

class CImagePreviewCtrl : public CStatic
{
public:
    CImagePreviewCtrl() = default;

    void LoadImage(const CString& path);
    void Clear();

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    std::unique_ptr<Gdiplus::Bitmap> m_bitmap;
    bool m_bLoadFailed = false;

    DECLARE_MESSAGE_MAP()
};
