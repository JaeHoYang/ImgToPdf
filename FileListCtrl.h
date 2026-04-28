#pragma once
#include "pch.h"
#include "AppLang.h"

/**
 * @brief 변환 목록의 파일 1개에 대한 상태 정보.
 *
 * CFileListCtrl::m_entries 벡터에 저장되며, 변환 워커가 remark 를
 * PostMessage 전에 기록하고 UI 스레드가 메시지 수신 후 읽는다.
 */
struct FileEntry
{
    CString srcPath;   ///< 원본 파일의 전체 경로
    CString srcName;   ///< 파일명만 (표시용)
    CString pdfName;   ///< 변환될 PDF 파일명

    /** @brief 항목의 변환 진행 상태 열거형. */
    enum class Status { Wait, Running, Success, Fail } status = Status::Wait;

    CString remark;    ///< 비고: 성공 시 "성공", 실패 시 실패 원인 문자열

    /** @brief 항목 종류 — 일반 이미지 또는 PDF 페이지 추출 항목. */
    enum class Type { Image, PdfPage } type = Type::Image;

    int pageIndex = 0;  ///< 0-based 페이지 인덱스 (PdfPage 타입에서만 사용)
    int pageTotal = 0;  ///< 전체 페이지 수 (PdfPage 타입에서만 사용)
};

/**
 * @brief 이미지/PDF 파일 목록을 표시하는 커스텀 CListCtrl.
 *
 * NM_CUSTOMDRAW 로 행 배경색을 칠하고, CImageList 로 상태 아이콘
 * (Wait/Running/Success/Fail)을 표시한다.
 *
 * 컬럼 너비는 OnSize → AdjustColumnWidths 로 자동 관리되며,
 * 세로 스크롤바 너비(SM_CXVSCROLL) 선제 차감으로 가로 스크롤을 방지한다.
 */
class CFileListCtrl : public CListCtrl
{
public:
    CFileListCtrl() = default;

    /**
     * @brief 리스트뷰 컬럼을 초기화한다.
     *
     * 호출 직후 AdjustColumnWidths 가 실행되어 초기 너비가 설정된다.
     */
    void SetupColumns();

    /**
     * @brief 항목을 목록 끝에 추가한다.
     * @param entry 추가할 FileEntry 복사본
     */
    void AddEntry(const FileEntry& entry);

    /**
     * @brief Image 타입 항목의 중복 경로 검사.
     * @param srcPath 검사할 파일 경로
     * @return 이미 같은 경로의 Image 항목이 있으면 true
     */
    bool HasEntry(const CString& srcPath) const;

    /**
     * @brief PdfPage 타입 항목의 중복 검사.
     * @param srcPath   PDF 파일 경로
     * @param pageIndex 0-based 페이지 인덱스
     * @return 이미 같은 PDF + 페이지의 항목이 있으면 true
     */
    bool HasEntry(const CString& srcPath, int pageIndex) const;

    /**
     * @brief 지정 인덱스 항목의 변환 상태를 갱신하고 행을 다시 그린다.
     * @param index  항목 인덱스 (0-based)
     * @param status 새 상태
     */
    void SetStatus(int index, FileEntry::Status status);

    /**
     * @brief 합치기 모드를 설정한다.
     *
     * 합치기 모드에서는 PDF 페이지 항목을 포함할 수 없으므로,
     * PdfPage 항목 유무를 확인한 후에 m_bMerge 를 변경한다.
     * 순서가 반대이면 pdfName 계산이 깨진다.
     *
     * @param bMerge true 이면 합치기 모드
     */
    void SetMergeMode(bool bMerge);

    /**
     * @brief 현재 선택된 항목들을 목록에서 제거하고 부모에 WM_LIST_ENTRIES_CHANGED 를 전송한다.
     *
     * erase 완료 후 PostMessage 하므로 부모 핸들러가 수신할 때 벡터가 확정 상태이다.
     */
    void RemoveSelected();

    /** @brief 모든 항목을 제거하고 컨트롤을 초기화한다. */
    void Clear();

    /**
     * @brief 지정 인덱스 항목을 위로 이동한다.
     * @param idx 이동할 항목 인덱스 (0-based)
     * @return 이동 성공 시 true, 맨 위이면 false
     */
    bool MoveUp(int idx);

    /**
     * @brief 지정 인덱스 항목을 아래로 이동한다.
     * @param idx 이동할 항목 인덱스 (0-based)
     * @return 이동 성공 시 true, 맨 아래이면 false
     */
    bool MoveDown(int idx);

    /**
     * @brief 행 배경색 강조 표시를 활성화/비활성화한다.
     * @param bEnable true 이면 상태별 배경색 표시
     */
    void SetRowColorsEnabled(bool bEnable);

    /**
     * @brief 현재 언어로 컬럼 헤더를 갱신한다.
     *
     * 언어 전환 시 호스트가 호출한다.
     */
    void ApplyLanguage();

    /**
     * @brief 내부 항목 벡터에 대한 읽기 전용 참조를 반환한다.
     * @return FileEntry 벡터 const 참조
     */
    const std::vector<FileEntry>& GetEntries() const { return m_entries; }

    /**
     * @brief 현재 합치기 모드 여부를 반환한다.
     * @return 합치기 모드이면 true
     */
    bool GetMergeMode() const { return m_bMerge; }

protected:
    /** @brief 우클릭 컨텍스트 메뉴를 표시한다. */
    afx_msg void OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief Delete 키로 선택 항목을 제거한다. */
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

    /** @brief 행 배경색과 상태 아이콘을 그린다. */
    afx_msg void OnNMCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    /** @brief 컨트롤 크기 변경 시 컬럼 너비를 재계산한다. */
    afx_msg void OnSize(UINT nType, int cx, int cy);

    /** @brief 상태 아이콘용 CImageList 를 빌드한다. */
    void BuildImageList();

    /**
     * @brief 컨트롤 너비에 맞게 컬럼 너비를 조정한다.
     *
     * 세로 스크롤바 너비(SM_CXVSCROLL) 를 선제 차감하여 가로 스크롤을 방지한다.
     *
     * @param cx 현재 컨트롤 클라이언트 너비
     */
    void AdjustColumnWidths(int cx);

    /**
     * @brief 지정 행의 텍스트와 아이콘을 리스트뷰에 다시 표시한다.
     * @param idx 갱신할 행 인덱스 (0-based)
     */
    void RefreshRow(int idx);

    CImageList             m_imageList;         ///< 상태 아이콘 이미지 리스트 (Wait/Running/Success/Fail)
    std::vector<FileEntry> m_entries;           ///< 항목 데이터 벡터
    bool                   m_bMerge     = false; ///< 합치기 모드 플래그
    bool                   m_bRowColors = false; ///< 행 배경색 강조 활성화 플래그

    DECLARE_MESSAGE_MAP()
};
