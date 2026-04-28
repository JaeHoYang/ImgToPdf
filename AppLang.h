#pragma once
#include "pch.h"
#include <vector>

/**
 * @brief 앱 표시 언어 열거형.
 *
 * g_lang 전역 변수가 현재 언어를 나타내며, LS() 헬퍼가 이를 참조한다.
 */
enum class Lang { KO, EN };

/** @brief 현재 선택된 표시 언어. SetLang() / LoadSavedLang() 으로 변경. */
extern Lang g_lang;

/**
 * @brief 앱 전체 문자열 ID 열거형.
 *
 * s_ko[] / s_en[] 배열과 동일한 인덱스 순서를 유지해야 한다.
 * 새 문자열 추가 시 enum, s_ko[], s_en[] 세 곳을 동시에 같은 순서로 수정.
 */
enum AppStringId : UINT
{
    // Common / Host (0-18)
    IDS_TAB1_LABEL = 0,    ///< 탭1 레이블 (이미지 변환)
    IDS_TAB2_LABEL,        ///< 탭2 레이블 (PDF 도구)
    IDS_TAB3_LABEL,        ///< 탭3 레이블 (MD 변환)
    IDS_MENU_HELP,         ///< 시스템 메뉴 도움말 항목
    IDS_TIP_EDITPATH,      ///< 경로 에디트박스 툴팁
    IDS_TIP_MERGE,         ///< 합치기 체크박스 툴팁
    IDS_TIP_BROWSE,        ///< 찾아보기 버튼 툴팁
    IDS_TIP_CONVERT,       ///< 변환 버튼 툴팁
    IDS_TIP_PROGRESS,      ///< 진행 카운터 툴팁
    IDS_TIP_MOVEUP,        ///< ▲ 이동 버튼 툴팁
    IDS_TIP_MOVEDOWN,      ///< ▼ 이동 버튼 툴팁
    IDS_TIP_CLEAR_BASE,    ///< 삭제 버튼 기본 툴팁
    IDS_CUE_PATH,          ///< 경로 에디트박스 플레이스홀더
    IDS_HELP_TEXT,         ///< 도움말 대화상자 본문
    IDS_HELP_CAPTION,      ///< 도움말 대화상자 제목
    IDS_PROGRESS_DONE,     ///< 진행 완료 문자열
    IDS_PROGRESS_ERROR,    ///< 진행 오류 문자열
    IDS_FOLDER_SELECT,     ///< SHBrowseForFolder 제목
    IDS_FILTER_ALL_LABEL,  ///< 파일 필터 "모든 파일" 레이블

    // Tab1 - ImgConvert (19-32)
    IDS_IMG_TIP_LIST = 19, ///< 이미지 목록 툴팁
    IDS_IMG_TIP_PREVIEW,   ///< 이미지 미리보기 툴팁
    IDS_IMG_TIP_OUTPUT,    ///< 출력 경로 에디트 툴팁
    IDS_IMG_CUE_OUTPUT,    ///< 출력 경로 에디트 플레이스홀더
    IDS_IMG_FILTER_LABEL,  ///< 이미지 파일 필터 레이블
    IDS_IMG_ERR_OUTDIR,    ///< 출력 폴더 오류 메시지
    IDS_IMG_ERR_TITLE,     ///< 이미지 변환 오류 제목
    IDS_IMG_CONFIRM_RESET, ///< 초기화 확인 메시지
    IDS_IMG_CONFIRM_TITLE, ///< 초기화 확인 제목
    IDS_IMG_BTN_STOP,      ///< 중단 버튼 레이블
    IDS_IMG_BTN_CONVERT,   ///< 변환 버튼 레이블
    IDS_IMG_CLEAR_DONE,    ///< 삭제 버튼 툴팁 (변환 완료 후)
    IDS_IMG_CLEAR_SEL,     ///< 삭제 버튼 툴팁 (항목 선택 시)
    IDS_IMG_CLEAR_NONE,    ///< 삭제 버튼 툴팁 (선택 없음)

