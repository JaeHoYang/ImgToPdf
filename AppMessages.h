#pragma once

/**
 * @file AppMessages.h
 * @brief 워커 스레드 → 다이얼로그 간 커스텀 WM_USER 메시지 상수 정의.
 *
 * resource.h 에 넣지 않는다 — VS 리소스 에디터가 비리소스 #define 을 자동 삭제한다.
 */

/** @brief 이미지 변환 항목 진행 상황. wParam=항목인덱스, lParam=ConvertStatus */
#define WM_CONVERT_PROGRESS             (WM_USER + 1)

/** @brief 이미지 변환 전체 완료. wParam=성공수, lParam=실패수 */
#define WM_CONVERT_DONE                 (WM_USER + 2)

/** @brief FileListCtrl::RemoveSelected() erase 완료 후 부모에게 통보. 합치기 상태 재계산 트리거. */
#define WM_LIST_ENTRIES_CHANGED         (WM_USER + 3)

/** @brief PDF 나누기/합치기/추출 작업 완료. wParam=성공여부(1/0) */
#define WM_PDF_TOOL_DONE                (WM_USER + 11)

/** @brief 탭이 호스트에게 버튼 상태 재계산을 요청. */
#define WM_TAB_STATE_CHANGED            (WM_USER + 20)

/** @brief 탭이 호스트에게 드롭 파일 라우팅을 요청. lParam=std::vector<CString>* (수신 측이 delete) */
#define WM_ROUTE_DROP                   (WM_USER + 21)

/** @brief MD 변환 전체 완료. wParam=성공여부(1/0) */
#define WM_MD_CONVERT_DONE              (WM_USER + 22)

/** @brief Word 변환 항목 1개 완료. wParam=항목인덱스, lParam=성공(1)/실패(0) */
#define WM_WORD_ITEM_DONE               (WM_USER + 23)

/** @brief Word 전체 변환 완료. wParam=성공수, lParam=실패수 */
#define WM_WORD_CONVERT_DONE            (WM_USER + 24)

/** @brief PPT 변환 항목 1개 완료. wParam=항목인덱스, lParam=성공(1)/실패(0) */
#define WM_PPT_ITEM_DONE                (WM_USER + 25)

/** @brief PPT 전체 변환 완료. wParam=성공수, lParam=실패수 */
#define WM_PPT_CONVERT_DONE             (WM_USER + 26)

/**
 * @brief PDF 드롭 후 백그라운드에서 페이지 수 로드 완료.
 *
 * lParam = new std::vector<std::pair<CString,int>>* (파일경로, 페이지수).
 * 수신 측(OnPdfPagesLoaded)에서 반드시 delete 해야 한다.
 * PostMessage 실패 시 전송 측에서 즉시 delete.
 */
#define WM_PDF_PAGES_LOADED             (WM_USER + 27)
