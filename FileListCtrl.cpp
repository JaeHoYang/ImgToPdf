#include "pch.h"
#include "resource.h"
#include "FileListCtrl.h"
#include "AppLang.h"

BEGIN_MESSAGE_MAP(CFileListCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(NM_RCLICK,     &CFileListCtrl::OnNMRClick)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CFileListCtrl::OnNMCustomDraw)
    ON_WM_KEYDOWN()
    ON_WM_SIZE()
END_MESSAGE_MAP()

// ── 상태 아이콘 생성 ──────────────────────────────────────────
// 16×16 GDI 드로잉으로 4종 생성
void CFileListCtrl::BuildImageList()
{
    const int SZ = 16;
    m_imageList.Create(SZ, SZ, ILC_COLOR32 | ILC_MASK, 4, 0);

    struct IconDef { COLORREF color; wchar_t glyph; };
    IconDef defs[4] = {
        { RGB(160, 160, 160), L' '  },   // Wait  — 회색 빈 원
        { RGB( 79, 142, 247), L'…'  },   // Running — 파란색
        { RGB( 62, 207, 142), L'✓'  },   // Success — 초록색
        { RGB(224,  82,  82), L'✗'  },   // Fail   — 빨간색
    };

    for (auto& def : defs)
    {
        CBitmap bmp;
        bmp.CreateBitmap(SZ, SZ, 1, 32, nullptr);
        CDC dc;
        dc.CreateCompatibleDC(nullptr);
        CBitmap* pOld = dc.SelectObject(&bmp);

        // 배경 투명 처리를 위해 마젠타로 채움
        dc.FillSolidRect(0, 0, SZ, SZ, RGB(255, 0, 255));

        // 원 그리기
        CBrush br(def.color);
        CPen   pen(PS_SOLID, 1, def.color);
        CBrush* pOldBr = dc.SelectObject(&br);
        CPen*   pOldPn = dc.SelectObject(&pen);
        dc.Ellipse(1, 1, SZ - 1, SZ - 1);
        dc.SelectObject(pOldBr);
        dc.SelectObject(pOldPn);

        dc.SelectObject(pOld);
        m_imageList.Add(&bmp, RGB(255, 0, 255));
    }

    SetImageList(&m_imageList, LVSIL_SMALL);
}

