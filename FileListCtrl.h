#pragma once
#include "pch.h"

// ── 파일 1개의 상태 ──────────────────────────────────────────
struct FileEntry
{
    CString srcPath;   // 원본 전체 경로
    CString srcName;   // 파일명만
    CString pdfName;   // 변환될 PDF 파일명

    enum class Status { Wait, Running, Success, Fail } status = Status::Wait;
    CString remark;   // 비고: 성공 시 "성공", 실패 시 실패 원인

    enum class Type { Image, PdfPage } type = Type::Image;
    int pageIndex = 0;   // 0-based (PdfPage 타입에서 사용)
    int pageTotal = 0;   // 전체 페이지 수
};

// ── 커스텀 CListCtrl ─────────────────────────────────────────
class CFileListCtrl : public CListCtrl
{
public:
    CFileListCtrl() = default;

    void SetupColumns();
    void AddEntry(const FileEntry& entry);
    bool HasEntry(const CString& srcPath) const;              // Image 타입 중복 체크
    bool HasEntry(const CString& srcPath, int pageIndex) const; // PdfPage 타입 중복 체크
    void SetStatus(int index, FileEntry::Status status);
    void SetMergeMode(bool bMerge);
    void RemoveSelected();
    void Clear();
    bool MoveUp(int idx);    // 선택 항목 위로 이동, 성공 시 true
    bool MoveDown(int idx);  // 선택 항목 아래로 이동, 성공 시 true
    void SetRowColorsEnabled(bool bEnable);

    const std::vector<FileEntry>& GetEntries() const { return m_entries; }

protected:
    afx_msg void OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnNMCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    void BuildImageList();
    void AdjustColumnWidths(int cx);
    void RefreshRow(int idx);  // 행 텍스트/아이콘 재표시

    CImageList             m_imageList;
    std::vector<FileEntry> m_entries;
    bool                   m_bMerge       = false;
    bool                   m_bRowColors   = false;

    DECLARE_MESSAGE_MAP()
};
