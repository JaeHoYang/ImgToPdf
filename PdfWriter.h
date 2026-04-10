#pragma once
#include "pch.h"

class PdfWriter
{
public:
    // ── 이미지 데이터 (병렬 로딩용으로 public 노출) ──────────
    struct ImageData
    {
        int                  width    = 0;
        int                  height   = 0;
        int                  channels = 3;
        std::vector<uint8_t> pixels;   // stb_image 디코딩된 RGB 픽셀
        std::vector<uint8_t> raw;      // JPEG 원본 바이트 (DCTDecode용)
        bool                 isJpeg   = false;
    };

    // ── 공개 API ─────────────────────────────────────────────
    // 이미지 1개 → PDF 1개  (outError: 실패 시 원인 문자열)
    static bool WriteSingle(const CString& srcPath, const CString& dstPath,
                            CString& outError);

    // 여러 이미지 경로 → 단일 다중페이지 PDF (내부 직렬 로딩)
    static bool WriteMerged(const std::vector<CString>& srcPaths,
                            const CString& dstPath, CString& outError);

    // 이미지 로딩 (병렬 로딩 후 merge 호출 시 사용)
    static bool LoadImage(const CString& path, ImageData& out, CString& outError);

    // 미리 로딩된 ImageData 목록으로 다중페이지 PDF 생성
    static bool WriteMergedFromData(const std::vector<ImageData>& images,
                                    const CString& dstPath, CString& outError);

private:
    // PDF 오브젝트 쓰기 헬퍼
    static int  WriteImageObj   (std::ostream& os, const ImageData& img,
                                 int objNum, std::vector<size_t>& xref);
    static int  WritePageObj    (std::ostream& os, int pagesObj,
                                 int imgObj, int contentObj,
                                 int w, int h, int objNum,
                                 std::vector<size_t>& xref);
    static int  WriteContentObj (std::ostream& os, int w, int h,
                                 int objNum, std::vector<size_t>& xref);
    static void WriteXrefTrailer(std::ostream& os,
                                 const std::vector<size_t>& xref,
                                 int rootObj, int pagesObj, int objCount);
};