// ── 컬럼 설정 ────────────────────────────────────────────────
void CFileListCtrl::SetupColumns()
{
    SetExtendedStyle(GetExtendedStyle()
        | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    BuildImageList();

    InsertColumn(0, _T(""), LVCFMT_CENTER, 24);
    InsertColumn(1, LS(IDS_FLC_COL1), LVCFMT_LEFT, 160);
    InsertColumn(2, LS(IDS_FLC_COL2), LVCFMT_LEFT, 160);
    InsertColumn(3, LS(IDS_FLC_COL3), LVCFMT_LEFT, 100);

    // 초기 컬럼 너비 적용
    CRect rc;
    GetClientRect(&rc);
    AdjustColumnWidths(rc.Width());
}

// ── 항목 추가 ────────────────────────────────────────────────
void CFileListCtrl::AddEntry(const FileEntry& entry)
{
    int idx = (int)m_entries.size();
    m_entries.push_back(entry);

    LVITEM item  = {};
    item.mask    = LVIF_TEXT | LVIF_IMAGE;
    item.iItem   = idx;
    item.iImage  = IMG_IDX_NONE;
    item.pszText = LPSTR_TEXTCALLBACK;  // 컬럼 0: 아이콘 전용, 텍스트 없음
    InsertItem(&item);
    SetItemText(idx, 1, entry.srcName);   // 컬럼 1: 원본 이미지 파일명
    SetItemText(idx, 2, entry.pdfName);   // 컬럼 2: 변환될 PDF 파일명
    SetItemText(idx, 3, _T(""));          // 컬럼 3: 비고 (초기 비어있음)

    if (m_bMerge && idx > 0)
        SetItemText(idx, 2, m_entries[0].pdfName);
}

// ── 중복 확인 ────────────────────────────────────────────────
bool CFileListCtrl::HasEntry(const CString& srcPath) const
{
    for (const auto& e : m_entries)
        if (e.type == FileEntry::Type::Image &&
            e.srcPath.CompareNoCase(srcPath) == 0) return true;
    return false;
}

bool CFileListCtrl::HasEntry(const CString& srcPath, int pageIndex) const
{
    for (const auto& e : m_entries)
        if (e.type == FileEntry::Type::PdfPage &&
            e.srcPath.CompareNoCase(srcPath) == 0 &&
            e.pageIndex == pageIndex) return true;
    return false;
}

// ── 상태 변경 ────────────────────────────────────────────────
void CFileListCtrl::SetStatus(int index, FileEntry::Status status)
{
    if (index < 0 || index >= (int)m_entries.size()) return;
    m_entries[index].status = status;

    int imgIdx = IMG_IDX_NONE;
    switch (status)
    {
    case FileEntry::Status::Running: imgIdx = IMG_IDX_RUNNING; break;
    case FileEntry::Status::Success: imgIdx = IMG_IDX_SUCCESS; break;
    case FileEntry::Status::Fail:    imgIdx = IMG_IDX_FAIL;    break;
    default: break;
    }
    LVITEM item = {};
    item.mask   = LVIF_IMAGE;
    item.iItem  = index;
    item.iImage = imgIdx;
    SetItem(&item);

    // 비고 컬럼 업데이트
    switch (status)
    {
    case FileEntry::Status::Success:
        SetItemText(index, 3, LS(IDS_FLC_STATUS_SUCCESS));
        break;
    case FileEntry::Status::Fail:
    {
        CString failText = m_entries[index].remark.IsEmpty()
                           ? LS(IDS_FLC_STATUS_FAIL) : m_entries[index].remark;
        SetItemText(index, 3, failText);
        break;
    }
    case FileEntry::Status::Running:
        SetItemText(index, 3, LS(IDS_FLC_STATUS_RUNNING));
        break;
    default:
        SetItemText(index, 3, _T(""));
        break;
    }

    RedrawItems(index, index);
}

// ── 합치기 모드 ──────────────────────────────────────────────
void CFileListCtrl::SetMergeMode(bool bMerge)
{
    if (m_entries.empty()) { m_bMerge = bMerge; return; }

    // PdfPage 항목이 있으면 합치기 모드 적용 불가
    for (const auto& e : m_entries)
        if (e.type == FileEntry::Type::PdfPage) { bMerge = false; break; }

    m_bMerge = bMerge;  // PdfPage 보정 후 저장

    if (bMerge)
    {
        // srcName 기준으로 매번 재계산 (중복 _통합 방지)
        CString srcName = m_entries[0].srcName;
        int dotPos = srcName.ReverseFind(_T('.'));
        CString stem = (dotPos >= 0) ? srcName.Left(dotPos) : srcName;
        CString mergedName = stem + LS(IDS_FLC_SUFFIX_MERGED);
        m_entries[0].pdfName = mergedName;
        for (int i = 0; i < (int)m_entries.size(); ++i)
            SetItemText(i, 2, mergedName);
    }
    else
    {
        // srcName에서 개별 pdfName 복원 (Image 타입만, PdfPage는 이미 올바른 pdfName 보유)
        for (int i = 0; i < (int)m_entries.size(); ++i)
        {
            if (m_entries[i].type == FileEntry::Type::PdfPage)
            {
                SetItemText(i, 2, m_entries[i].pdfName);
                continue;
            }
            CString srcName = m_entries[i].srcName;
            int dotPos = srcName.ReverseFind(_T('.'));
            CString stem = (dotPos >= 0) ? srcName.Left(dotPos) : srcName;
            m_entries[i].pdfName = stem + _T(".pdf");
            SetItemText(i, 2, m_entries[i].pdfName);
        }
    }
}

// ── 선택 항목 제거 ───────────────────────────────────────────
void CFileListCtrl::RemoveSelected()
{
    // 역순 삭제
    int i = GetNextItem(-1, LVNI_SELECTED);
    while (i >= 0)
    {
        DeleteItem(i);
        m_entries.erase(m_entries.begin() + i);
        i = GetNextItem(-1, LVNI_SELECTED);
    }
    // m_entries 갱신 완료 후 부모에 알림 (합치기 상태 재계산)
    CWnd* pParent = GetParent();
    if (pParent) pParent->PostMessage(WM_LIST_ENTRIES_CHANGED);
}

void CFileListCtrl::Clear()
{
    DeleteAllItems();
    m_entries.clear();
}

// ── 행 재표시 ────────────────────────────────────────────────
void CFileListCtrl::RefreshRow(int idx)
{
    if (idx < 0 || idx >= (int)m_entries.size()) return;
    const FileEntry& e = m_entries[idx];

    SetItemText(idx, 1, e.srcName);
    SetItemText(idx, 2, e.pdfName);

    int imgIdx = IMG_IDX_NONE;
    switch (e.status)
    {
    case FileEntry::Status::Running: imgIdx = IMG_IDX_RUNNING; break;
    case FileEntry::Status::Success: imgIdx = IMG_IDX_SUCCESS; break;
    case FileEntry::Status::Fail:    imgIdx = IMG_IDX_FAIL;    break;
    default: break;
    }
    LVITEM item = {};
    item.mask   = LVIF_IMAGE;
    item.iItem  = idx;
    item.iImage = imgIdx;
    SetItem(&item);

    CString remark;
    switch (e.status)
    {
    case FileEntry::Status::Success: remark = LS(IDS_FLC_STATUS_SUCCESS); break;
    case FileEntry::Status::Fail:
        remark = e.remark.IsEmpty() ? LS(IDS_FLC_STATUS_FAIL) : e.remark; break;
    case FileEntry::Status::Running: remark = LS(IDS_FLC_STATUS_RUNNING); break;
    default: break;
    }
    SetItemText(idx, 3, remark);
}

// ── 항목 위/아래 이동 ────────────────────────────────────────
bool CFileListCtrl::MoveUp(int idx)
{
    if (idx <= 0 || idx >= (int)m_entries.size()) return false;
    if (m_entries[idx].type == FileEntry::Type::PdfPage) return false;

    std::swap(m_entries[idx], m_entries[idx - 1]);
    RefreshRow(idx - 1);
    RefreshRow(idx);

    SetItemState(idx,     0,                          LVIS_SELECTED | LVIS_FOCUSED);
    SetItemState(idx - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    EnsureVisible(idx - 1, FALSE);
    return true;
}

bool CFileListCtrl::MoveDown(int idx)
{
    int n = (int)m_entries.size();
    if (idx < 0 || idx >= n - 1) return false;
    if (m_entries[idx].type == FileEntry::Type::PdfPage) return false;

    std::swap(m_entries[idx], m_entries[idx + 1]);
    RefreshRow(idx);
    RefreshRow(idx + 1);

    SetItemState(idx,     0,                          LVIS_SELECTED | LVIS_FOCUSED);
    SetItemState(idx + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    EnsureVisible(idx + 1, FALSE);
    return true;
}

void CFileListCtrl::SetRowColorsEnabled(bool bEnable)
{
    m_bRowColors = bEnable;
    if (GetSafeHwnd())
        Invalidate();
}

// ── 키 입력 ──────────────────────────────────────────────────
void CFileListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_DELETE)
        RemoveSelected();
    CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

// ── 우클릭 컨텍스트 메뉴 ─────────────────────────────────────
void CFileListCtrl::OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int idx = pNMIA->iItem;
    bool hasItem = (idx >= 0 && idx < (int)m_entries.size());

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 1, LS(IDS_FLC_MENU_REMOVE));
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 2, LS(IDS_FLC_MENU_OPEN_LOC));
    menu.AppendMenu(hasItem ? MF_STRING : MF_STRING | MF_GRAYED, 4, LS(IDS_FLC_MENU_OPEN));
    menu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)nullptr);
    menu.AppendMenu(m_entries.empty() ? MF_STRING | MF_GRAYED : MF_STRING, 3, LS(IDS_FLC_MENU_CLEAR_ALL));

    CPoint pt;
    GetCursorPos(&pt);
    int cmd = menu.TrackPopupMenu(
        TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, this);

    if (cmd == 1)
    {
        RemoveSelected();
    }
    else if (cmd == 2)
    {
        CString path = m_entries[idx].srcPath;
        ShellExecute(nullptr, _T("explore"),
            path.Left(path.ReverseFind(_T('\\'))),
            nullptr, nullptr, SW_SHOWNORMAL);
    }
    else if (cmd == 3)
    {
        Clear();
        CWnd* pParent = GetParent();
        if (pParent) pParent->PostMessage(WM_LIST_ENTRIES_CHANGED);
    }
    else if (cmd == 4)
    {
        ShellExecute(nullptr, _T("open"), m_entries[idx].srcPath,
                     nullptr, nullptr, SW_SHOWNORMAL);
    }
    *pResult = 0;
}

