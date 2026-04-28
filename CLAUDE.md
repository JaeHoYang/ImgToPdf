# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

이미지(JPG/PNG/BMP/TIFF/GIF) ↔ PDF 양방향 변환 Windows MFC 데스크톱 앱.  
드래그앤드롭, 다중 파일 병렬 변환, 합치기(다중 페이지 PDF), PDF→페이지별 JPG 추출, ▲▼ 순서 변경, 인앱 도움말(툴팁·F1·시스템 메뉴), 삭제 버튼 이중 모드(선택 제거/전체 초기화), 에디트박스 플레이스홀더, **런타임 한국어/영어 전환**(재시작 불필요)을 제공한다.  
3탭 구조: 탭1 이미지 변환 / 탭2 PDF 도구 / 탭3 MD 변환. 탭 순서 변경 후 레지스트리 저장.  
탭2에 **AI 요약 기능** 포함: WinRT OCR로 텍스트 추출 → 로컬 Ollama(llama3.2:3b)로 구조화 요약 → 메모장 열기.  
배포: `installer/ImgToPdf_setup.iss` (Inno Setup 6.x, v2.1). Ollama 자동 다운로드/설치 지원.

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
설치 시 앱 언어(한국어/English) 선택 → `HKCU\Software\jaeho\ImgToPdf\Settings\Lang`에 기록.

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

- `GetPageCount` — RAII `struct RoInit` → StorageFile → PdfDocument → get_PageCount
- `RenderPageToJpg` — PdfPage → InMemoryRandomAccessStream → RenderToStreamAsync → CreateStreamOverRandomAccessStream → Gdiplus::Bitmap → JPEG 저장(품질 90)
- 각 워커 스레드에서 `RoInitialize / RoUninitialize` 호출 필수. **`GetPageCount`·`RenderPageToJpg` 모두 RAII `struct RoInit { ~RoInit(){RoUninitialize();} }` 패턴 사용** — 예외 발생 시에도 RoUninitialize 보장.
- `shcore.lib` 필요 (`CreateStreamOverRandomAccessStream`)

### 언어 전환 (AppLang)

런타임 한국어/영어 전환. 재시작 불필요.

- `AppLang.h`: `enum class Lang { KO, EN }`, `AppStringId` enum (115개+ ID), `LS(id)`, `BuildFilter()`, `SetLang()`, `LoadSavedLang()` 선언
- `AppLang.cpp`: `s_ko[]` / `s_en[]` — `static const wchar_t*` 배열. 한국어는 Unicode escape(`\uXXXX`)로 저장(인코딩 무관).
- `g_lang` 전역 → `LS(id)`가 참조. 언어 설정은 `HKCU\Software\jaeho\ImgToPdf\Settings\Lang`(DWORD, 0=KO, 1=EN).
- `ImgToPdf.cpp`의 `SetRegistryKey(_T("jaeho"))` 필수 — 없으면 `WriteProfileInt`가 INI 파일 폴백으로 저장 실패.
- 토글: 시스템 메뉴 `IDM_LANG_TOGGLE = 0x1002` → `SetLang()` → `ApplyLanguage()` 호출로 즉시 반영.
- `LoadSavedLang()`은 `OnInitDialog()` 최상단에서 호출.

**ApplyLanguage 패턴**  
`CTabDlgBase`에 `virtual void ApplyLanguage() {}` 선언. 각 탭 클래스와 `CFileListCtrl` 모두 override.  
호출 체인: `CImgToPdfDlg::ApplyLanguage()` → 탭 레이블 갱신 → 각 탭 `ApplyLanguage()` → `m_listFiles.ApplyLanguage()`.

**EM_SETCUEBANNER 주의**: 반드시 로컬 `CString`에 저장 후 `(LPARAM)(LPCWSTR)cue` 캐스트. 임시 객체 포인터 전달 금지.  
**툴팁 갱신**: `m_toolTip.UpdateTipText(LS(...), &ctrl)` — `DeleteTool`/`AddTool` 불필요.  
**컬럼 헤더 갱신**: `LVCOLUMN lvc; lvc.mask = LVCF_TEXT; SetColumn(n, &lvc)` 패턴.  
**문자열 추가 시**: `AppStringId` enum → `s_ko[]` → `s_en[]` 세 곳을 동시에, **같은 인덱스 순서**로 수정.

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

### 커스텀 메시지 (AppMessages.h)

모든 커스텀 메시지는 `AppMessages.h`에 정의. `resource.h`에 넣지 말 것 — VS 리소스 에디터가 non-resource `#define`을 제거한다.

