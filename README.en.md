# ImgToPdf

> Multi-function file conversion desktop app for Windows — Image ↔ PDF · PDF Tools · Markdown · Word · PPT Conversion

[![Platform](https://img.shields.io/badge/platform-Windows%2010%2B-blue)](https://www.microsoft.com/windows)
[![Language](https://img.shields.io/badge/language-C%2B%2B17%20%2F%20MFC-orange)](https://docs.microsoft.com/cpp)
[![Version](https://img.shields.io/badge/version-2.1-green)](https://github.com)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)

[한국어 README →](README.md)

---

## Overview

ImgToPdf is a file conversion tool that runs entirely on built-in Windows 10 APIs — no external libraries required.  
Five tabs handle image, PDF, Markdown, Word, and PPT conversions in a single window, with runtime Korean/English language switching.  
Tab 2 includes **local AI summarization via Ollama** — no internet connection or API key needed.

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│  ImgToPdf v2.3                                                      [─][□][✕]     │
├────────────────────────────────────────────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  [◀] [▶]  │
│  │  Image   │  │   PDF    │  │   MD     │  │   Word   │  │   PPT    │            │
│  │ Convert  │  │  Tools   │  │ Convert  │  │ Convert  │  │ Convert  │            │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  └──────────┘            │
│ ┌──────────────────────────────────────────────────────────────────────────────┐   │
│ │                        Active tab child dialog area                          │   │
│ └──────────────────────────────────────────────────────────────────────────────┘   │
│  Path: [___________________________________]  [Browse]  [Convert]                  │
│  [██████████░░░░░]  (3 / 1 / 10)  [▲] [▼] [Delete]                                │
└────────────────────────────────────────────────────────────────────────────────────┘
```

![Screenshot](imgtopdf.png)

---

## Features

### Tab 1 — Image Convert

| Feature | Description |
|---------|-------------|
| Image → PDF | JPG · PNG · BMP · TIFF · GIF → individual PDFs or a merged multi-page PDF |
| PDF → JPG | Extract each PDF page as a JPG (quality 90, via WinRT `Windows.Data.Pdf`) |
| Drag & Drop | Drop files or folders; duplicates are automatically ignored |
| Parallel conversion | Worker thread pool based on `hardware_concurrency()` |
| Image preview | Click a list item to show a GDI+ aspect-ratio-fit preview |
| Reorder | ▲▼ buttons to reorder conversion sequence |
| Progress | Progress bar + 3-color counter `(Done / Failed / Total)` in real time |
| Delete dual mode | Before conversion: remove selected / After conversion: full reset |

### Tab 2 — PDF Tools

| Feature | Description |
|---------|-------------|
| Split | Split a PDF page by page, or by specified ranges (e.g. `1-3,5,7-9`) |
| Merge | Combine multiple PDFs into one (use ▲▼ to reorder) |
| Extract | Extract specific pages → single PDF or one PDF per page |
| **AI Summary** | OCR-extract text via WinRT, then summarize with local Ollama (llama3.2:3b) |
| Open in Notepad | Open the summary result in Notepad for saving or editing |
| Summary language | Radio buttons to select Korean or English summary output |

### Tab 3 — MD Convert

| Feature | Description |
|---------|-------------|
| HTML | Markdown → UTF-8 BOM HTML file |
| PDF | Markdown → PDF (RTF rendering → A4 page layout) |
| HTML + PDF | Generate both HTML and PDF simultaneously |
| Preview | Click a list item to see a converted preview in RichEdit |

### Tab 4 — Word Convert

| Feature | Description |
|---------|-------------|
| Word → PDF | Convert `.doc` / `.docx` → PDF |
| Auto engine detection | Registry scan for Word then LibreOffice; uses whichever is installed |
| Microsoft Word | COM IDispatch: `Presentations.Open` → `SaveAs(PDF)` → `Close` |
| LibreOffice | Runs `soffice.exe --headless --convert-to pdf` as a subprocess |
| Drag & Drop | Drop files directly onto the list |
| Preview | Shell thumbnail or file icon shown on selection |

### Tab 5 — PPT Convert

| Feature | Description |
|---------|-------------|
| PPT → PDF | Convert `.ppt` / `.pptx` → PDF |
| Auto engine detection | Registry scan for PowerPoint then LibreOffice |
| Microsoft PowerPoint | COM IDispatch: `SaveAs(ppSaveAsPDF=32)`; preserves original slide dimensions |
| LibreOffice | Runs `soffice.exe --headless --convert-to pdf` as a subprocess |
| Drag & Drop | Drop files directly onto the list |
| Preview | Shell thumbnail or file icon shown on selection |

### Common Features

| Feature | Description |
|---------|-------------|
| Tab reorder | ◀▶ buttons to move tabs left/right; order is saved across restarts |
| Runtime language | System menu → `Switch to English` / `한국어로 전환` (no restart needed) |
| In-app help | F1 key or system menu → usage guide dialog |
| Unicode path support | Full support for non-ASCII characters in all file paths |
| Right-click menu | Remove item · Open file location · Open file · Clear all |

---

## Requirements

| Item | Minimum |
|------|---------|
| OS | Windows 10 version 1803 or later |
| Build tool | Visual Studio 2022 (v143 toolset) |
| Windows SDK | 10.0 or later |
| Runtime | Visual C++ Redistributable (MFC Dynamic) |
| AI Summary (optional) | [Ollama](https://ollama.com) + `llama3.2:3b` model (runs locally, no internet required) |

---

## Installation

### Installer (recommended)

1. Download `ImgToPdf_Setup_v2.3.exe` from the [Releases](https://github.com/JaeHoYang/ImgToPdf/releases) page
2. Run the installer and follow the wizard
3. On the **App Language** step, select `한국어 (Korean)` or `English`
4. After installation, confirm whether to **auto-install Ollama** → downloads and installs automatically if agreed
5. Any previous version is automatically removed before installation

> No administrator privileges required.

### Build from source

```bash
# Release x64 build
msbuild ImgToPdf.vcxproj /p:Configuration=Release /p:Platform=x64
```

Or open `ImgToPdf.sln` in Visual Studio 2022 and build with `Release | x64`.

#### Build the installer

1. Install [Inno Setup 6.x](https://jrsoftware.org/isdl.php)
2. Confirm `Release x64` build is complete
3. Open `installer\ImgToPdf_setup.iss` in Inno Setup Compiler → press **F9**
4. Output: `installer\ImgToPdf_Setup_v2.3.exe`

> If `mfc142u.dll`, `msvcp140.dll`, `vcruntime140.dll`, and `vcruntime140_1.dll` are present in `x64\Release`, they are bundled automatically.

---

## Usage

### Image → PDF

1. Drag & drop image files or click **[Browse]** to add them
2. *(Optional)* Check **Merge** → combine all images into a single multi-page PDF
3. *(Optional)* Use **▲▼** to reorder
4. Click **[Convert]** → saved in the same folder as the source files

### PDF → JPG

1. Add a PDF file → pages are scanned automatically
2. Click **[Convert]** → output: `filename_p001.jpg`, `filename_p002.jpg`, …

### PDF Split / Merge / Extract (Tab 2)

- **Split**: Add a PDF → choose page-by-page or enter ranges (e.g. `1-3,5,7`) → **[Run]**
- **Merge**: Add multiple PDFs → reorder with ▲▼ → enter output name → **[Run]**
- **Extract**: Add a PDF → enter page numbers (e.g. `1,3,5-7`) → **[Run]**

### AI Summary (Tab 2)

> Ollama must be running locally (`http://localhost:11434`).

1. Select a PDF in the list
2. Choose summary language: `● Korean` or `○ English`
3. Click **[Summarize]** → OCR extracts text, Ollama generates a structured summary
4. Click **[Notepad]** to open the result in Notepad for saving or editing

Summary output format:
```
[Overview]
(4-6 sentence summary)

[Key Points]
• Point 1
• Point 2  …

[Conclusion]
(3-4 sentences)
```

#### Ollama Setup (one-time)

```bash
# 1. Install Ollama from https://ollama.com (or let the app installer do it)
# 2. Download the model
ollama pull llama3.2:3b
```

### MD Convert (Tab 3)

1. Add `.md` or `.markdown` files
2. Select output format: `HTML` / `PDF` / `HTML+PDF`
3. *(Optional)* Specify output folder (leave blank to save beside source)
4. Click **[Convert]**

### Word Convert (Tab 4)

> Microsoft Word or LibreOffice must be installed.

1. Drag & drop `.doc` / `.docx` files or click **[Browse]** to add them
2. *(Optional)* Specify output folder (leave blank to save beside source)
3. Click **[Convert]** → PDF saved

### PPT Convert (Tab 5)

> Microsoft PowerPoint or LibreOffice must be installed.

1. Drag & drop `.ppt` / `.pptx` files or click **[Browse]** to add them
2. *(Optional)* Specify output folder (leave blank to save beside source)
3. Click **[Convert]** → PDF saved

### Language Switch

Click `─` (system menu, top-left) → **Switch to English** → switches instantly (no restart)

---

## Build Options

| Item | Value |
|------|-------|
| C++ standard | C++17 |
| Character set | Unicode |
| MFC | Dynamic (`UseOfMfc=Dynamic`) |
| Extra compile flags | `/utf-8` |
| Link libraries | `gdiplus.lib` `shell32.lib` `runtimeobject.lib` `shcore.lib` |
| ComCtl32 | v6 (activated via linker manifest pragma) |

---

## Tech Stack

| Role | Technology |
|------|------------|
| UI framework | MFC `CDialogEx` / `CTabCtrl` / child dialogs |
| Image loading | [`stb_image.h`](https://github.com/nothings/stb) (header-only, third-party) |
| Image rendering | GDI+ (`Gdiplus::Bitmap`) |
| PDF generation | Hand-written PDF 1.4 stream (no external library) |
| PDF reading/rendering | `Windows.Data.Pdf` WinRT API (built into Windows 10) |
| WinRT wrapping | WRL + SyncWait pattern (async → sync) |
| MD conversion | Built-in `MdConverter` — Markdown → RTF → `EM_FORMATRANGE` A4 layout |
| Parallel processing | `std::thread` + `std::atomic<int>` lock-free index dispatch |
| Language switching | C++ `wchar_t*` arrays (`s_ko[]`/`s_en[]`) + `LS()` helper |
| AI Summary (OCR) | `Windows.Media.Ocr` WinRT API (built into Windows 10) |
| AI Summary (LLM) | WinHTTP → Ollama REST API (`llama3.2:3b`, local) |
| Installer | Inno Setup 6.x (with Ollama auto-install support) |

---

## Project Structure

```
ImgToPdf/
├── ImgToPdf.sln
├── ImgToPdf.vcxproj
├── ImgToPdf.vcxproj.filters    ← Solution Explorer folder structure
│
├── [App Entry]
│   ├── ImgToPdf.h / .cpp       ← App entry point, GDI+ initialization
│   ├── pch.h / .cpp            ← Precompiled header
│   └── targetver.h
│
├── [Main Dialog]
│   ├── ImgToPdfDlg.h / .cpp    ← Main dialog (tab container)
│   └── TabDlgBase.h            ← Common tab interface
│
├── [Tab Dialogs]
│   ├── ImgConvertDlg.h / .cpp  ← Tab 1: Image ↔ PDF
│   ├── PdfToolsDlg.h / .cpp    ← Tab 2: PDF split/merge/extract + AI summary
│   ├── MdConvertDlg.h / .cpp   ← Tab 3: Markdown → HTML/PDF
│   ├── WordConvertDlg.h / .cpp ← Tab 4: Word → PDF (Word/LibreOffice COM)
│   └── PptConvertDlg.h / .cpp  ← Tab 5: PPT → PDF (PowerPoint/LibreOffice)
│
├── [Conversion Engine]
│   ├── ConvertWorker.h / .cpp  ← Image conversion thread pool
│   ├── PdfWriter.h / .cpp      ← PDF 1.4 stream writer
│   ├── PdfConverter.h / .cpp   ← WinRT PDF reader / JPG renderer
│   └── MdConverter.h / .cpp    ← Markdown → RTF/HTML converter
│
├── [UI Controls]
│   ├── FileListCtrl.h / .cpp   ← Custom CListCtrl (status icons, row colors)
│   ├── ImagePreviewCtrl.h / .cpp ← GDI+ image preview
│   ├── ProgressLabel.cpp       ← 3-color counter label
│   └── ColorButton.h           ← Custom color button
│
├── [Language & Messages]
│   ├── AppLang.h / .cpp        ← Runtime language switching (LS, BuildFilter)
│   ├── AppMessages.h           ← Custom WM_USER message constants
│   └── resource.h              ← Control IDs, dialog IDs
│
├── [Resources]
│   ├── ImgToPdf.rc             ← Dialog templates
│   └── ImgToPdf.ico
│
├── installer/
│   └── ImgToPdf_setup.iss      ← Inno Setup script
│
└── third_party/
    └── stb_image.h
```

---

## Supported Formats

### Input

| Type | Extensions |
|------|------------|
| Image | `.jpg` `.jpeg` `.png` `.bmp` `.tiff` `.tif` `.gif` |
| PDF | `.pdf` |
| Markdown | `.md` `.markdown` |
| Word | `.doc` `.docx` |
| PowerPoint | `.ppt` `.pptx` |

### Output

| Conversion | Output file |
|------------|-------------|
| Image → PDF (individual) | `filename.pdf` |
| Image → PDF (merged) | `filename_merged.pdf` |
| PDF → JPG | `filename_p001.jpg`, `_p002.jpg`, … |
| PDF split (page-by-page) | `filename_p001.pdf`, `_p002.pdf`, … |
| PDF split (by range) | `filename_range1.pdf`, `_range2.pdf`, … |
| PDF merge | `specified_name.pdf` |
| PDF extract (single) | `filename_extracted.pdf` |
| Markdown → HTML | `filename.html` |
| Markdown → PDF | `filename.pdf` |
| Word → PDF | `filename.pdf` |
| PPT → PDF | `filename.pdf` |

---

## Author

- **jaeho**
- pim9697@gmail.com

## Development

This project was built with **[Claude](https://claude.ai/code) (Anthropic)** via Vibe Coding,  
and the source code was reviewed by **[Gemini](https://gemini.google.com) (Google)**.

---

## License

[MIT License](LICENSE)
