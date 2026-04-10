#include "pch.h"
#include "PdfConverter.h"

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Data::Pdf;

// ── 비동기 완료 동기 대기 헬퍼 ──────────────────────────────────

// IAsyncOperation<StorageFile*> 동기 대기
static HRESULT SyncWaitFile(
    IAsyncOperation<StorageFile*>* pAsync,
    IStorageFile** ppOut)
{
    HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    auto cb = Callback<IAsyncOperationCompletedHandler<StorageFile*>>(
        [hEv](IAsyncOperation<StorageFile*>*, AsyncStatus) -> HRESULT {
            SetEvent(hEv); return S_OK;
        });
    pAsync->put_Completed(cb.Get());
    WaitForSingleObject(hEv, INFINITE);
    CloseHandle(hEv);
    // GetResults는 IPdfDocument** 가 아닌 IStorageFile** 를 반환
    return pAsync->GetResults(ppOut);
}

// IAsyncOperation<PdfDocument*> 동기 대기
// GetResults의 반환 타입은 IPdfDocument** (인터페이스 포인터)
static HRESULT SyncWaitPdfDoc(
    IAsyncOperation<PdfDocument*>* pAsync,
    IPdfDocument** ppOut)
{
    HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    auto cb = Callback<IAsyncOperationCompletedHandler<PdfDocument*>>(
        [hEv](IAsyncOperation<PdfDocument*>*, AsyncStatus) -> HRESULT {
            SetEvent(hEv); return S_OK;
        });
    pAsync->put_Completed(cb.Get());
    WaitForSingleObject(hEv, INFINITE);
    CloseHandle(hEv);
    return pAsync->GetResults(ppOut);
}

// IAsyncAction 동기 대기
static HRESULT SyncWaitAction(IAsyncAction* pAsync)
{
    HANDLE hEv = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    auto cb = Callback<IAsyncActionCompletedHandler>(
        [hEv](IAsyncAction*, AsyncStatus) -> HRESULT {
            SetEvent(hEv); return S_OK;
        });
    pAsync->put_Completed(cb.Get());
    WaitForSingleObject(hEv, INFINITE);
    CloseHandle(hEv);
    return S_OK;
}

// ── JPEG 인코더 CLSID 조회 ────────────────────────────────────
CLSID PdfConverter::GetJpegEncoderClsid()
{
    CLSID clsid = {};
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return clsid;

    std::vector<BYTE> buf(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    Gdiplus::GetImageEncoders(num, size, codecs);
    for (UINT i = 0; i < num; ++i)
        if (wcscmp(codecs[i].MimeType, L"image/jpeg") == 0)
            return codecs[i].Clsid;
    return clsid;
}

// ── 공통: StorageFile 얻기 ────────────────────────────────────
static HRESULT GetStorageFile(const CString& path, IStorageFile** ppFile)
{
    ComPtr<IStorageFileStatics> statics;
    HRESULT hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Storage_StorageFile).Get(),
        IID_PPV_ARGS(&statics));
    if (FAILED(hr)) return hr;

    HString hPath;
    hPath.Set(path.GetString(), path.GetLength());

    ComPtr<IAsyncOperation<StorageFile*>> asyncOp;
    hr = statics->GetFileFromPathAsync(hPath.Get(), &asyncOp);
    if (FAILED(hr)) return hr;

    return SyncWaitFile(asyncOp.Get(), ppFile);
}

// ── 공통: PdfDocument 로딩 ────────────────────────────────────
static HRESULT LoadPdfDocument(IStorageFile* pFile, IPdfDocument** ppDoc)
{
    ComPtr<IPdfDocumentStatics> statics;
    HRESULT hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Data_Pdf_PdfDocument).Get(),
        IID_PPV_ARGS(&statics));
    if (FAILED(hr)) return hr;

    ComPtr<IAsyncOperation<PdfDocument*>> asyncOp;
    hr = statics->LoadFromFileAsync(pFile, &asyncOp);
    if (FAILED(hr)) return hr;

    return SyncWaitPdfDoc(asyncOp.Get(), ppDoc);
}