    // Tab2 - PdfTools (33-64)
    IDS_PDF_COL0 = 33,         ///< PDF 목록 컬럼0 (파일명)
    IDS_PDF_COL1,              ///< PDF 목록 컬럼1 (페이지 수)
    IDS_PDF_COL2,              ///< PDF 목록 컬럼2 (비고)
    IDS_PDF_COL3,              ///< PDF 목록 컬럼3
    IDS_PDF_CUE_PATH,          ///< PDF 탭 경로 에디트 플레이스홀더
    IDS_PDF_FILTER_LABEL,      ///< PDF 파일 필터 레이블
    IDS_PDF_TIP_LIST,          ///< PDF 목록 툴팁
    IDS_PDF_TIP_SPLIT,         ///< 나누기 라디오 툴팁
    IDS_PDF_TIP_MERGE_OP,      ///< 합치기 라디오 툴팁
    IDS_PDF_TIP_EXTRACT,       ///< 추출 라디오 툴팁
    IDS_PDF_TIP_SPLIT_EACH,    ///< 페이지별 나누기 라디오 툴팁
    IDS_PDF_TIP_SPLIT_RANGE,   ///< 범위 나누기 라디오 툴팁
    IDS_PDF_TIP_RANGE_EDIT,    ///< 범위 입력 에디트 툴팁
    IDS_PDF_TIP_MERGENAME,     ///< 합치기 파일명 에디트 툴팁
    IDS_PDF_TIP_EXTRACT_PAGES, ///< 추출 페이지 에디트 툴팁
    IDS_PDF_TIP_EXT_SINGLE,    ///< 단일 파일 추출 라디오 툴팁
    IDS_PDF_TIP_EXT_EACH,      ///< 페이지별 추출 라디오 툴팁
    IDS_PDF_TIP_OUTPUT,        ///< PDF 출력 경로 툴팁
    IDS_PDF_TIP_BROWSE,        ///< PDF 탐색 버튼 툴팁
    IDS_PDF_STATUS_WORKING,    ///< 작업 중 상태 문자열
    IDS_PDF_STATUS_DONE,       ///< 작업 완료 상태 문자열
    IDS_PDF_STATUS_FAIL,       ///< 작업 실패 상태 문자열
    IDS_PDF_STATUS_ERROR,      ///< 작업 오류 상태 문자열
    IDS_PDF_STATUS_RANGE_ERR,  ///< 범위 오류 상태 문자열
    IDS_PDF_BTN_RUN,           ///< 실행 버튼 레이블
    IDS_PDF_BTN_RUNNING,       ///< 실행 중 버튼 레이블
    IDS_PDF_MENU_REMOVE,       ///< 우클릭 메뉴 - 항목 제거
    IDS_PDF_MENU_OPEN_LOC,     ///< 우클릭 메뉴 - 위치 열기
    IDS_PDF_MENU_OPEN,         ///< 우클릭 메뉴 - 파일 열기
    IDS_PDF_MENU_CLEAR_ALL,    ///< 우클릭 메뉴 - 전체 지우기
    IDS_PDF_REMARK_DONE,       ///< 목록 비고 - 완료
    IDS_PDF_REMARK_FAIL,       ///< 목록 비고 - 실패

    // Tab3 - MdConvert (65-101)
    IDS_MD_COL0 = 65,          ///< MD 목록 컬럼0 (파일명)
    IDS_MD_COL1,               ///< MD 목록 컬럼1 (형식)
    IDS_MD_COL2,               ///< MD 목록 컬럼2 (상태)
    IDS_MD_CUE_PATH,           ///< MD 탭 경로 에디트 플레이스홀더
    IDS_MD_FILTER_LABEL,       ///< MD 파일 필터 레이블
    IDS_MD_TIP_LIST,           ///< MD 목록 툴팁
    IDS_MD_TIP_PREVIEW,        ///< MD 미리보기 툴팁
    IDS_MD_TIP_HTML,           ///< HTML 라디오 툴팁
    IDS_MD_TIP_PDF,            ///< PDF 라디오 툴팁
    IDS_MD_TIP_HTMLPDF,        ///< HTML+PDF 라디오 툴팁
    IDS_MD_TIP_OUTPUT,         ///< MD 출력 경로 툴팁
    IDS_MD_TIP_BROWSE,         ///< MD 탐색 버튼 툴팁
    IDS_MD_ERR_OUTDIR,         ///< MD 출력 폴더 오류 메시지
    IDS_MD_ERR_TITLE,          ///< MD 오류 제목
    IDS_MD_CONFIRM_RESET,      ///< MD 초기화 확인 메시지
    IDS_MD_CONFIRM_TITLE,      ///< MD 초기화 확인 제목
    IDS_MD_STATUS_RENDERING,   ///< 렌더링 중 상태
    IDS_MD_STATUS_CONVERTING,  ///< 변환 중 상태
    IDS_MD_STATUS_WAIT,        ///< 대기 상태
    IDS_MD_STATUS_RENDERING_ITEM,  ///< 항목 렌더링 중 상태
    IDS_MD_STATUS_CONVERTING_ITEM, ///< 항목 변환 중 상태
    IDS_MD_STATUS_DONE,        ///< 변환 완료 상태
    IDS_MD_STATUS_FAIL,        ///< 변환 실패 상태
    IDS_MD_STATUS_PARTIAL_FAIL,///< 일부 실패 상태
    IDS_MD_STATUS_DONE_HTML_PDF, ///< HTML+PDF 완료 상태
    IDS_MD_STATUS_DONE_PDF,    ///< PDF 완료 상태
    IDS_MD_STATUS_DONE_HTML,   ///< HTML 완료 상태
    IDS_MD_PREVIEW_INIT,       ///< 미리보기 초기 안내
    IDS_MD_PREVIEW_ERR,        ///< 미리보기 오류
    IDS_MD_BTN_CONVERT,        ///< MD 변환 버튼 레이블
    IDS_MD_CLEAR_DONE,         ///< MD 삭제 버튼 툴팁 (완료 후)
    IDS_MD_CLEAR_SEL,          ///< MD 삭제 버튼 툴팁 (선택 시)
    IDS_MD_CLEAR_NONE,         ///< MD 삭제 버튼 툴팁 (선택 없음)
    IDS_MD_MENU_REMOVE,        ///< 우클릭 메뉴 - 항목 제거
    IDS_MD_MENU_OPEN_LOC,      ///< 우클릭 메뉴 - 위치 열기
    IDS_MD_MENU_OPEN_NOTEPAD,  ///< 우클릭 메뉴 - 메모장으로 열기
    IDS_MD_MENU_CLEAR_ALL,     ///< 우클릭 메뉴 - 전체 지우기

