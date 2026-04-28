#pragma once
#include "pch.h"

/**
 * @brief 변환 진행 상황을 3색 텍스트로 표시하는 커스텀 레이블 (CStatic 서브클래스).
 *
 * Owner-draw 방식으로 "(완료/실패/총)" 형태의 텍스트를 렌더링하며,
 * 각 숫자는 완료=초록, 실패=빨강, 총=기본색으로 표시된다.
 *
 * SetIdle() 로 초기 상태로 복귀하거나,
 * SetCounts() 로 현재 진행 수치를 갱신한다.
 */
class CProgressLabel : public CStatic
{
public:
    /**
     * @brief 레이블을 유휴(idle) 상태로 초기화한다.
     *
     * 카운터를 모두 0 으로 재설정하고 빈 화면으로 다시 그린다.
     */
    void SetIdle();

    /**
     * @brief 변환 진행 카운터를 갱신하고 즉시 다시 그린다.
     * @param success 완료(성공) 항목 수
     * @param fail    실패 항목 수
     * @param total   전체 항목 수
     */
    void SetCounts(int success, int fail, int total);

protected:
    /** @brief Owner-draw: 3색 "(완료/실패/총)" 텍스트를 그린다. */
    afx_msg void OnPaint();

    /**
     * @brief 배경 지우기를 직접 처리하여 깜빡임을 방지한다.
     * @return 항상 TRUE
     */
    afx_msg BOOL OnEraseBkgnd(CDC*);

    int  m_success = 0;   ///< 완료(성공) 항목 수
    int  m_fail    = 0;   ///< 실패 항목 수
    int  m_total   = 0;   ///< 전체 항목 수
    bool m_idle    = true; ///< 유휴 상태 플래그 (true 이면 텍스트 미표시)

    DECLARE_MESSAGE_MAP()
};
