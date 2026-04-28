#pragma once
#include "pch.h"

/**
 * @brief GDI+ 기반 이미지 미리보기 컨트롤 (CStatic 서브클래스).
 *
 * 선택된 파일의 이미지를 비율을 유지하여 Fit 렌더링으로 표시한다.
 * m_bitmap 은 unique_ptr 로 관리되어 소멸 시 자동 해제된다.
 *
 * 이미지 로딩에 실패하면 m_bLoadFailed 가 true 로 설정되고
 * 컨트롤 영역이 회색으로 표시된다.
 */
class CImagePreviewCtrl : public CStatic
{
public:
    CImagePreviewCtrl() = default;

    /**
     * @brief 파일 경로에서 이미지를 로드하여 표시한다.
     *
     * GDI+ Bitmap 으로 로드하며, JPEG/PNG/BMP/TIFF/GIF 를 지원한다.
     * 로드 실패 시 회색 배경을 표시한다.
     *
     * @param path 로드할 이미지 파일의 전체 경로
     */
    void LoadImage(const CString& path);

    /**
     * @brief 셸 썸네일(파일 아이콘)을 로드하여 표시한다.
     *
     * Word/PPT 등 이미지가 아닌 파일의 미리보기에 사용된다.
     * IShellItemImageFactory 를 통해 시스템 썸네일을 요청한다.
     *
     * @param path 썸네일을 가져올 파일의 전체 경로
     */
    void LoadShellThumbnail(const CString& path);

    /**
     * @brief 현재 표시 중인 이미지를 지우고 빈 상태로 초기화한다.
     */
    void Clear();

protected:
    /** @brief 컨트롤 영역에 이미지를 비율 유지 Fit 방식으로 그린다. */
    afx_msg void OnPaint();

    /**
     * @brief 배경 지우기를 직접 처리하여 깜빡임을 방지한다.
     * @return 항상 TRUE (배경을 직접 처리했음을 Windows 에 알림)
     */
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    std::unique_ptr<Gdiplus::Bitmap> m_bitmap;       ///< 현재 표시 중인 GDI+ 비트맵
    bool                             m_bLoadFailed = false; ///< 이미지 로드 실패 플래그

    DECLARE_MESSAGE_MAP()
};