    // FileListCtrl (102-112)
    IDS_FLC_COL1 = 102,        ///< FileListCtrl 컬럼1
    IDS_FLC_COL2,              ///< FileListCtrl 컬럼2
    IDS_FLC_COL3,              ///< FileListCtrl 컬럼3
    IDS_FLC_STATUS_SUCCESS,    ///< 성공 상태 텍스트
    IDS_FLC_STATUS_FAIL,       ///< 실패 상태 텍스트
    IDS_FLC_STATUS_RUNNING,    ///< 진행 중 상태 텍스트
    IDS_FLC_SUFFIX_MERGED,     ///< 합치기 출력 파일명 접미사
    IDS_FLC_MENU_REMOVE,       ///< 우클릭 메뉴 - 항목 제거
    IDS_FLC_MENU_OPEN_LOC,     ///< 우클릭 메뉴 - 위치 열기
    IDS_FLC_MENU_OPEN,         ///< 우클릭 메뉴 - 파일 열기
    IDS_FLC_MENU_CLEAR_ALL,    ///< 우클릭 메뉴 - 전체 지우기

    // Quality slider (113-114)
    IDS_IMG_QUALITY_FMT,       ///< 품질 슬라이더 포맷 문자열
    IDS_IMG_TIP_QUALITY,       ///< 품질 슬라이더 툴팁

    // PDF thumbnail (115)
    IDS_PDF_TIP_THUMB,         ///< PDF 썸네일 툴팁

    // Tab4 - WordConvert (116-139)
    IDS_TAB4_LABEL = 116,      ///< 탭4 레이블 (Word 변환)
    IDS_WORD_COL0,             ///< Word 목록 컬럼0 (파일명)
    IDS_WORD_COL1,             ///< Word 목록 컬럼1 (엔진)
    IDS_WORD_COL2,             ///< Word 목록 컬럼2 (상태)
    IDS_WORD_CUE_PATH,         ///< Word 탭 경로 에디트 플레이스홀더
    IDS_WORD_FILTER_LABEL,     ///< Word 파일 필터 레이블
    IDS_WORD_TIP_LIST,         ///< Word 목록 툴팁
    IDS_WORD_TIP_OUTPUT,       ///< Word 출력 경로 툴팁
    IDS_WORD_TIP_BROWSE,       ///< Word 탐색 버튼 툴팁
    IDS_WORD_ERR_NO_ENGINE,    ///< Word/LibreOffice 모두 미설치 오류
    IDS_WORD_ERR_OUTDIR,       ///< Word 출력 폴더 오류
    IDS_WORD_ERR_TITLE,        ///< Word 오류 대화상자 제목
    IDS_WORD_STATUS_WORKING,   ///< Word 변환 중 상태
    IDS_WORD_STATUS_DONE,      ///< Word 변환 완료 상태
    IDS_WORD_STATUS_FAIL,      ///< Word 변환 실패 상태
    IDS_WORD_BTN_CONVERT,      ///< Word 변환 버튼 레이블
    IDS_WORD_MENU_REMOVE,      ///< 우클릭 메뉴 - 항목 제거
    IDS_WORD_MENU_OPEN_LOC,    ///< 우클릭 메뉴 - 위치 열기
    IDS_WORD_MENU_OPEN,        ///< 우클릭 메뉴 - 파일 열기
    IDS_WORD_MENU_CLEAR_ALL,   ///< 우클릭 메뉴 - 전체 지우기
    IDS_WORD_CONFIRM_RESET,    ///< Word 초기화 확인 메시지
    IDS_WORD_CONFIRM_TITLE,    ///< Word 초기화 확인 제목
    IDS_WORD_ENGINE_WORD,      ///< 엔진 표시 - Microsoft Word
    IDS_WORD_ENGINE_LIBRE,     ///< 엔진 표시 - LibreOffice