// ── GetPageCount ─────────────────────────────────────────────
int PdfConverter::GetPageCount(const CString& pdfPath)
{
    RoInitialize(RO_INIT_MULTITHREADED);

    int count = 0;
    do
    {
        ComPtr<IStorageFile> file;
        if (FAILED(GetStorageFile(pdfPath, &file))) break;

        ComPtr<IPdfDocument> doc;
        if (FAILED(LoadPdfDocument(file.Get(), &doc))) break;

        UINT32 pageCount = 0;
        if (FAILED(doc->get_PageCount(&pageCount))) break;
        count = (int)pageCount;
    } while (false);

    RoUninitialize();
    return count;
}

// ── RenderPageToJpg ──────────────────────────────────────────
bool PdfConverter::RenderPageToJpg(const CString& pdfPath,
                                   int            pageIndex,
                                   const CString& dstPath,
                                   CString&       errMsg,
                                   int            quality)
{
    RoInitialize(RO_INIT_MULTITHREADED);
    bool ok = false;

    do
    {
        // 1) StorageFile 얻기
        ComPtr<IStorageFile> file;
        if (FAILED(GetStorageFile(pdfPath, &file)))
        {
            errMsg = _T("PDF 파일 열기 실패");
            break;
        }

        // 2) PdfDocument 로딩
        ComPtr<IPdfDocument> doc;
        if (FAILED(LoadPdfDocument(file.Get(), &doc)))
        {
            errMsg = _T("PDF 파싱 실패");
            break;
        }

        // 3) 페이지 얻기
        ComPtr<IPdfPage> page;
        if (FAILED(doc->GetPage((UINT32)pageIndex, &page)))
        {
            errMsg = _T("페이지 접근 실패");
            break;
        }

        // 4) InMemoryRandomAccessStream을 IRandomAccessStream으로 활성화
        ComPtr<IRandomAccessStream> rasStream;
        HRESULT hr = RoActivateInstance(
            HStringReference(
                RuntimeClass_Windows_Storage_Streams_InMemoryRandomAccessStream
            ).Get(),
            reinterpret_cast<IInspectable**>(rasStream.GetAddressOf()));
        if (FAILED(hr))
        {
            errMsg = _T("스트림 생성 실패");
            break;
        }

        // 5) RenderToStreamAsync
        ComPtr<IAsyncAction> asyncAction;
        hr = page->RenderToStreamAsync(rasStream.Get(), &asyncAction);
        if (FAILED(hr))
        {
            errMsg = _T("렌더링 실패");
            break;
        }
        SyncWaitAction(asyncAction.Get());

        // 6) IRandomAccessStream → IStream (COM interop)
        ComPtr<IStream> comStream;
        hr = CreateStreamOverRandomAccessStream(
            rasStream.Get(), IID_PPV_ARGS(&comStream));
        if (FAILED(hr))
        {
            errMsg = _T("스트림 변환 실패");
            break;
        }

        // 스트림 시작으로 이동
        LARGE_INTEGER liZero = {};
        comStream->Seek(liZero, STREAM_SEEK_SET, nullptr);

        // 7) GDI+ Bitmap 생성
        std::unique_ptr<Gdiplus::Bitmap> bmp(
            Gdiplus::Bitmap::FromStream(comStream.Get()));
        if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok)
        {
            errMsg = _T("이미지 변환 실패");
            break;
        }

        // 8) JPEG 저장
        CLSID jpegClsid = GetJpegEncoderClsid();
        Gdiplus::EncoderParameters encParams;
        encParams.Count = 1;
        encParams.Parameter[0].Guid           = Gdiplus::EncoderQuality;
        encParams.Parameter[0].Type           = Gdiplus::EncoderParameterValueTypeLong;
        encParams.Parameter[0].NumberOfValues = 1;
        ULONG q = (ULONG)quality;
        encParams.Parameter[0].Value          = &q;

        Gdiplus::Status st = bmp->Save(dstPath.GetString(), &jpegClsid, &encParams);
        if (st != Gdiplus::Ok)
        {
            errMsg = _T("JPG 저장 실패");
            break;
        }

        ok = true;
    } while (false);

    RoUninitialize();
    return ok;
}
