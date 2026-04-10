# ImgToPdf

이미지 ↔ PDF 양방향 변환 Windows 데스크탑 도구

- **이미지 → PDF**: JPG / PNG / BMP / TIFF / GIF 파일을 개별 또는 다중 페이지 PDF로 변환
- **PDF → JPG**: PDF 파일을 페이지별 JPG 이미지로 추출

외부 DLL 없이 Windows 10 기본 API만 사용합니다.

---

## 주요 기능

| 기능 | 설명 |
|------|------|
| 이미지 → PDF | 개별 PDF 또는 합치기(다중 페이지 PDF) 선택 가능 |
| PDF → JPG | PDF 추가 시 페이지 수 자동 스캔 → 페이지별 행으로 펼쳐 표시 |
| 드래그 앤 드롭 | 파일·폴더 드롭 지원 |
| 병렬 변환 | `hardware_concurrency()` 기반 워커 스레드 풀 |
| 순서 변경 | ▲▼ 버튼으로 이미지 항목 순서 조정 |
| 진행 상태 | 프로그레스바 + 3색 카운터 `(완료 / 실패 / 총계)` 실시간 표시 |
| 이미지 미리보기 | 목록 클릭 시 하단에 비율 유지 미리보기 |
| 삭제 (이중 모드) | 변환 전: 선택 항목 제거 / 변환 완료 후: 전체 초기화 (확인 대화상자) |
| 인앱 도움말 | F1 키 또는 시스템 메뉴 → 사용 방법 대화상자 |
| 경로 힌트 | 에디트박스에 "F1을 누르면 사용 방법이 나옵니다." 플레이스홀더 표시 |
| 한글 경로 지원 | 입출력 경로 한글 완전 지원 |

---

## 스크린샷

![ImgToPdf](imgtopdf.png)