// ── 행 배경색 커스텀 드로 ────────────────────────────────────
void CFileListCtrl::OnNMCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    switch (pCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    case CDDS_ITEMPREPAINT:
    {
        int idx = (int)pCD->nmcd.dwItemSpec;
        if (m_bRowColors && idx >= 0 && idx < (int)m_entries.size())
        {
            switch (m_entries[idx].status)
            {
            case FileEntry::Status::Running:
                pCD->clrTextBk = RGB(30, 58, 110); break;
            case FileEntry::Status::Fail:
                pCD->clrTextBk = RGB(80, 30, 30);  break;
            case FileEntry::Status::Success:
                pCD->clrTextBk = RGB(20, 50, 35);  break;
            default:
                pCD->clrTextBk = CLR_DEFAULT;      break;
            }
        }
        else
        {
            pCD->clrTextBk = CLR_DEFAULT;
        }
        *pResult = CDRF_DODEFAULT;
        return;
    }
    }
    *pResult = CDRF_DODEFAULT;
}

void CFileListCtrl::ApplyLanguage()
{
    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT;
    CString col1 = LS(IDS_FLC_COL1); lvc.pszText = col1.GetBuffer(); SetColumn(1, &lvc); col1.ReleaseBuffer();
    CString col2 = LS(IDS_FLC_COL2); lvc.pszText = col2.GetBuffer(); SetColumn(2, &lvc); col2.ReleaseBuffer();
    CString col3 = LS(IDS_FLC_COL3); lvc.pszText = col3.GetBuffer(); SetColumn(3, &lvc); col3.ReleaseBuffer();
}

// ── 컬럼 자동 너비 ───────────────────────────────────────────
void CFileListCtrl::AdjustColumnWidths(int cx)
{
    // 세로 스크롤바 너비를 미리 빼서 가로 스크롤 발생 방지
    int scrollW = GetSystemMetrics(SM_CXVSCROLL);
    int avail   = cx - 24 - scrollW;
    if (avail <= 0) return;

    int col1w = avail * 2 / 5;
    int col2w = avail * 2 / 5;
    int col3w = avail - col1w - col2w;
    SetColumnWidth(1, col1w);
    SetColumnWidth(2, col2w);
    SetColumnWidth(3, col3w);
}

void CFileListCtrl::OnSize(UINT nType, int cx, int cy)
{
    CListCtrl::OnSize(nType, cx, cy);
    if (cx <= 24) return;
    AdjustColumnWidths(cx);
}
