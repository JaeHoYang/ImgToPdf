# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

이미지(JPG/PNG/BMP/TIFF/GIF) ↔ PDF 양방향 변환 Windows MFC 데스크톱 앱.  
드래그앤드롭, 다중 파일 병렬 변환, 합치기(다중 페이지 PDF), PDF→페이지별 JPG 추출, ▲▼ 순서 변경, 인앱 도움말(툴팁·F1·시스템 메뉴), 삭제 버튼 이중 모드(선택 제거/전체 초기화), 에디트박스 플레이스홀더 기능을 제공한다.  
배포: `installer/ImgToPdf_setup.iss` (Inno Setup 6.x).

## 빌드

Visual Studio 2022, x64, Debug/Release. IDE에서 직접 빌드하거나:

```
msbuild ImgToPdf.vcxproj /p:Configuration=Debug /p:Platform=x64
msbuild ImgToPdf.vcxproj /p:Configuration=Release /p:Platform=x64
```

**필수 컴파일 플래그**: `/utf-8` (소스 내 한국어 주석), `gdiplus.lib`, `shell32.lib`, `runtimeobject.lib`, `shcore.lib` 링크.  
MFC Dynamic 사용 (`UseOfMfc=Dynamic`), C++17.  
ComCtl32 v6 활성화: `ImgToPdf.cpp`의 `#pragma comment(linker, "/manifestdependency:...")` — `EM_SETCUEBANNER` 등 v6 전용 기능 사용에 필수. `pch.h`에 `#pragma comment(lib, "comctl32.lib")` 추가 금지 (v5 강제 로드).

**설치 파일 빌드**: Release x64 빌드 후 `installer/ImgToPdf_setup.iss` → Inno Setup Compiler에서 F9.

## 아키텍처

### 스레드 모델

변환 버튼 클릭 → `ConvertWorker::Start(ConvertTask)` → 코디네이터 스레드 생성 → `hardware_concurrency()` 개의 워커 스레드. 워커→UI 통신은 `PostMessage(hNotify, WM_CONVERT_PROGRESS/DONE)` 만 사용한다. 공유 데이터(`FileEntry::remark`)는 PostMessage 전에 워커가 쓰고, UI가 메시지 수신 후 읽는다 — x86/x64 메모리 오더 보장.

**합치기 모드**: Phase 1(병렬 로딩) → Phase 2(단일 스레드 PDF 쓰기).  
**개별 모드**: `std::atomic<int> nextIdx`로 lock-free 인덱스 분배.

`ConvertTask::entries`는 `m_listFiles.m_entries` 벡터의 포인터 목록이므로, 변환 중에는 `DragAcceptFiles(false)`로 벡터 재할당을 막는다.

### PDF 생성 (PdfWriter)

외부 PDF 라이브러리 없이 PDF 1.4 스트림을 직접 작성한다.
- JPEG → `/DCTDecode` 필터로 원본 바이트 그대로 embed (무손실)
- 그 외 포맷 → stb_image로 디코딩 후 raw RGB 무압축 embed

xref 테이블 오프셋은 **오브젝트 번호 = 쓰기 순서**가 일치해야 유효한 PDF가 생성된다. `WriteSingle`의 오브젝트 번호 배정(1=Catalog, 2=Pages, 3=Image, 4=Content, 5=Page)은 이 순서를 반드시 지켜야 한다.

한글 경로 지원: stb_image에 `STBI_WINDOWS_UTF8` 정의, `std::ofstream`은 `(LPCTSTR)dstPath` (wchar_t 경로) 직접 전달.

### PDF 읽기 / JPG 렌더링 (PdfConverter)

`Windows.Data.Pdf` WinRT API (Windows 10 내장, 외부 DLL 불필요). WRL + SyncWait 패턴으로 비동기 API를 동기 호출로 래핑한다.

- `GetPageCount` — RoInitialize → StorageFile → PdfDocument → get_PageCount → RoUninitialize
- `RenderPageToJpg` — PdfPage → InMemoryRandomAccessStream → RenderToStreamAsync → CreateStreamOverRandomAccessStream → Gdiplus::Bitmap → JPEG 저장(품질 90)
- 각 워커 스레드에서 `RoInitialize / RoUninitialize` 호출 필수
- `shcore.lib` 필요 (`CreateStreamOverRandomAccessStream`)

### UI 컴포넌트

- `CFileListCtrl` — `CListCtrl` 서브클래스. `NM_CUSTOMDRAW`로 행 배경색, `CImageList`로 상태 아이콘(Wait/Running/Success/Fail). 컬럼 너비는 자체 `OnSize`에서 `AdjustColumnWidths(cx)`로 관리 (세로 스크롤바 너비 `SM_CXVSCROLL` 선제 차감으로 가로 스크롤 방지). `MoveUp/MoveDown`은 `std::swap + RefreshRow` 패턴으로 인플레이스 재정렬.
- `CProgressLabel` — `CStatic` 서브클래스. owner-draw로 `(완료/실패/총)` 3색 텍스트 렌더링.
- `CImagePreviewCtrl` — `CStatic` 서브클래스. GDI+로 비율 유지 Fit 렌더링. `m_bitmap`은 `unique_ptr<Gdiplus::Bitmap>`.
- `CImgToPdfDlg` — 창 크기 변경 시 `ResizeControls`에서 초기 컨트롤 rect(OnInitDialog에서 캡처한 `m_rcXxx0`)와 delta(`dw`, `dh`)로 위치 조정. 프로그레스 행 우측에 `m_btnMoveUp(▲)`, `m_btnMoveDown(▼)`, `m_btnClear(삭제)` 배치.
- `CToolTipCtrl m_toolTip` — `PreTranslateMessage`에서 `RelayEvent`로 모든 컨트롤에 툴팁 전달. `OnInitDialog`에서 10개 컨트롤 등록. 삭제 버튼 툴팁은 `UpdateClearButtonState()`에서 `UpdateTipText()`로 상태에 따라 동적 갱신.
- 인앱 도움말 — 시스템 메뉴(`IDM_HELP_USAGE`) 추가 + F1(`WM_HELPINFO`) 핸들링 → `ShowHelpDialog()` 호출 → `::MessageBoxW`로 표시. 제작자/이메일(`jaeho9697@gmail.com`) 포함.
- 에디트박스 플레이스홀더 — `OnInitDialog`에서 `EM_SETCUEBANNER`로 "F1을 누르면 사용 방법이 나옵니다." 설정. ComCtl32 v6 필수. 초기 포커스는 `m_listFiles.SetFocus(); return FALSE` 로 리스트뷰에 줘야 플레이스홀더가 보임. 전체 초기화 시 `m_editPath.SetWindowText(_T(""))` 하면 플레이스홀더 자동 복원.

