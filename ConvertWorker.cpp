#include "pch.h"
#include "ConvertWorker.h"
#include "PdfWriter.h"
#include "PdfConverter.h"
#include "resource.h"

ConvertWorker::~ConvertWorker() { Stop(); }

void ConvertWorker::Stop()
{
    if (m_coordinator.joinable())
        m_coordinator.join();
    m_running = false;
}

void ConvertWorker::Start(ConvertTask task)
{
    Stop();
    m_running = true;
    m_coordinator = std::thread(&ConvertWorker::RunCoordinator, this,
                                std::move(task));
}

void ConvertWorker::RunCoordinator(ConvertTask task)
{
    int n = (int)task.entries.size();
    if (n == 0) { ::PostMessage(task.hNotify, WM_CONVERT_DONE, 0, 0); m_running = false; return; }

    unsigned hw      = std::thread::hardware_concurrency();
    int threadCount  = (int)max(1u, min(hw, (unsigned)n));

    std::atomic<int> success{ 0 }, fail{ 0 };

    if (task.bMerge) RunMerge(task, threadCount, success, fail);
    else             RunIndividual(task, threadCount, success, fail);

    ::PostMessage(task.hNotify, WM_CONVERT_DONE,
                  (WPARAM)success.load(), (LPARAM)fail.load());
    m_running = false;
}

// ── 개별 변환 모드 ────────────────────────────────────────────

void ConvertWorker::RunIndividual(const ConvertTask& task,
                                  int threadCount,
                                  std::atomic<int>& success,
                                  std::atomic<int>& fail)
{
    int n = (int)task.entries.size();
    std::atomic<int> nextIdx{ 0 };

    auto workerFn = [&]()
    {
        while (true)
        {
            if (*task.pStop) break;

            int idx = nextIdx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= n) break;

            auto* e = task.entries[idx];

            ::PostMessage(task.hNotify, WM_CONVERT_PROGRESS,
                          (WPARAM)idx, (LPARAM)ConvertStatus::Running);

            CString dir = task.outDir.IsEmpty()
                ? e->srcPath.Left(e->srcPath.ReverseFind(_T('\\')))
                : task.outDir;
            CString dst = dir + _T("\\") + e->pdfName;

            CString errMsg;
            bool ok = false;
            if (e->type == FileEntry::Type::Image)
                ok = PdfWriter::WriteSingle(e->srcPath, dst, errMsg, task.jpegQuality);
            else  // PdfPage
                ok = PdfConverter::RenderPageToJpg(
                         e->srcPath, e->pageIndex, dst, errMsg);

            // remark 기록 후 PostMessage (순서 보장)
            e->remark = ok ? CString(_T("")) : errMsg;
            ok ? ++success : ++fail;

            ::PostMessage(task.hNotify, WM_CONVERT_PROGRESS,
                          (WPARAM)idx,
                          ok ? (LPARAM)ConvertStatus::Success
                             : (LPARAM)ConvertStatus::Fail);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(threadCount);
    for (int t = 0; t < threadCount; ++t)
        workers.emplace_back(workerFn);
    for (auto& w : workers) w.join();
}

// ── 합치기 모드 ───────────────────────────────────────────────

void ConvertWorker::RunMerge(const ConvertTask& task,
                             int threadCount,
                             std::atomic<int>& success,
                             std::atomic<int>& fail)
{
    int n = (int)task.entries.size();

    // PdfPage 항목이 섞이면 합치기 불가 → 전체 Fail
    for (auto* e : task.entries)
    {
        if (e->type == FileEntry::Type::PdfPage)
        {
            for (int i = 0; i < n; ++i)
            {
                task.entries[i]->remark = _T("합치기 모드는 이미지 파일만 지원합니다");
                ::PostMessage(task.hNotify, WM_CONVERT_PROGRESS,
                              (WPARAM)i, (LPARAM)ConvertStatus::Fail);
            }
            ++fail;
            return;
        }
    }

    std::vector<PdfWriter::ImageData> images(n);
    std::vector<bool>                 loadOk(n, false);
    std::vector<CString>              loadErr(n);

    std::atomic<int> nextIdx{ 0 };

    // Phase 1: 병렬 로딩
    auto loadFn = [&]()
    {
        while (true)
        {
            if (*task.pStop) break;
            int idx = nextIdx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= n) break;

            auto* e = task.entries[idx];
            ::PostMessage(task.hNotify, WM_CONVERT_PROGRESS,
                          (WPARAM)idx, (LPARAM)ConvertStatus::Running);

            CString err;
            loadOk[idx]  = PdfWriter::LoadImage(e->srcPath, images[idx], err, task.jpegQuality);
            loadErr[idx] = err;
        }
    };

    {
        std::vector<std::thread> loaders;
        loaders.reserve(threadCount);
        for (int t = 0; t < threadCount; ++t) loaders.emplace_back(loadFn);
        for (auto& l : loaders) l.join();
    }

    if (*task.pStop)
    {
        for (int i = 0; i < n; ++i)
            if (nextIdx.load() > i)
                ::PostMessage(task.hNotify, WM_CONVERT_PROGRESS,
                              (WPARAM)i, (LPARAM)ConvertStatus::Fail);
        ++fail;
        return;
    }

    // 로딩 실패 항목 처리
    bool anyFail = false;
    for (int i = 0; i < n; ++i)
    {
        if (!loadOk[i])
        {
            task.entries[i]->remark = loadErr[i];
            ::PostMessage(task.hNotify, WM_CONVERT_PROGRESS,
                          (WPARAM)i, (LPARAM)ConvertStatus::Fail);
            anyFail = true;
        }
    }
    if (anyFail) { ++fail; return; }

    // Phase 2: 단일 스레드 PDF 쓰기
    CString firstPath = task.entries[0]->srcPath;
    CString dir       = task.outDir.IsEmpty()
        ? firstPath.Left(firstPath.ReverseFind(_T('\\')))
        : task.outDir;
    CString dstPath   = dir + _T("\\") + task.entries[0]->pdfName;

    CString errMsg;
    bool ok = PdfWriter::WriteMergedFromData(images, dstPath, errMsg);
    ok ? ++success : ++fail;

    // 전체 항목 성공/실패 remark 기록 후 PostMessage
    for (int i = 0; i < n; ++i)
    {
        task.entries[i]->remark = ok ? CString(_T("")) : errMsg;
        ::PostMessage(task.hNotify, WM_CONVERT_PROGRESS,
                      (WPARAM)i,
                      ok ? (LPARAM)ConvertStatus::Success
                         : (LPARAM)ConvertStatus::Fail);
    }
}
