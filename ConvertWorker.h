#pragma once
#include "pch.h"
#include "FileListCtrl.h"

// lParam 값으로 사용되는 변환 상태 코드
enum class ConvertStatus : LPARAM
{
    Success = 0,
    Fail    = 1,
    Running = 2,
};

struct ConvertTask
{
    std::vector<FileEntry*> entries;   // 변환할 항목 포인터 목록
    bool                    bMerge;    // 합치기 여부
    HWND                    hNotify;   // 진행 알림 받을 다이얼로그 HWND
    std::atomic<bool>*      pStop;     // 중단 플래그 포인터
};

class ConvertWorker
{
public:
    ConvertWorker() = default;
    ~ConvertWorker();

    void Start(ConvertTask task);
    void Stop();
    bool IsRunning() const { return m_running.load(); }

private:
    // 코디네이터 스레드: 스레드풀 구성 후 완료 대기
    void RunCoordinator(ConvertTask task);

    // 개별 변환 모드: 워커들이 원자적 인덱스로 파일 분배
    void RunIndividual(const ConvertTask& task,
                       int threadCount,
                       std::atomic<int>& success,
                       std::atomic<int>& fail);

    // 합치기 모드: 로딩은 병렬, PDF 쓰기는 단일 스레드
    void RunMerge(const ConvertTask& task,
                  int threadCount,
                  std::atomic<int>& success,
                  std::atomic<int>& fail);

    std::thread       m_coordinator;
    std::atomic<bool> m_running{ false };
};