    // Tab5 - PptConvert (140-162)
    IDS_TAB5_LABEL = 140,      ///< 탭5 레이블 (PPT 변환)
    IDS_PPT_COL0,              ///< PPT 목록 컬럼0 (파일명)
    IDS_PPT_COL1,              ///< PPT 목록 컬럼1 (엔진)
    IDS_PPT_COL2,              ///< PPT 목록 컬럼2 (상태)
    IDS_PPT_CUE_PATH,          ///< PPT 탭 경로 에디트 플레이스홀더
    IDS_PPT_TIP_LIST,          ///< PPT 목록 툴팁
    IDS_PPT_TIP_OUTPUT,        ///< PPT 출력 경로 툴팁
    IDS_PPT_TIP_BROWSE,        ///< PPT 탐색 버튼 툴팁
    IDS_PPT_ERR_NO_ENGINE,     ///< PowerPoint/LibreOffice 모두 미설치 오류
    IDS_PPT_ERR_OUTDIR,        ///< PPT 출력 폴더 오류
    IDS_PPT_ERR_TITLE,         ///< PPT 오류 대화상자 제목
    IDS_PPT_STATUS_WORKING,    ///< PPT 변환 중 상태
    IDS_PPT_STATUS_DONE,       ///< PPT 변환 완료 상태
    IDS_PPT_STATUS_FAIL,       ///< PPT 변환 실패 상태
    IDS_PPT_BTN_CONVERT,       ///< PPT 변환 버튼 레이블
    IDS_PPT_MENU_REMOVE,       ///< 우클릭 메뉴 - 항목 제거
    IDS_PPT_MENU_OPEN_LOC,     ///< 우클릭 메뉴 - 위치 열기
    IDS_PPT_MENU_OPEN,         ///< 우클릭 메뉴 - 파일 열기
    IDS_PPT_MENU_CLEAR_ALL,    ///< 우클릭 메뉴 - 전체 지우기
    IDS_PPT_CONFIRM_RESET,     ///< PPT 초기화 확인 메시지
    IDS_PPT_CONFIRM_TITLE,     ///< PPT 초기화 확인 제목
    IDS_PPT_ENGINE_PPT,        ///< 엔진 표시 - Microsoft PowerPoint
    IDS_PPT_ENGINE_LIBRE,      ///< 엔진 표시 - LibreOffice

    IDS_WORD_TIP_PREVIEW,      ///< Word 미리보기 툴팁
    IDS_PPT_TIP_PREVIEW,       ///< PPT 미리보기 툴팁

    IDS_PDF_SUMMARIZE_BTN,     ///< AI 요약 버튼 레이블
    IDS_PDF_SUMMARIZE_WORKING, ///< AI 요약 중 상태 문자열
    IDS_PDF_SUMMARIZE_TIP,     ///< AI 요약 버튼 툴팁
    IDS_PDF_OPEN_NOTEPAD_TIP,  ///< 메모장으로 열기 버튼 툴팁
    IDS_PDF_SUMMARY_TIP,       ///< 요약 결과 에디트 툴팁
    IDS_PDF_SUMMARY_GUIDE,     ///< 요약 결과 에디트 안내 텍스트

    IDS_APP_STRING_COUNT       ///< 문자열 배열 크기 (항상 마지막에 위치)
};

/**
 * @brief 현재 언어로 해당 ID의 문자열을 반환한다.
 * @param id AppStringId 열거값
 * @return 현재 g_lang 에 따른 한국어 또는 영어 문자열
 */
CString LS(AppStringId id);

/**
 * @brief 표시 언어를 변경하고 레지스트리에 저장한다.
 * @param lang 변경할 언어 (Lang::KO 또는 Lang::EN)
 */
void SetLang(Lang lang);

/**
 * @brief 앱 시작 시 레지스트리에서 저장된 언어 설정을 로드한다.
 *
 * OnInitDialog() 최상단에서 호출해야 LS() 가 올바른 언어를 반환한다.
 */
void LoadSavedLang();

/**
 * @brief OPENFILENAME.lpstrFilter 에 사용할 null 구분 필터 문자열을 빌드한다.
 *
 * 형식: "레이블\0패턴\0모든 파일\0*.*\0\0"
 * 반환된 vector 가 살아있는 동안만 lpstrFilter 포인터가 유효하다.
 *
 * @param labelId 필터 레이블 문자열 ID
 * @param pattern 와일드카드 패턴 (예: L"*.jpg;*.png")
 * @return null 구분 TCHAR 배열
 */
std::vector<TCHAR> BuildFilter(AppStringId labelId, LPCTSTR pattern);
