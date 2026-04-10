#pragma once
#include "pch.h"

class CProgressLabel : public CStatic
{
public:
    void SetIdle();
    void SetCounts(int success, int fail, int total);

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*);

    int  m_success = 0;
    int  m_fail    = 0;
    int  m_total   = 0;
    bool m_idle    = true;

    DECLARE_MESSAGE_MAP()
};
