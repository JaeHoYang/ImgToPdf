#include "pch.h"
#include "PdfWriter.h"

// stb_image: 헤더 온리 라이브러리, 이 파일에서만 구현부 포함
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8   // 한글 경로 지원
#include "third_party/stb_image.h"

// ── 내부 유틸 ────────────────────────────────────────────────

static std::string WideToUtf8(const CString& str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, str, -1, &out[0], len, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

static bool IsJpeg(const CString& path)
{
    CString ext = PathFindExtension(path);
    ext.MakeLower();
    return (ext == _T(".jpg") || ext == _T(".jpeg"));
}

// ── 이미지 로딩 ──────────────────────────────────────────────

bool PdfWriter::LoadImage(const CString& path, ImageData& out, CString& outError)
{
    std::string utf8 = WideToUtf8(path);
    int w, h, ch;
    unsigned char* data = stbi_load(utf8.c_str(), &w, &h, &ch, 3);
    if (!data)
    {
        const char* reason = stbi_failure_reason();
        CString msg;
        msg.Format(_T("이미지 로드 실패: %S"), reason ? reason : "unknown");
        outError = msg;
        return false;
    }

    out.width    = w;
    out.height   = h;
    out.channels = 3;
    out.pixels.assign(data, data + (size_t)w * h * 3);
    stbi_image_free(data);

    out.isJpeg = IsJpeg(path);
    if (out.isJpeg)
    {
        std::ifstream f((LPCTSTR)path, std::ios::binary);
        if (f)
            out.raw.assign(std::istreambuf_iterator<char>(f), {});
        else
            out.isJpeg = false;
    }
    return true;
}

// ── PDF 오브젝트 쓰기 헬퍼 ───────────────────────────────────

static size_t StreamPos(std::ostream& os)
{
    return (size_t)os.tellp();
}

int PdfWriter::WriteImageObj(std::ostream& os, const ImageData& img,
                             int objNum, std::vector<size_t>& xref)
{
    xref.push_back(StreamPos(os));
    os << objNum << " 0 obj\n"
       << "<< /Type /XObject /Subtype /Image\n"
       << "   /Width "  << img.width  << "\n"
       << "   /Height " << img.height << "\n"
       << "   /ColorSpace /DeviceRGB\n"
       << "   /BitsPerComponent 8\n";

    if (img.isJpeg)
    {
        os << "   /Filter /DCTDecode\n"
           << "   /Length " << img.raw.size() << "\n>>\nstream\n";
        os.write(reinterpret_cast<const char*>(img.raw.data()), img.raw.size());
    }
    else
    {
        // 무압축 RGB
        size_t len = (size_t)img.width * img.height * 3;
        os << "   /Length " << len << "\n>>\nstream\n";
        os.write(reinterpret_cast<const char*>(img.pixels.data()), len);
    }
    os << "\nendstream\nendobj\n";
    return objNum;
}

int PdfWriter::WriteContentObj(std::ostream& os, int w, int h,
                               int objNum, std::vector<size_t>& xref)
{
    // 이미지를 페이지 전체에 배치하는 content stream
    std::string content = "q " + std::to_string(w) + " 0 0 "
                        + std::to_string(h) + " 0 0 cm /Im0 Do Q\n";
    xref.push_back(StreamPos(os));
    os << objNum << " 0 obj\n"
       << "<< /Length " << content.size() << " >>\nstream\n"
       << content
       << "endstream\nendobj\n";
    return objNum;
}

int PdfWriter::WritePageObj(std::ostream& os, int pagesObj,
                            int imgObj, int contentObj,
                            int w, int h, int objNum,
                            std::vector<size_t>& xref)
{
    xref.push_back(StreamPos(os));
    os << objNum << " 0 obj\n"
       << "<< /Type /Page /Parent " << pagesObj << " 0 R\n"
       << "   /MediaBox [0 0 " << w << " " << h << "]\n"
       << "   /Resources << /XObject << /Im0 " << imgObj << " 0 R >> >>\n"
       << "   /Contents " << contentObj << " 0 R\n>>\nendobj\n";
    return objNum;
}

void PdfWriter::WriteXrefTrailer(std::ostream& os,
                                 const std::vector<size_t>& xref,
                                 int rootObj, int pagesObj, int objCount)
{
    size_t xrefPos = StreamPos(os);
    os << "xref\n0 " << (objCount + 1) << "\n";
    os << "0000000000 65535 f \n";
    for (size_t off : xref)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%010zu 00000 n \n", off);
        os << buf;
    }
    os << "trailer\n"
       << "<< /Size " << (objCount + 1)
       << " /Root " << rootObj << " 0 R >>\n"
       << "startxref\n" << xrefPos << "\n%%EOF\n";
}