```
┌──────────────────────────────────────────────────────────────────────┐
│  ImgToPdf                                               [─][□][✕]   │
├──────────────────────────────────────────────────────────────────────┤
│  경로: [F1을 누르면 사용 방법이 나옵니다.]  [☐합치기] [찾기] [변환] │
├──────────────────────────────────────────────────────────────────────┤
│  [████████████░░░░░░░░░░░]  (3/1/10)  [▲] [▼] [삭제]               │
├──────────────────────────────────────────────────────────────────────┤
│  ● │ image001.jpg              │ image001.pdf                        │
│  ● │ image002.png              │ image002.pdf                        │
│  ● │ document.pdf (p.1/3)      │ document_p001.jpg                   │
│  ● │ document.pdf (p.2/3)      │ document_p002.jpg                   │
│  ● │ document.pdf (p.3/3)      │ document_p003.jpg                   │
├──────────────────────────────────────────────────────────────────────┤
│                       이미지 미리보기                                 │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 요구사항

| 항목 | 최소 사양 |
|------|-----------|
| OS | Windows 10 (버전 1803 이상) |
| 빌드 도구 | Visual Studio 2022 (v143 툴셋) |
| SDK | Windows 10 SDK 10.0 이상 |
| 런타임 | MFC Dynamic (Visual C++ 재배포 패키지) |

---

## 빌드

### Visual Studio

1. `ImgToPdf.sln` 열기
2. 구성: `Debug` 또는 `Release`, 플랫폼: `x64`
3. **빌드 → 솔루션 빌드** (`Ctrl+Shift+B`)

### MSBuild (명령줄)

```bash
msbuild ImgToPdf.vcxproj /p:Configuration=Release /p:Platform=x64
```

### 빌드 옵션

| 항목 | 값 |
|------|----|
| C++ 표준 | C++17 (`/std:c++17`) |
| 문자셋 | Unicode |
| MFC | Dynamic |
| 추가 컴파일 플래그 | `/utf-8` |
| 링크 라이브러리 | `gdiplus.lib`, `shell32.lib`, `runtimeobject.lib`, `shcore.lib` |
| ComCtl32 | v6 (linker manifest pragma로 활성화) |

---

## 설치 파일 빌드 (Inno Setup)

1. [Inno Setup](https://jrsoftware.org/isdl.php) 설치
2. VS에서 `Release x64` 빌드 → `x64\Release\ImgToPdf.exe` 확인
3. `installer\ImgToPdf_setup.iss` 를 Inno Setup Compiler로 열기
4. **F9** (Compile) → `installer\ImgToPdf_Setup_v1.0.exe` 생성

> **런타임 DLL**: `x64\Release` 폴더에 `mfc142u.dll`, `msvcp140.dll`, `vcruntime140.dll`, `vcruntime140_1.dll` 이 있으면 설치 파일에 포함됩니다. 없어도 빌드는 됩니다 (`skipifsourcedoesntexist`).

---

## 사용법

### 이미지 → PDF

1. 이미지 파일(JPG / PNG / BMP / TIFF / GIF)을 드래그 앤 드롭하거나 **[찾기]** 버튼으로 추가
2. *(선택)* **합치기** 체크박스 활성화 → 모든 이미지를 하나의 다중 페이지 PDF로 합침
3. *(선택)* **▲▼** 버튼으로 변환 순서 조정
4. **[변환]** 클릭
5. PDF 파일이 원본 이미지와 동일한 폴더에 저장됨

### PDF → JPG

1. PDF 파일을 드래그 앤 드롭하거나 **[찾기]** 버튼으로 추가
2. 페이지 수가 자동 스캔되어 각 페이지가 별도 행으로 표시됨
3. **[변환]** 클릭
4. `원본명_p001.jpg`, `원본명_p002.jpg`, … 형태로 저장됨

### 삭제 버튼

| 상태 | 동작 |
|------|------|
| 항목 선택됨 (변환 전) | 선택한 항목만 목록에서 제거 |
| 변환 완료 후 | 확인 대화상자 → 목록·진행 상태 전체 초기화 |

### 도움말

- **F1** 키 또는 **시스템 메뉴(─) → 사용 방법** 으로 인앱 도움말 표시
- 문의: jaeho9697@gmail.com

### 출력 경로

모든 변환 파일은 **원본 파일과 동일한 폴더**에 저장됩니다.

---

## 기술 스택

| 역할 | 기술 |
|------|------|
| UI 프레임워크 | MFC `CDialogEx` 기반 다이얼로그 앱 |
| 이미지 로딩 | [`stb_image.h`](https://github.com/nothings/stb) (헤더 온리) |
| 이미지 렌더링 | GDI+ (`Gdiplus::Bitmap`) |
| PDF 생성 | PDF 1.4 스트림 직접 구현 (외부 라이브러리 없음) |
| PDF 읽기·렌더링 | `Windows.Data.Pdf` WinRT API (Windows 10 내장) |
| 비동기→동기 | WRL(Windows Runtime C++ Template Library) + SyncWait 패턴 |
| 병렬 처리 | `std::thread` + `std::atomic<int>` lock-free 인덱스 분배 |
| 설치 파일 | Inno Setup 6.x |

---

## 프로젝트 구조

```
ImgToPdf/
├── ImgToPdf.sln / .vcxproj
├── resource.h                  ← 컨트롤 ID, 커스텀 메시지 정의
├── ImgToPdf.rc                 ← 다이얼로그 템플릿
├── pch.h / pch.cpp             ← 프리컴파일드 헤더
│
├── ImgToPdfDlg.h / .cpp        ← 메인 다이얼로그
├── FileListCtrl.h / .cpp       ← 커스텀 CListCtrl (상태 아이콘, 행 배경색, 순서 이동)
├── ImagePreviewCtrl.h / .cpp   ← GDI+ 미리보기 커스텀 CStatic
├── ProgressLabel.h / .cpp      ← 3색 카운터 텍스트 커스텀 CStatic
├── ConvertWorker.h / .cpp      ← 변환 스레드 풀
├── PdfWriter.h / .cpp          ← PDF 1.4 스트림 생성
├── PdfConverter.h / .cpp       ← WinRT 기반 PDF 읽기 및 JPG 변환
│
├── installer/
│   └── ImgToPdf_setup.iss      ← Inno Setup 설치 스크립트
│
└── third_party/
    └── stb_image.h
```

---

## 지원 파일 형식

### 입력

| 종류 | 확장자 |
|------|--------|
| 이미지 | `.jpg` `.jpeg` `.png` `.bmp` `.tiff` `.tif` `.gif` |
| PDF | `.pdf` |

### 출력

| 입력 | 출력 |
|------|------|
| 이미지 (개별) | `원본명.pdf` |
| 이미지 (합치기) | `원본명_통합.pdf` |
| PDF 페이지 | `원본명_p001.jpg`, `원본명_p002.jpg`, … |

---

## 제작자

- **jaeho**
- jaeho9697@gmail.com

---

## 라이선스

MIT License