### 삭제 버튼 (m_btnClear) 상태 머신

`m_bConversionDone` 플래그로 두 가지 모드를 전환한다.

| 상태 | 활성 | 클릭 동작 | 툴팁 |
|------|------|-----------|------|
| 변환 중 | 비활성 | — | — |
| 항목 선택됨 (변환 전) | 활성 | `RemoveSelected()` | "선택한 항목을 목록에서 제거합니다." |
| 변환 완료 (`m_bConversionDone=true`) | 활성 | 확인 MessageBox → 전체 초기화 | "목록·진행 상태를 전체 초기화합니다." |
| 선택 없음·변환 전 | 비활성 | — | 두 기능 모두 안내 |

`UpdateClearButtonState()` 호출 위치: `SetConvertingState`, `OnConvertDone`, `OnLvnItemChangedListFiles`, `OnListEntriesChanged`, `OnBnClickedClear`.

### WM_LIST_ENTRIES_CHANGED (WM_USER+3)

`CFileListCtrl::RemoveSelected()` 완료 후 `GetParent()->PostMessage(WM_LIST_ENTRIES_CHANGED)` 전송. `m_entries.erase` 이후에 발송하므로 `UpdateMergeCheckState` 호출 시 벡터가 확정 상태임을 보장. `LVN_ITEMCHANGED`에서 직접 호출하면 erase 전이라 PDF 페이지 항목이 잔류해 합치기 체크박스가 재활성되지 않는 버그가 발생한다.

## 주요 제약 및 주의사항

- `resource.h`에 한국어/em-dash 주석 금지 — RC 전처리기 오류 발생.
- `.rc` 파일 상단에 `#pragma code_page(65001)` 필요 (한국어 리터럴).
- `pch.h`에 `<afxdialogex.h>` 포함 필수 (`CDialogEx` 정의). `VC_EXTRA_LEAN` 사용 금지.
- `targetver.h`에서 `WINVER`/`_WIN32_WINNT` 정의는 `#ifndef` 가드 후 `#include <sdkddkver.h>` 순서.
- GDI+ 종료(`GdiplusShutdown`) 전에 `CImgToPdfDlg`가 소멸되어야 한다 — `ImgToPdf.cpp`의 중괄호 스코프로 보장.
- `SetupColumns()` 호출 후 `AdjustColumnWidths`가 즉시 실행되므로, 컬럼 추가/제거 시 두 곳 모두 수정.
- **`TaskDialogIndirect` 사용 금지** — `comctl32.lib` `#pragma comment` 없이 링크 시 런타임 ordinal 345 오류 발생. 대화상자 대신 `::MessageBoxW`(전역 Win32 API)를 사용한다. `CWnd::MessageBoxW`는 인수 3개(text, caption, type)이므로 HWND를 첫 인수로 넘기려면 `::MessageBoxW` 호출 필수.
- `pch.h`에 `#pragma comment(lib, "comctl32.lib")` 추가 금지 — ComCtl32 v5 강제 로드로 ordinal 345 누락.
- `FileListCtrl::SetMergeMode`에서 `m_bMerge` 대입은 PdfPage 존재 여부 확인 **이후**에 수행해야 한다 — 순서가 바뀌면 PdfPage 항목의 pdfName이 이미지 규칙으로 재계산되어 깨짐.
- `LVN_ITEMCHANGED` 핸들러에서 선택 상태가 바뀔 때마다 `UpdateMoveButtonState()`, `UpdateClearButtonState()` 호출 — 변환 중·PdfPage·맨위/맨아래 조건을 모두 검사.
- `RemoveSelected` 후 합치기 활성화 버그: `LVN_ITEMCHANGED`는 `DeleteItem` 시점(erase 전)에 발생하므로 `UpdateMergeCheckState`를 직접 호출하면 안 됨. `WM_LIST_ENTRIES_CHANGED` PostMessage를 통해 erase 완료 후 처리해야 한다.
- 전체 초기화 시 에디트박스(`m_editPath`)도 반드시 `SetWindowText(_T(""))` 초기화할 것 — 비우면 플레이스홀더가 자동 복원됨.
- `ShowHelpDialog`에서 `TaskDialogIndirect` 사용 금지 — `::MessageBoxW` 사용. `CWnd::MessageBoxW`(인수 3개)와 혼동 주의, HWND를 직접 넘길 때는 `::MessageBoxW(GetSafeHwnd(), ...)`.
- ComCtl32 v6 manifest는 `ImgToPdf.cpp`의 linker pragma로만 활성화 — 별도 `.manifest` 파일 불필요.
- 설치 파일(`installer/ImgToPdf_setup.iss`)은 Inno Setup 6.x용. VS Installer Projects / WiX는 사용하지 않음.
