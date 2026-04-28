#pragma once
#include "pch.h"
#include "FileListCtrl.h"

/**
 * @brief WM_CONVERT_PROGRESS 메시지의 lParam 값으로 사용되는 변환 상태 코드.
 */
enum class ConvertStatus : LPARAM
{
    Success = 0, ///< 변환 성공
    Fail    = 1, ///< 변환 실패
    Running = 2, ///< 변환 진행 중
};

/**
 * @brief ConvertWorker::Start() 에 전달하는 변환 작업 명세.
 *
 * entries 는 m_listFiles.m_entries 벡터 내 포인터 목록이므로
 * 변환 중에는 벡터가 재할당되지 않도록 DragAcceptFiles(false) 로 보호해야 한다.
 */
struct ConvertTask
{
    std::vector<FileEntry*> entries;        ///< 변환할 항목 포인터 목록 (벡터 소유권 없음)
    bool                    bMerge;         ///< true: 모든 이미지를 단일 PDF로 합치기
    HWND                    hNotify;        ///< WM_CONVERT_PROGRESS / WM_CONVERT_DONE 수신 HWND
    std::atomic<bool>*      pStop;          ///< 중단 요청 플래그 포인터
    CString                 outDir;         ///< 출력 폴더 경로 (비어있으면 원본 파일 폴더)
    int                     jpegQuality = 90; ///< 비JPEG 이미지 PDF 임베드 품질 (10~100)
};

/**
 * @brief 이미지 ↔ PDF 변환을 멀티스레드로 수행하는 워커.
 *
 * Start() 로 코디네이터 스레드를 생성하고, 그 안에서 hardware_concurrency()
 * 개의 워커 스레드를 운용한다.
 *
 * - **개별 모드**: std::atomic<int> nextIdx 로 lock-free 파일 분배
 * - **합치기 모드**: Phase1(병렬 로딩) → Phase2(단일 스레드 PDF 쓰기)
 *
 * UI 스레드와의 통신은 PostMessage 만 사용하며, FileEntry::remark 는
 * PostMessage 전 워커에서 쓰고 UI 수신 후 읽어 메모리 오더를 보장한다.
 */
class ConvertWorker
{
public:
    ConvertWorker() = default;

    /**
     * @brief 소멸자. 실행 중인 스레드가 있으면 완료될 때까지 대기한다.
     */
    ~ConvertWorker();

    /**
     * @brief 변환 작업을 비동기로 시작한다.
     *
     * 이미 실행 중이면 무시된다. 코디네이터 스레드를 새로 생성하며,
     * 스레드는 완료 후 자동으로 정리된다.
     *
     * @param task 변환 명세 (값으로 복사되어 스레드에 전달됨)
     */
    void Start(ConvertTask task);

    /**
     * @brief 변환 중단을 요청한다.
     *
     * task.pStop 플래그를 true 로 설정하여 워커 스레드들이 다음 파일부터
     * 처리를 건너뛰도록 한다. 즉시 종료를 보장하지 않는다.
     */
    void Stop();

    /**
     * @brief 현재 변환 중 여부를 반환한다.
     * @return 변환 스레드가 실행 중이면 true
     */
    bool IsRunning() const { return m_running.load(); }

private:
    /**
     * @brief 코디네이터 스레드 진입점.
     *
     * 스레드풀을 구성하고 모든 워커 완료를 기다린 뒤
     * WM_CONVERT_DONE 을 호스트에 PostMessage 한다.
     *
     * @param task 변환 명세 (값 복사로 소유)
     */
    void RunCoordinator(ConvertTask task);

    /**
     * @brief 개별 변환 모드 실행.
     *
     * 각 워커 스레드가 원자적 인덱스 nextIdx 로 파일을 분배받아
     * 독립적으로 PDF를 생성한다.
     *
     * @param task        변환 명세
     * @param threadCount 워커 스레드 수
     * @param success     성공 카운터 (원자적)
     * @param fail        실패 카운터 (원자적)
     */
    void RunIndividual(const ConvertTask& task,
                       int threadCount,
                       std::atomic<int>& success,
                       std::atomic<int>& fail);

    /**
     * @brief 합치기 모드 실행.
     *
     * Phase1: 병렬로 모든 이미지를 로딩한다.
     * Phase2: 단일 스레드에서 로딩된 데이터로 다중페이지 PDF를 생성한다.
     *
     * @param task        변환 명세
     * @param threadCount Phase1 병렬 워커 수
     * @param success     성공 카운터 (원자적)
     * @param fail        실패 카운터 (원자적)
     */
    void RunMerge(const ConvertTask& task,
                  int threadCount,
                  std::atomic<int>& success,
                  std::atomic<int>& fail);

    std::thread       m_coordinator;        ///< 코디네이터 스레드
    std::atomic<bool> m_running{ false };   ///< 변환 실행 중 플래그
};