| 상수 | 값 | 방향 | 용도 |
|------|----|------|------|
| `WM_CONVERT_PROGRESS` | WM_USER+1 | 워커→UI | wParam=항목인덱스, lParam=상태코드 |
| `WM_CONVERT_DONE` | WM_USER+2 | 워커→UI | wParam=성공수, lParam=실패수 |
| `WM_LIST_ENTRIES_CHANGED` | WM_USER+3 | FileListCtrl→Dialog | erase 완료 후 합치기·버튼 상태 갱신 트리거 |
| `WM_PDF_TOOL_DONE` | WM_USER+11 | 탭2 워커→UI | wParam=성공여부(1/0) |
| `WM_TAB_STATE_CHANGED` | WM_USER+20 | 탭→호스트 | 버튼 상태 재계산 트리거 |
| `WM_ROUTE_DROP` | WM_USER+21 | 탭→호스트 | 드롭 파일 라우팅 |
| `WM_MD_CONVERT_DONE` | WM_USER+22 | 탭3 워커→UI | wParam=성공여부(1/0) |
| `WM_WORD_ITEM_DONE` | WM_USER+23 | 워커→UI | Word 변환 항목 완료 |
| `WM_WORD_CONVERT_DONE` | WM_USER+24 | 워커→UI | Word 전체 변환 완료 |
| `WM_PPT_ITEM_DONE` | WM_USER+25 | 워커→UI | PPT 변환 항목 완료 |
| `WM_PPT_CONVERT_DONE` | WM_USER+26 | 워커→UI | PPT 전체 변환 완료 |
| `WM_PDF_PAGES_LOADED` | WM_USER+27 | 백그라운드 스레드→탭2 | PDF 드롭 후 페이지 수 비동기 로드 완료. lParam=`new vector<pair<CString,int>>*` (수신 측에서 delete 필수) |

### 탭2 AI 요약 (CPdfToolsDlg)

PDF → OCR → Ollama 파이프라인. 모두 백그라운드 스레드(`m_summaryWorker`)에서 실행.

**PDF 드롭 시 페이지 수 비동기 로드**  
`AddPdfFiles` → 즉시 `pages=-1`("..." 표시)로 목록에 추가 → 백그라운드 스레드에서 `GetPageCount` 실행 → `WM_PDF_PAGES_LOADED`로 결과 전달 → `OnPdfPagesLoaded`에서 갱신. `CanRun()`은 `pages < 0`인 항목이 있으면 false 반환.  
`PostMessage` 실패 시 lParam으로 전달한 `vector*`를 즉시 `delete` — UI 스레드가 수신하지 못하면 누수 발생.

**요약 스레드 안전성**  
`m_summaryWorker`는 `detach()`로 실행. raw HWND를 캡처하므로 `setResult` 람다 내에서 **반드시 `::IsWindow()` 가드** 후 `SetWindowText`/`EnableWindow` 호출. 다이얼로그 소멸 후 dangling HWND 크래시 방지.  
`OnDestroy`에서 `if (m_summaryWorker.joinable()) m_summaryWorker.join()` 필수.

**버튼 상태 규칙**  
- `IDC_BTN_PDF_SUMMARIZE`: 항목 선택 + 미실행 중일 때 활성  
- `IDC_BTN_PDF_OPEN_NOTEPAD`: 요약 **성공** 시에만 활성(`setResult(..., true)`). 목록 항목 삭제·선택 해제 시 `OnLvnItemChangedListPdf`에서 비활성.

**OCR**: `Windows.Media.Ocr.OcrEngine::TryCreateFromUserProfileLanguages` → 최대 5페이지 렌더링 후 텍스트 추출.  
**Ollama**: WinHTTP POST `http://localhost:11434/api/generate`, model=`llama3.2:3b`, stream=false. JSON `response` 필드 수동 파싱. `\n` → `\r\n` 변환(CEdit 다중 행).  
**메모장 열기**: `GetTempFileNameW` → `MoveFileW`로 `.txt` 확장자 부여 → UTF-8 BOM 저장 → `ShellExecuteW(notepad.exe)`. 실패 경로마다 `DeleteFileW` 필수.

### WM_LIST_ENTRIES_CHANGED (WM_USER+3)

`CFileListCtrl::RemoveSelected()` 완료 후 `GetParent()->PostMessage(WM_LIST_ENTRIES_CHANGED)` 전송. `m_entries.erase` 이후에 발송하므로 `UpdateMergeCheckState` 호출 시 벡터가 확정 상태임을 보장. `LVN_ITEMCHANGED`에서 직접 호출하면 erase 전이라 PDF 페이지 항목이 잔류해 합치기 체크박스가 재활성되지 않는 버그가 발생한다.

## 주요 제약 및 주의사항

