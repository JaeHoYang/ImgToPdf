#pragma once
#include "pch.h"

/**
 * @brief 외부 PDF 라이브러리 없이 PDF 1.4 스트림을 직접 생성하는 유틸리티 클래스.
 *
 * - JPEG 이미지는 /DCTDecode 필터로 원본 바이트를 그대로 embed (무손실)
 * - 그 외 포맷(PNG, BMP, TIFF, GIF)은 stb_image 로 디코딩 후 raw RGB embed
 *
 * xref 테이블 오프셋은 오브젝트 번호 = 쓰기 순서 가 일치해야 유효한 PDF 가 생성된다.
 * WriteSingle() 의 오브젝트 번호 배정(1=Catalog, 2=Pages, 3=Image, 4=Content, 5=Page)은
 * 이 순서를 반드시 지켜야 한다.
 *
 * 한글 경로 지원: stb_image 에 STBI_WINDOWS_UTF8 정의,
 * std::ofstream 은 wchar_t 경로를 직접 전달.
 */
class PdfWriter
{
public:
    /**
     * @brief 병렬 로딩 및 합치기 모드에서 사용하는 이미지 데이터 컨테이너.
     *
     * 병렬 Phase1 에서 각 워커가 LoadImage() 로 이 구조체를 채우고,
     * Phase2 에서 단일 스레드가 WriteMergedFromData() 로 PDF 를 생성한다.
     */
    struct ImageData
    {
        int                  width    = 0;     ///< 이미지 픽셀 너비
        int                  height   = 0;     ///< 이미지 픽셀 높이
        int                  channels = 3;     ///< 채널 수 (RGB=3)
        std::vector<uint8_t> pixels;           ///< stb_image 디코딩된 RGB 픽셀 데이터
        std::vector<uint8_t> raw;              ///< JPEG 원본 바이트 (DCTDecode 용)
        bool                 isJpeg   = false; ///< true 이면 raw 데이터로 embed
        int                  pageW    = 0;     ///< PDF MediaBox 너비 (points, 0 이면 width 사용)
        int                  pageH    = 0;     ///< PDF MediaBox 높이 (points, 0 이면 height 사용)
    };

    /**
     * @brief 이미지 1개를 PDF 1개로 변환한다.
     * @param srcPath  원본 이미지 파일 경로 (한글 경로 지원)
     * @param dstPath  출력 PDF 파일 경로
     * @param outError 실패 시 원인 문자열이 저장됨
     * @param quality  비JPEG 이미지 압축 품질 (10~100, 기본값 90)
     * @return 성공 시 true
     */
    static bool WriteSingle(const CString& srcPath, const CString& dstPath,
                            CString& outError, int quality = 90);

    /**
     * @brief 여러 이미지를 순서대로 내부 직렬 로딩하여 단일 다중페이지 PDF 로 합친다.
     * @param srcPaths 원본 이미지 파일 경로 목록
     * @param dstPath  출력 PDF 파일 경로
     * @param outError 실패 시 원인 문자열이 저장됨
     * @param quality  비JPEG 이미지 압축 품질 (10~100, 기본값 90)
     * @return 성공 시 true
     */
    static bool WriteMerged(const std::vector<CString>& srcPaths,
                            const CString& dstPath, CString& outError, int quality = 90);

    /**
     * @brief 이미지 파일을 로드하여 ImageData 구조체에 채운다.
     *
     * 병렬 Phase1 에서 각 워커 스레드가 호출한다.
     * JPEG 는 raw 바이트를 보존하고, 그 외 포맷은 RGB 픽셀로 디코딩한다.
     *
     * @param path     로드할 이미지 파일 경로
     * @param out      결과가 채워질 ImageData
     * @param outError 실패 시 원인 문자열이 저장됨
     * @param quality  비JPEG 이미지 품질 힌트 (10~100)
     * @return 성공 시 true
     */
    static bool LoadImage(const CString& path, ImageData& out, CString& outError, int quality = 90);

    /**
     * @brief 미리 로딩된 ImageData 목록으로 다중페이지 PDF 를 생성한다.
     *
     * 병렬 Phase1 완료 후 Phase2 에서 단일 스레드가 호출한다.
     *
     * @param images   ImageData 목록 (순서대로 PDF 페이지가 됨)
     * @param dstPath  출력 PDF 파일 경로
     * @param outError 실패 시 원인 문자열이 저장됨
     * @return 성공 시 true
     */
    static bool WriteMergedFromData(const std::vector<ImageData>& images,
                                    const CString& dstPath, CString& outError);

private:
    /**
     * @brief 이미지 XObject PDF 오브젝트를 스트림에 쓴다.
     * @param os     출력 스트림
     * @param img    이미지 데이터
     * @param objNum 이 오브젝트에 부여할 번호
     * @param xref   xref 오프셋 테이블 (append)
     * @return 다음 오브젝트 번호
     */
    static int  WriteImageObj   (std::ostream& os, const ImageData& img,
                                 int objNum, std::vector<size_t>& xref);

    /**
     * @brief 단일 Page 오브젝트를 스트림에 쓴다.
     * @param os        출력 스트림
     * @param pagesObj  Pages 오브젝트 번호
     * @param imgObj    Image XObject 오브젝트 번호
     * @param contentObj Content 스트림 오브젝트 번호
     * @param w         페이지 너비 (points)
     * @param h         페이지 높이 (points)
     * @param objNum    이 오브젝트에 부여할 번호
     * @param xref      xref 오프셋 테이블 (append)
     * @return 다음 오브젝트 번호
     */
    static int  WritePageObj    (std::ostream& os, int pagesObj,
                                 int imgObj, int contentObj,
                                 int w, int h, int objNum,
                                 std::vector<size_t>& xref);

    /**
     * @brief 이미지를 페이지에 배치하는 Content 스트림 오브젝트를 쓴다.
     * @param os     출력 스트림
     * @param w      이미지 너비 (points)
     * @param h      이미지 높이 (points)
     * @param objNum 이 오브젝트에 부여할 번호
     * @param xref   xref 오프셋 테이블 (append)
     * @return 다음 오브젝트 번호
     */
    static int  WriteContentObj (std::ostream& os, int w, int h,
                                 int objNum, std::vector<size_t>& xref);

    /**
     * @brief PDF xref 테이블과 trailer 를 스트림 끝에 쓴다.
     * @param os       출력 스트림
     * @param xref     각 오브젝트의 파일 내 바이트 오프셋 목록
     * @param rootObj  Catalog 오브젝트 번호
     * @param pagesObj Pages 오브젝트 번호
     * @param objCount 전체 오브젝트 수
     */
    static void WriteXrefTrailer(std::ostream& os,
                                 const std::vector<size_t>& xref,
                                 int rootObj, int pagesObj, int objCount);
};