// ── 공개 API ─────────────────────────────────────────────────

bool PdfWriter::WriteSingle(const CString& srcPath, const CString& dstPath,
                            CString& outError)
{
    ImageData img;
    if (!LoadImage(srcPath, img, outError)) return false;

    std::ofstream os((LPCTSTR)dstPath, std::ios::binary);
    if (!os) { outError = _T("출력 파일 생성 실패"); return false; }

    os << "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";

    // 오브젝트 번호를 쓰는 순서에 맞게 배정
    const int OBJ_CATALOG = 1, OBJ_PAGES = 2,
              OBJ_IMAGE = 3, OBJ_CONTENT = 4, OBJ_PAGE = 5;
    std::vector<size_t> xref;

    xref.push_back(StreamPos(os));
    os << OBJ_CATALOG << " 0 obj\n<< /Type /Catalog /Pages "
       << OBJ_PAGES << " 0 R >>\nendobj\n";

    xref.push_back(StreamPos(os));
    os << OBJ_PAGES << " 0 obj\n"
       << "<< /Type /Pages /Kids [" << OBJ_PAGE << " 0 R] /Count 1 >>\n"
       << "endobj\n";

    WriteImageObj(os, img, OBJ_IMAGE, xref);
    WriteContentObj(os, img.width, img.height, OBJ_CONTENT, xref);
    WritePageObj(os, OBJ_PAGES, OBJ_IMAGE, OBJ_CONTENT,
                 img.width, img.height, OBJ_PAGE, xref);
    WriteXrefTrailer(os, xref, OBJ_CATALOG, OBJ_PAGES, 5);

    if (!os.good()) { outError = _T("PDF 쓰기 오류"); return false; }
    return true;
}

bool PdfWriter::WriteMerged(const std::vector<CString>& srcPaths,
                            const CString& dstPath, CString& outError)
{
    if (srcPaths.empty()) { outError = _T("파일 목록 없음"); return false; }

    std::vector<ImageData> images;
    images.reserve(srcPaths.size());
    for (const auto& p : srcPaths)
    {
        ImageData img;
        if (!LoadImage(p, img, outError)) return false;
        images.push_back(std::move(img));
    }
    return WriteMergedFromData(images, dstPath, outError);
}

bool PdfWriter::WriteMergedFromData(const std::vector<ImageData>& images,
                                    const CString& dstPath, CString& outError)
{
    int n = (int)images.size();
    if (n == 0) { outError = _T("이미지 데이터 없음"); return false; }

    std::ofstream os((LPCTSTR)dstPath, std::ios::binary);
    if (!os) { outError = _T("출력 파일 생성 실패"); return false; }

    os << "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";

    // 오브젝트 레이아웃 (n페이지):
    // 1=Catalog, 2=Pages
    // 페이지 i: imgObj=3+i*3, contentObj=4+i*3, pageObj=5+i*3
    const int OBJ_CATALOG = 1, OBJ_PAGES = 2;
    std::vector<size_t> xref;

    // 1: Catalog
    xref.push_back(StreamPos(os));
    os << OBJ_CATALOG << " 0 obj\n<< /Type /Catalog /Pages "
       << OBJ_PAGES << " 0 R >>\nendobj\n";

    // 2: Pages — Kids 목록을 미리 구성 (PDF는 선형 순서로 작성)
    std::string kidsStr;
    for (int i = 0; i < n; ++i)
        kidsStr += std::to_string(5 + i * 3) + " 0 R ";

    xref.push_back(StreamPos(os));
    os << OBJ_PAGES << " 0 obj\n"
       << "<< /Type /Pages /Kids [" << kidsStr << "] /Count " << n << " >>\n"
       << "endobj\n";

    // 각 페이지별 Image / Content / Page 오브젝트
    for (int i = 0; i < n; ++i)
    {
        int imgObj     = 3 + i * 3;
        int contentObj = 4 + i * 3;
        int pageObj    = 5 + i * 3;

        WriteImageObj  (os, images[i], imgObj, xref);
        WriteContentObj(os, images[i].width, images[i].height, contentObj, xref);
        WritePageObj   (os, OBJ_PAGES, imgObj, contentObj,
                        images[i].width, images[i].height, pageObj, xref);
    }

    int totalObjs = 2 + n * 3;
    WriteXrefTrailer(os, xref, OBJ_CATALOG, OBJ_PAGES, totalObjs);

    if (!os.good()) { outError = _T("PDF 쓰기 오류"); return false; }
    return true;
}
