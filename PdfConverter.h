#pragma once
#include "pch.h"

/**
 * @brief Windows.Data.Pdf WinRT API 를 사용하여 PDF 를 읽고 JPG 로 렌더링하는 유틸리티 클래스.
 *
 * WRL + SyncWait 패턴으로 비동기 WinRT API 를 동기 호출로 래핑한다.
 * 외부 PDF DLL 없이 Windows 10 내장 API 만 사용한다 (Windows.Data.Pdf).
 *
 * 각 워커 스레드에서 호출하기 전에 RoInitialize / RoUninitialize 가 필요하며,
 * 내부적으로 RAII struct RoInit 패턴으로 처리한다 — 예외 발생 시에도 RoUninitialize 보장.
 *
 * 필요 링크: runtimeobject.lib, shcore.lib (CreateStreamOverRandomAccessStream)
 */
class PdfConverter
{
public:
    /**
     * @brief PDF 파일의 전체 페이지 수를 반환한다.
     *
     * StorageFile → PdfDocument → get_PageCount 순으로 WinRT API 를 호출한다.
     * 읽기 실패 또는 파일이 유효하지 않으면 0 을 반환한다.
     *
     * @param pdfPath PDF 파일의 전체 경로 (한글 경로 지원)
     * @return 페이지 수 (실패 시 0)
     */
    static int  GetPageCount(const CString& pdfPath);

    /**
     * @brief PDF 의 특정 페이지를 JPG 파일로 렌더링하여 저장한다.
     *
     * PdfPage → InMemoryRandomAccessStream → RenderToStreamAsync → Gdiplus::Bitmap
     * → JPEG 저장(기본 품질 90) 순으로 처리한다.
     *
     * @param pdfPath   PDF 파일의 전체 경로
     * @param pageIndex 렌더링할 페이지 인덱스 (0-based)
     * @param dstPath   출력 JPG 파일의 전체 경로
     * @param errMsg    실패 시 원인 문자열이 저장됨
     * @param quality   JPEG 저장 품질 (10~100, 기본값 90)
     * @return 성공 시 true, 실패 시 false (errMsg 에 이유 저장)
     */
    static bool RenderPageToJpg(const CString& pdfPath,
                                int            pageIndex,
                                const CString& dstPath,
                                CString&       errMsg,
                                int            quality = 90);

private:
    /**
     * @brief 시스템에서 JPEG 인코더의 CLSID 를 조회한다.
     *
     * GDI+ Gdiplus::GetImageEncoders 를 통해 "image/jpeg" MIME 타입 인코더를 찾는다.
     *
     * @return JPEG 인코더 CLSID (없으면 CLSID_NULL)
     */
    static CLSID GetJpegEncoderClsid();
};
