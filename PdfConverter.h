#pragma once
#include "pch.h"

class PdfConverter
{
public:
    // PDF 파일의 전체 페이지 수 반환 (0 = 실패/읽기 불가)
    static int  GetPageCount(const CString& pdfPath);

    // pageIndex(0-based) 페이지를 JPG 파일로 저장
    // 반환: true = 성공, false = 실패 (errMsg에 이유)
    static bool RenderPageToJpg(const CString& pdfPath,
                                int            pageIndex,
                                const CString& dstPath,
                                CString&       errMsg,
                                int            quality = 90);

private:
    static CLSID GetJpegEncoderClsid();
};