- `resource.h`에 한국어/em-dash 주석 금지 — RC 전처리기 오류 발생.
- `.rc` 파일 상단에 `#pragma code_page(65001)` 필요 (한국어 리터럴).
- `pch.h`에 `<afxdialogex.h>` 포함 필수 (`CDialogEx` 정의). `VC_EXTRA_LEAN` 사용 금지.
- `targetver.h`에서 `WINVER`/`_WIN32_WINNT` 정의는 `#ifndef` 가드 후 `#include <sdkddkver.h>` 순서.
- GDI+ 종료(`GdiplusShutdown`) 전에 `CImgToPdfDlg`가 소멸되어야 한다 — `ImgToPdf.cpp`의 중괄호 스코프로 보장.
- `SetupColumns()` 호출 후 `AdjustColumnWidths`가 즉시 실행되므로, 컬럼 추가/제거 시 두 곳 모두 수정.
- **`TaskDialogIndirect` 사용 금지** — ComCtl32 v6 ordinal 345 런타임 오류 발생. `::MessageBoxW(GetSafeHwnd(), text, caption, flags)` 사용. `CWnd::MessageBoxW`(인수 3개)와 혼동 주의.
- `pch.h`에 `#pragma comment(lib, "comctl32.lib")` 추가 금지 — ComCtl32 v5 강제 로드로 ordinal 345 누락.
- `FileListCtrl::SetMergeMode`에서 `m_bMerge` 대입은 PdfPage 존재 여부 확인 **이후**에 수행해야 한다 — 순서가 바뀌면 PdfPage 항목의 pdfName이 이미지 규칙으로 재계산되어 깨짐.
- `LVN_ITEMCHANGED` 핸들러에서 선택 상태가 바뀔 때마다 `UpdateMoveButtonState()`, `UpdateClearButtonState()` 호출 — 변환 중·PdfPage·맨위/맨아래 조건을 모두 검사.
- `RemoveSelected` 후 합치기 활성화 버그: `LVN_ITEMCHANGED`는 `DeleteItem` 시점(erase 전)에 발생하므로 `UpdateMergeCheckState`를 직접 호출하면 안 됨. `WM_LIST_ENTRIES_CHANGED` PostMessage를 통해 erase 완료 후 처리해야 한다.
- 전체 초기화 시 에디트박스(`m_editPath`)도 반드시 `SetWindowText(_T(""))` 초기화할 것 — 비우면 플레이스홀더가 자동 복원됨.
- ComCtl32 v6 manifest는 `ImgToPdf.cpp`의 linker pragma로만 활성화 — 별도 `.manifest` 파일 불필요.
- 설치 파일(`installer/ImgToPdf_setup.iss`)은 Inno Setup 6.x용. VS Installer Projects / WiX는 사용하지 않음.
- `SetRegistryKey(_T("jaeho"))` — `ImgToPdf.cpp` `InitInstance()` 최상단에 반드시 존재해야 `WriteProfileInt`/`GetProfileInt`가 레지스트리를 사용한다. 없으면 INI 파일 폴백으로 언어·탭 순서 저장 실패.
- `AppLang.cpp`의 삼항 연산자에서 `CString`과 `const wchar_t*` 혼용 시 C2445 오류 발생 — 양쪽을 `CString(...)` 또는 `const wchar_t*`로 통일해야 한다.
- `BuildFilter()` 반환값은 `std::vector<TCHAR>`이므로 `lpstrFilter = filterVec.data()` — `filterVec`이 살아있는 스코프 내에서만 유효.
- `LS()` 호출 결과를 `EM_SETCUEBANNER`에 바로 넘기지 말 것 — 임시 객체 소멸 문제. 로컬 `CString cue = LS(...)` 후 `(LPARAM)(LPCWSTR)cue` 사용 필수.
- 탭2 `setResult` 람다: raw HWND 캡처 후 `detach()` → 반드시 `::IsWindow(hEdit)` 가드 먼저. 가드 없이 `SetWindowText`/`EnableWindow` 호출 시 다이얼로그 소멸 후 크래시.
- `WM_PDF_PAGES_LOADED` lParam은 `new vector<pair<CString,int>>*` — `OnPdfPagesLoaded`에서 반드시 `delete`. `PostMessage` 실패 시 스레드에서 즉시 `delete`.
- `PdfConverter::GetPageCount` / `RenderPageToJpg`에서 `RoInitialize`·`RoUninitialize`는 RAII `struct RoInit`으로만 처리. 직접 쌍으로 호출 금지 — 예외 경로에서 `RoUninitialize` 누락됨.
- `CPdfToolsDlg::OnDestroy`에서 `m_summaryWorker.joinable()` → `join()` 필수. 누락 시 joinable 스레드가 소멸자에서 `std::terminate` 호출.
- `OnBnClickedOpenNotepad`의 `GetTempFileNameW`는 빈 파일을 즉시 생성한다. `MoveFileW` 또는 `ofstream` 실패 시 `DeleteFileW`로 정리 필수 — 누락 시 `%TEMP%`에 빈 파일 누적.
- 인스톨러(`ImgToPdf_setup.iss`) Ollama 자동설치: `CurStepChanged(ssPostInstall)` → `FindOllamaExe()` 미발견 → `DownloadAndInstallOllama()` (이중 MsgBox 동의) → PowerShell `Invoke-WebRequest` → `OllamaSetup.exe` 실행 → 재확인 → `TryPullOllamaModel()`. CustomMessages는 한국어/영어 모두 정의.
