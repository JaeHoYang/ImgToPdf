#include "pch.h"
#include "AppLang.h"

Lang g_lang = Lang::KO;

static const wchar_t* s_ko[IDS_APP_STRING_COUNT] = {
    // Common / Host (0-18)
    L"\uc774\ubbf8\uc9c0 \ubcc0\ud658",           // IDS_TAB1_LABEL
    L"PDF \ub3c4\uad6c",                           // IDS_TAB2_LABEL
    L"MD \ubcc0\ud658",                            // IDS_TAB3_LABEL
    L"\uc0ac\uc6a9 \ubc29\ubc95(&H)...",           // IDS_MENU_HELP
    L"\ubcc0\ud658\ud560 \ud30c\uc77c\uc758 \uacbd\ub85c\uac00 \ud45c\uc2dc\ub429\ub2c8\ub2e4.", // IDS_TIP_EDITPATH
    L"\uccb4\ud06c: \ubaa8\ub4e0 \uc774\ubbf8\uc9c0 \u2192 \ud558\ub098\uc758 \ub2e4\uc911 \ud398\uc774\uc9c0 PDF\n\ubbf8\uccb4\ud06c: \uac01 \uc774\ubbf8\uc9c0 \u2192 \uac1c\ubcc4 PDF \ud30c\uc77c\n(PDF \ud56d\ubaa9 \ud3ec\ud568 \uc2dc \uc790\ub3d9 \ube44\ud65c\uc131)", // IDS_TIP_MERGE
    L"\ud30c\uc77c\uc744 \uc120\ud0dd\ud569\ub2c8\ub2e4.\n\ub2e4\uc911 \uc120\ud0dd \uc9c0\uc6d0 | \ucc3d\uc5d0 \ub4dc\ub798\uadf8\uc564\ub4dc\ub86d\ub3c4 \uac00\ub2a5", // IDS_TIP_BROWSE
    L"\ubcc0\ud658/\uc2e4\ud589 \uc2dc\uc791\n\uc2e4\ud589 \uc911 \ud074\ub9ad \uc2dc \uc911\ub2e8", // IDS_TIP_CONVERT
    L"(\uc644\ub8cc / \uc2e4\ud328 / \ucd1d\uacc4)\n\uc644\ub8cc: \ucd08\ub85d  \uc2e4\ud328: \ube68\uac15  \ucd1d\uacc4: \uae30\ubcf8\uc0c9", // IDS_TIP_PROGRESS
    L"\uc120\ud0dd\ud55c \ud56d\ubaa9\uc744 \uc704\ub85c \uc774\ub3d9", // IDS_TIP_MOVEUP
    L"\uc120\ud0dd\ud55c \ud56d\ubaa9\uc744 \uc544\ub798\ub85c \uc774\ub3d9", // IDS_TIP_MOVEDOWN
    L"\uc120\ud0dd \ud56d\ubaa9\uc744 \ubaa9\ub85d\uc5d0\uc11c \uc81c\uac70\ud569\ub2c8\ub2e4.", // IDS_TIP_CLEAR_BASE
    L"F1\uc744 \ub204\ub974\uba74 \uc0ac\uc6a9 \ubc29\ubc95\uc774 \ub098\uc635\ub2c8\ub2e4.", // IDS_CUE_PATH
    // IDS_HELP_TEXT (long)
    L"ImgToPdf v2.1 \u2014 \ub2e4\uae30\ub2a5 \ud30c\uc77c \ubcc0\ud658 \ub3c4\uad6c\n"
    L"\n"
    L"[ \uacf5\ud1b5 \ucf58\ud2b8\ub864 ]\n"
    L"\uc0c1\ub2e8 [\uc0c3\uae30]: \ud604\uc7ac \ud0ed\uc5d0 \ud30c\uc77c\uc744 \ucd94\uac00\ud569\ub2c8\ub2e4.\n"
    L"\uc0c1\ub2e8 [\ubcc0\ud658/\uc2e4\ud589]: \ud604\uc7ac \ud0ed\uc758 \uc791\uc5c5\uc744 \uc2e4\ud589\ud569\ub2c8\ub2e4.\n"
    L"[\u25b2][\u25bc]: \uc120\ud0dd \ud56d\ubaa9 \uc21c\uc11c \ubcc0\uacbd  [\uc0ad\uc81c]: \uc120\ud0dd \ud56d\ubaa9 \uc81c\uac70\n"
    L"\ubaa9\ub85d \ub354\ube14\ud074\ub9ad \u2192 \ud574\ub2f9 \ud30c\uc77c\uc744 \ubc14\ub85c \uc5f4\uc5b4 \ud655\uc778\ud569\ub2c8\ub2e4.\n"
    L"\ubaa9\ub85d \uc6b0\ud074\ub9ad \u2192 \ud56d\ubaa9 \uc81c\uac70 \u00b7 \ud30c\uc77c \uc704\uce58 \uc5f4\uae30 \u00b7 \ud30c\uc77c \uc5f4\uae30 \u00b7 \uc804\uccb4 \uc0ad\uc81c\n"
    L"\n"
    L"[ \ud0ed1: \uc774\ubbf8\uc9c0 \ubcc0\ud658 ]\n"
    L"JPG/PNG/BMP/TIFF/GIF \u2192 PDF \ubcc0\ud658.\n"
    L"\ud30c\uc77c\uc740 \ub4dc\ub798\uadf8\uc564\ub4dc\ub86d \ub610\ub294 [\uc0c1\uae30] \ubc84\ud2bc\uc73c\ub85c \ucd94\uac00\ud558\uc138\uc694.\n"
    L"\ub354\ube14\ud074\ub9ad: \uc2dc\uc2a4\ud15c \uae30\ubcf8 \uc774\ubbf8\uc9c0 \ubdf0\uc5b4\ub85c \ud30c\uc77c\uc744 \uc5fd\ub2c8\ub2e4.\n"
    L"\ud569\uce58\uae30 \uccb4\ud06c \u2192 \ubaa8\ub4e0 \uc774\ubbf8\uc9c0\ub97c \ud558\ub098\uc758 \ub2e4\uc911 \ud398\uc774\uc9c0 PDF\ub85c \ud569\uce58.\n"
    L"\ucd9c\ub825 \uacbd\ub85c: \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.\n"
    L"\n"
    L"[ \ud0ed2: PDF \ub3c4\uad6c ]\n"
    L"\ub098\ub204\uae30: PDF\ub97c \ud398\uc774\uc9c0\ubcc4 \ub610\ub294 \ubc94\uc704\ubcc4\ub85c \ubd84\ud560.\n"
    L"\ud569\uce58\uae30: \uc5ec\ub7ec PDF\ub97c \ud558\ub098\ub85c \ubcd1\ud569 (\u25b2\u25bc\ub85c \uc21c\uc11c \uc870\uc815).\n"
    L"\ud398\uc774\uc9c0 \ucd94\ucd9c: \uc9c0\uc815 \ud398\uc774\uc9c0\ub9cc \ucd94\ucd9c (\uc608: 1,3,5-7,10).\n"
    L"\ub354\ube14\ud074\ub9ad: \uc2dc\uc2a4\ud15c \uae30\ubcf8 PDF \ubdf0\uc5b4\ub85c \ud30c\uc77c\uc744 \uc5fd\ub2c8\ub2e4.\n"
    L"\ucd9c\ub825 \uacbd\ub85c: \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.\n"
    L"AI \uc694\uc57d: OCR \ud14d\uc2a4\ud2b8 \ucd94\ucd9c \ud6c4 Ollama(llama3.2:3b)\ub85c \uad6c\uc870\ud654 \uc694\uc57d \uc0dd\uc131 (\uba54\ubaa8\uc7a5\uc73c\ub85c \uc5f4\uae30).\n"
    L"\n"
    L"[ \ud0ed3: MD \ubcc0\ud658 ]\n"
    L"Markdown \u2192 HTML / PDF / HTML+PDF \ubcc0\ud658.\n"
    L"HTML+PDF: HTML\uacfc PDF \ud30c\uc77c\uc744 \ub3d9\uc2dc\uc5d0 \uc0dd\uc131\ud569\ub2c8\ub2e4.\n"
    L"\ub354\ube14\ud074\ub9ad: \uba54\ubaa8\uc7a5\uc5d0\uc11c \ud574\ub2f9 MD \ud30c\uc77c\uc744 \uc5fd\ub2c8\ub2e4.\n"
    L"\ucd9c\ub825 \uacbd\ub85c: \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.\n"
    L"\n"
    L"[ \ud0ed4: Word \ubcc0\ud658 ]\n"
    L".doc/.docx \u2192 PDF \ubcc0\ud658.\n"
    L"\ubcc0\ud658 \uc5d4\uc9c4: Microsoft Word \ub610\ub294 LibreOffice (\ub458 \uc911 \ud558\ub098 \ud544\uc694).\n"
    L"\ub354\ube14\ud074\ub9ad: \ud30c\uc77c\uc744 \uc678\ubd80 \uc571\uc73c\ub85c \uc5fd\ub2c8\ub2e4.\n"
    L"\ucd9c\ub825 \ud3f4\ub354: \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.\n"
    L"\n"
    L"[ \ud0ed5: PPT \ubcc0\ud658 ]\n"
    L".ppt/.pptx \u2192 PDF \ubcc0\ud658.\n"
    L"\ubcc0\ud658 \uc5d4\uc9c4: Microsoft PowerPoint \ub610\ub294 LibreOffice (\ub458 \uc911 \ud558\ub098 \ud544\uc694).\n"
    L"\ub354\ube14\ud074\ub9ad: \ud30c\uc77c\uc744 \uc678\ubd80 \uc571\uc73c\ub85c \uc5fd\ub2c8\ub2e4.\n"
    L"\ucd9c\ub825 \ud3f4\ub354: \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.\n"
    L"\n"
    L"[ \ud0ed \uc21c\uc11c \ubcc0\uacbd ]\n"
    L"\u25c4 \u25ba \ubc84\ud2bc\uc73c\ub85c \ud604\uc7ac \ud0ed\uc744 \uc88c\uc6b0\ub85c \uc774\ub3d9. \uc21c\uc11c\ub294 \uc7ac\uc2dc\uc791 \ud6c4\uc5d0\ub3c4 \uc720\uc9c0\ub429\ub2c8\ub2e4.\n"
    L"\n"
    L"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
    L"\uc81c\uc791\uc790  jaeho\n"
    L"\ubb38\uc758    jaeho9697@gmail.com",
    L"\uc0ac\uc6a9 \ubc29\ubc95",                  // IDS_HELP_CAPTION
    L"\uc644\ub8cc",                               // IDS_PROGRESS_DONE
    L"\uc624\ub958",                               // IDS_PROGRESS_ERROR
    L"\ucd9c\ub825 \ud3f4\ub354 \uc120\ud0dd",    // IDS_FOLDER_SELECT
    L"\ubaa8\ub4e0 \ud30c\uc77c",                  // IDS_FILTER_ALL_LABEL

    // Tab1 - ImgConvert (19-32)
    L"\ud30c\uc77c \ubaa9\ub85d\n\u2022 \ub4dc\ub798\uadf8\uc564\ub4dc\ub86d \ub610\ub294 [\uc0c1\uae30]\ub85c \ucd94\uac00\n\u2022 \ud074\ub9ad: \ubbf8\ub9ac\ubcf4\uae30 \ud45c\uc2dc\n\u2022 Delete \ud0a4: \uc120\ud0dd \ud56d\ubaa9 \uc81c\uac70", // IDS_IMG_TIP_LIST
    L"\ubaa9\ub85d\uc5d0\uc11c \uc774\ubbf8\uc9c0 \ud56d\ubaa9\uc744 \ud074\ub9ad\ud558\uba74 \ubbf8\ub9ac\ubcf4\uae30\uac00 \ud45c\uc2dc\ub429\ub2c8\ub2e4.", // IDS_IMG_TIP_PREVIEW
    L"\ucd9c\ub825 \ud3f4\ub354 \uacbd\ub85c. \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.", // IDS_IMG_TIP_OUTPUT
    L"\ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.", // IDS_IMG_CUE_OUTPUT
    L"\uc774\ubbf8\uc9c0 \ud30c\uc77c",            // IDS_IMG_FILTER_LABEL
    L"\ucd9c\ub825 \ud3f4\ub354\uac00 \uc874\uc7ac\ud558\uc9c0 \uc54a\uc2b5\ub2c8\ub2e4.\n\ud3f4\ub354 \uacbd\ub85c\ub97c \ud655\uc778\ud558\uac70\ub098 \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.", // IDS_IMG_ERR_OUTDIR
    L"\ucd9c\ub825 \ud3f4\ub354 \uc624\ub958",     // IDS_IMG_ERR_TITLE
    L"\ubaa9\ub85d\uacfc \uc9c4\ud589 \uc0c1\ud0dc\ub97c \ubaa8\ub450 \ucd08\uae30\ud654\ud569\ub2c8\ub2e4.\n\uacc4\uc18d\ud558\uc2dc\uaca0\uc2b5\ub2c8\uae4c?", // IDS_IMG_CONFIRM_RESET
    L"\ucd08\uae30\ud654 \ud655\uc778",            // IDS_IMG_CONFIRM_TITLE
    L"\uc911\ub2e8",                               // IDS_IMG_BTN_STOP
    L"\ubcc0\ud658",                               // IDS_IMG_BTN_CONVERT
    L"\ubaa9\ub85d\u00b7\uc9c4\ud589 \uc0c1\ud0dc\ub97c \uc804\uccb4 \ucd08\uae30\ud654\ud569\ub2c8\ub2e4.", // IDS_IMG_CLEAR_DONE
    L"\uc120\ud0dd\ud55c \ud56d\ubaa9\uc744 \ubaa9\ub85d\uc5d0\uc11c \uc81c\uac70\ud569\ub2c8\ub2e4.", // IDS_IMG_CLEAR_SEL
    L"\ud56d\ubaa9\uc744 \uc120\ud0dd\ud558\uba74 \uc81c\uac70\ud560 \uc218 \uc788\uc2b5\ub2c8\ub2e4.\n\ubcc0\ud658 \uc644\ub8cc \ud6c4\uc5d0\ub294 \uc804\uccb4 \ucd08\uae30\ud654\ud569\ub2c8\ub2e4.", // IDS_IMG_CLEAR_NONE

    // Tab2 - PdfTools (33-64)
    L"\ud30c\uc77c\uba85",                         // IDS_PDF_COL0
    L"\ud398\uc774\uc9c0\uc218",                   // IDS_PDF_COL1
    L"\ubcc0\ud658\ub420\ud30c\uc77c\uba85",       // IDS_PDF_COL2
    L"\ube44\uace0",                               // IDS_PDF_COL3
    L"\ucd94\uac00\ud560 PDF \ud30c\uc77c \uc704\uce58", // IDS_PDF_CUE_PATH
    L"PDF \ud30c\uc77c",                           // IDS_PDF_FILTER_LABEL
    L"PDF \ud30c\uc77c \ubaa9\ub85d\n\u2022 \ub4dc\ub798\uadf8\uc564\ub4dc\ub86d \ub610\ub294 [\uc0c1\uae30]\ub85c \ucd94\uac00\n\u2022 \ub354\ube14\ud074\ub9ad: \uae30\ubcf8 PDF \ubdf0\uc5b4\ub85c \uc5f4\uae30\n\u2022 Delete \ud0a4: \uc120\ud0dd \ud56d\ubaa9 \uc81c\uac70", // IDS_PDF_TIP_LIST
    L"PDF\ub97c \ud398\uc774\uc9c0\ubcc4 \ub610\ub294 \ubc94\uc704\ubcc4\ub85c \ubd84\ud560\ud569\ub2c8\ub2e4.", // IDS_PDF_TIP_SPLIT
    L"\uc5ec\ub7ec PDF\ub97c \ud558\ub098\ub85c \ubcd1\ud569\ud569\ub2c8\ub2e4. (\u25b2\u25bc\ub85c \uc21c\uc11c \uc870\uc815)", // IDS_PDF_TIP_MERGE_OP
    L"PDF\uc5d0\uc11c \uc9c0\uc815\ud55c \ud398\uc774\uc9c0\ub9cc \ucd94\ucd9c\ud569\ub2c8\ub2e4.", // IDS_PDF_TIP_EXTRACT
    L"\uac01 \ud398\uc774\uc9c0\ub97c \uac1c\ubcc4 PDF\ub85c \ubd84\ud560\ud569\ub2c8\ub2e4.", // IDS_PDF_TIP_SPLIT_EACH
    L"\uc9c0\uc815\ud55c \ubc94\uc704\ubcc4\ub85c PDF\ub97c \ubd84\ud560\ud569\ub2c8\ub2e4.", // IDS_PDF_TIP_SPLIT_RANGE
    L"\ubc94\uc704 \uc608: 1-3,5,7-9 (\ucf64\ub9c8\ub85c \uad6c\ubd84)", // IDS_PDF_TIP_RANGE_EDIT
    L"\ubcd1\ud569 \ud6c4 \uc800\uc7a5\ud560 PDF \ud30c\uc77c\uba85\uc744 \uc785\ub825\ud558\uc138\uc694.", // IDS_PDF_TIP_MERGENAME
    L"\ucd94\ucd9c\ud560 \ud398\uc774\uc9c0 \ubc94\uc704. \uc608: 1,3,5-7,10", // IDS_PDF_TIP_EXTRACT_PAGES
    L"\ucd94\ucd9c\ud55c \ud398\uc774\uc9c0\ub97c \ud558\ub098\uc758 PDF\ub85c \uc800\uc7a5\ud569\ub2c8\ub2e4.", // IDS_PDF_TIP_EXT_SINGLE
    L"\ucd94\ucd9c\ud55c \uac01 \ud398\uc774\uc9c0\ub97c \uac1c\ubcc4 PDF\ub85c \uc800\uc7a5\ud569\ub2c8\ub2e4.", // IDS_PDF_TIP_EXT_EACH
    L"\ucd9c\ub825 \ud3f4\ub354 \uacbd\ub85c. \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.", // IDS_PDF_TIP_OUTPUT
    L"\ucd9c\ub825 \ud3f4\ub354\ub97c \uc120\ud0dd\ud569\ub2c8\ub2e4.",   // IDS_PDF_TIP_BROWSE
    L"\uc791\uc5c5 \uc911...",                     // IDS_PDF_STATUS_WORKING
    L"\uc644\ub8cc",                               // IDS_PDF_STATUS_DONE
    L"\uc2e4\ud328",                               // IDS_PDF_STATUS_FAIL
    L"\uc624\ub958\uac00 \ubc1c\uc0dd\ud588\uc2b5\ub2c8\ub2e4. \ud30c\uc77c \uacbd\ub85c\ub97c \ud655\uc778\ud558\uc138\uc694.", // IDS_PDF_STATUS_ERROR
    L"\ud398\uc774\uc9c0 \ubc94\uc704\uac00 \uc798\ubabb\ub418\uc5c8\uc2b5\ub2c8\ub2e4. \uc608) 1,3,5-7,10", // IDS_PDF_STATUS_RANGE_ERR
    L"\uc2e4\ud589",                               // IDS_PDF_BTN_RUN
    L"\uc2e4\ud589 \uc911...",                     // IDS_PDF_BTN_RUNNING
    L"\ubaa9\ub85d\uc5d0\uc11c \uc81c\uac70",     // IDS_PDF_MENU_REMOVE
    L"\ud30c\uc77c \uc704\uce58 \uc5f4\uae30",    // IDS_PDF_MENU_OPEN_LOC
    L"\ud30c\uc77c \uc5f4\uae30",                  // IDS_PDF_MENU_OPEN
    L"\uc804\uccb4 \uc0ad\uc81c",                  // IDS_PDF_MENU_CLEAR_ALL
    L"\uc644\ub8cc",                               // IDS_PDF_REMARK_DONE
    L"\uc2e4\ud328",                               // IDS_PDF_REMARK_FAIL

    // Tab3 - MdConvert (65-101)
    L"\ud30c\uc77c\uba85",                         // IDS_MD_COL0
    L"\ubcc0\ud658\ub420 \ud30c\uc77c\uba85",      // IDS_MD_COL1
    L"\ube44\uace0",                               // IDS_MD_COL2
    L"MD \ud30c\uc77c \uc704\uce58",               // IDS_MD_CUE_PATH
    L"Markdown \ud30c\uc77c",                      // IDS_MD_FILTER_LABEL
    L"MD \ud30c\uc77c \ubaa9\ub85d\n\u2022 \ub4dc\ub798\uadf8\uc564\ub4dc\ub86d \ub610\ub294 [\uc0c1\uae30]\ub85c \ucd94\uac00\n\u2022 \ud074\ub9ad: \ubbf8\ub9ac\ubcf4\uae30 \ud45c\uc2dc\n\u2022 \ub354\ube14\ud074\ub9ad: \uba54\ubaa8\uc7a5\uc73c\ub85c \uc5f4\uae30\n\u2022 Delete \ud0a4: \uc120\ud0dd \ud56d\ubaa9 \uc81c\uac70", // IDS_MD_TIP_LIST
    L"\ubaa9\ub85d\uc5d0\uc11c MD \ud30c\uc77c\uc744 \ud074\ub9ad\ud558\uba74 \ubcc0\ud658 \ubbf8\ub9ac\ubcf4\uae30\uac00 \ud45c\uc2dc\ub429\ub2c8\ub2e4.", // IDS_MD_TIP_PREVIEW
    L"Markdown\uc744 HTML \ud30c\uc77c\ub85c \ubcc0\ud658\ud569\ub2c8\ub2e4.", // IDS_MD_TIP_HTML
    L"Markdown\uc744 PDF \ud30c\uc77c\ub85c \ubcc0\ud658\ud569\ub2c8\ub2e4.", // IDS_MD_TIP_PDF
    L"HTML\uacfc PDF \ud30c\uc77c\uc744 \ub3d9\uc2dc\uc5d0 \uc0dd\uc131\ud569\ub2c8\ub2e4.", // IDS_MD_TIP_HTMLPDF
    L"\ucd9c\ub825 \ud3f4\ub354 \uacbd\ub85c. \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.", // IDS_MD_TIP_OUTPUT
    L"\ucd9c\ub825 \ud3f4\ub354\ub97c \uc120\ud0dd\ud569\ub2c8\ub2e4.",   // IDS_MD_TIP_BROWSE
    L"\ucd9c\ub825 \ud3f4\ub354\uac00 \uc874\uc7ac\ud558\uc9c0 \uc54a\uc2b5\ub2c8\ub2e4.\n\ud3f4\ub354 \uacbd\ub85c\ub97c \ud655\uc778\ud558\uac70\ub098 \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.", // IDS_MD_ERR_OUTDIR
    L"\ucd9c\ub825 \ud3f4\ub354 \uc624\ub958",     // IDS_MD_ERR_TITLE
    L"\ubaa9\ub85d\uacfc \uc9c4\ud589 \uc0c1\ud0dc\ub97c \ubaa8\ub450 \ucd08\uae30\ud654\ud569\ub2c8\ub2e4.\n\uacc4\uc18d\ud558\uc2dc\uaca0\uc2b5\ub2c8\uae4c?", // IDS_MD_CONFIRM_RESET
    L"\ucd08\uae30\ud654 \ud655\uc778",            // IDS_MD_CONFIRM_TITLE
    L"\ub80c\ub354\ub9c1 \uc911...",               // IDS_MD_STATUS_RENDERING
    L"\ubcc0\ud658 \uc911...",                     // IDS_MD_STATUS_CONVERTING
    L"\ub300\uae30",                               // IDS_MD_STATUS_WAIT
    L"\ub80c\ub354\ub9c1",                         // IDS_MD_STATUS_RENDERING_ITEM
    L"\ubcc0\ud658 \uc911",                        // IDS_MD_STATUS_CONVERTING_ITEM
    L"\uc644\ub8cc",                               // IDS_MD_STATUS_DONE
    L"\uc2e4\ud328",                               // IDS_MD_STATUS_FAIL
    L"\uc77c\ubd80 \ud30c\uc77c \ubcc0\ud658 \uc2e4\ud328", // IDS_MD_STATUS_PARTIAL_FAIL
    L"HTML+PDF \ubcc0\ud658 \uc644\ub8cc",         // IDS_MD_STATUS_DONE_HTML_PDF
    L"PDF \ubcc0\ud658 \uc644\ub8cc",              // IDS_MD_STATUS_DONE_PDF
    L"HTML \ubcc0\ud658 \uc644\ub8cc",             // IDS_MD_STATUS_DONE_HTML
    L"\ubaa9\ub85d\uc5d0\uc11c \ud30c\uc77c\uc744 \ud074\ub9ad\ud558\uba74 \ubbf8\ub9ac\ubcf4\uae30\uac00 \ud45c\uc2dc\ub429\ub2c8\ub2e4.", // IDS_MD_PREVIEW_INIT
    L"\ud30c\uc77c\uc744 \uc77d\uc744 \uc218 \uc5c6\uc2b5\ub2c8\ub2e4.", // IDS_MD_PREVIEW_ERR
    L"\ubcc0\ud658",                               // IDS_MD_BTN_CONVERT
    L"\ubaa9\ub85d\u00b7\uc9c4\ud589 \uc0c1\ud0dc\ub97c \uc804\uccb4 \ucd08\uae30\ud654\ud569\ub2c8\ub2e4.", // IDS_MD_CLEAR_DONE
    L"\uc120\ud0dd\ud55c \ud56d\ubaa9\uc744 \ubaa9\ub85d\uc5d0\uc11c \uc81c\uac70\ud569\ub2c8\ub2e4.", // IDS_MD_CLEAR_SEL
    L"\ud56d\ubaa9\uc744 \uc120\ud0dd\ud558\uba74 \uc81c\uac70\ud560 \uc218 \uc788\uc2b5\ub2c8\ub2e4.", // IDS_MD_CLEAR_NONE
    L"\ubaa9\ub85d\uc5d0\uc11c \uc81c\uac70",     // IDS_MD_MENU_REMOVE
    L"\ud30c\uc77c \uc704\uce58 \uc5f4\uae30",    // IDS_MD_MENU_OPEN_LOC
    L"\uba54\ubaa8\uc7a5\uc73c\ub85c \uc5f4\uae30", // IDS_MD_MENU_OPEN_NOTEPAD
    L"\uc804\uccb4 \uc0ad\uc81c",                  // IDS_MD_MENU_CLEAR_ALL

    // FileListCtrl (102-112)
    L"\ud30c\uc77c\uba85",                         // IDS_FLC_COL1
    L"\ubcc0\ud658\ub420 \ud30c\uc77c\uba85",      // IDS_FLC_COL2
    L"\ube44\uace0",                               // IDS_FLC_COL3
    L"\uc131\uacf5",                               // IDS_FLC_STATUS_SUCCESS
    L"\uc2e4\ud328",                               // IDS_FLC_STATUS_FAIL
    L"\ubcc0\ud658 \uc911...",                     // IDS_FLC_STATUS_RUNNING
    L"_\ud1b5\ud569.pdf",                           // IDS_FLC_SUFFIX_MERGED
    L"\ubaa9\ub85d\uc5d0\uc11c \uc81c\uac70",     // IDS_FLC_MENU_REMOVE
    L"\ud30c\uc77c \uc704\uce58 \uc5f4\uae30",    // IDS_FLC_MENU_OPEN_LOC
    L"\ud30c\uc77c \uc5f4\uae30",                  // IDS_FLC_MENU_OPEN
    L"\uc804\uccb4 \uc0ad\uc81c",                  // IDS_FLC_MENU_CLEAR_ALL

    // Quality slider (113-114)
    L"\ud488\uc9c8: %d%%",                         // IDS_IMG_QUALITY_FMT
    L"\uc774\ubbf8\uc9c0 \ucca9\uc219 \ud488\uc9c8 (10~100)\nJPEG \ud30c\uc77c\uc740 \ubb34\uc190\uc2e4 \uc784\ubca0\ub4dc (\ud488\uc9c8 \ubb34\uad00)", // IDS_IMG_TIP_QUALITY

    // PDF thumbnail (115)
    L"PDF \uccab \ud398\uc774\uc9c0 \ubbf8\ub9ac\ubcf4\uae30\n\ud074\ub9ad\ud558\uba74 \ud655\ub300 \ubcf4\uae30", // IDS_PDF_TIP_THUMB

    // Tab4 - WordConvert (116-139)
    L"Word \ubcc0\ud658",                              // IDS_TAB4_LABEL
    L"\ud30c\uc77c\uba85",                             // IDS_WORD_COL0
    L"\uc5d4\uc9c4",                                   // IDS_WORD_COL1
    L"\uc0c1\ud0dc",                                   // IDS_WORD_COL2
    L"Word \ud30c\uc77c \uacbd\ub85c",                 // IDS_WORD_CUE_PATH
    L"Word \ubb38\uc11c",                              // IDS_WORD_FILTER_LABEL
    L"Word \ud30c\uc77c(.doc/.docx)\uc744 \ub04c\uc5b4\ub2e4 \ub193\uc73c\uc138\uc694\n\uc0c1\ub2e8 [\uc0c3\uae30] \ubc84\ud2bc\uc73c\ub85c\ub3c4 \ucd94\uac00 \uac00\ub2a5", // IDS_WORD_TIP_LIST
    L"\ucd9c\ub825 \ud3f4\ub354. \ube44\uc6cc\ub450\uba74 \uc6d0\ubcf8 \ud30c\uc77c\uacfc \uac19\uc740 \ud3f4\ub354\uc5d0 \uc800\uc7a5\ub429\ub2c8\ub2e4.", // IDS_WORD_TIP_OUTPUT
    L"\ucd9c\ub825 \ud3f4\ub354 \uc120\ud0dd",        // IDS_WORD_TIP_BROWSE
    L"Microsoft Word \ub610\ub294 LibreOffice\uac00 \uc124\uce58\ub418\uc9c0 \uc54a\uc558\uc2b5\ub2c8\ub2e4.\n\ubcc0\ud658\ud558\ub824\uba74 \ub458 \uc911 \ud558\ub098\uac00 \ud544\uc694\ud569\ub2c8\ub2e4.", // IDS_WORD_ERR_NO_ENGINE
    L"\ucd9c\ub825 \uacbd\ub85c\uac00 \uc874\uc7ac\ud558\uc9c0 \uc54a\uc2b5\ub2c8\ub2e4.",   // IDS_WORD_ERR_OUTDIR
    L"\ubcc0\ud658 \uc624\ub958",                      // IDS_WORD_ERR_TITLE
    L"\ubcc0\ud658 \uc911...",                         // IDS_WORD_STATUS_WORKING
    L"\uc644\ub8cc",                                   // IDS_WORD_STATUS_DONE
    L"\uc2e4\ud328",                                   // IDS_WORD_STATUS_FAIL
    L"\ubcc0\ud658",                                   // IDS_WORD_BTN_CONVERT
    L"\ubaa9\ub85d\uc5d0\uc11c \uc81c\uac70",         // IDS_WORD_MENU_REMOVE
    L"\ud30c\uc77c \uc704\uce58 \uc5f4\uae30",        // IDS_WORD_MENU_OPEN_LOC
    L"\ud30c\uc77c \uc5f4\uae30",                     // IDS_WORD_MENU_OPEN
    L"\uc804\uccb4 \uc0ad\uc81c",                     // IDS_WORD_MENU_CLEAR_ALL
    L"\ubaa9\ub85d\uacfc \ubcc0\ud658 \uacb0\uacfc\ub97c \ucd08\uae30\ud654\ud558\uc2dc\uaca0\uc2b5\ub2c8\uae4c?", // IDS_WORD_CONFIRM_RESET
    L"\ucd08\uae30\ud654",                             // IDS_WORD_CONFIRM_TITLE
    L"Microsoft Word",                                 // IDS_WORD_ENGINE_WORD
    L"LibreOffice",                                    // IDS_WORD_ENGINE_LIBRE

    // Tab5 - PptConvert (140-162)
    L"PPT 변환",                              // IDS_TAB5_LABEL
    L"파일명",                            // IDS_PPT_COL0
    L"엔진",                                  // IDS_PPT_COL1
    L"상태",                                  // IDS_PPT_COL2
    L"PPT 파일 경로",                 // IDS_PPT_CUE_PATH
    L"PPT 파일(.ppt/.pptx)을 끌어다 놓으세요\n상단 [찾기] 버튼으로도 추가 가능", // IDS_PPT_TIP_LIST
    L"출력 폴더. 비워두면 원본 파일과 같은 폴더에 저장됩니다.", // IDS_PPT_TIP_OUTPUT
    L"출력 폴더 선택",        // IDS_PPT_TIP_BROWSE
    L"Microsoft PowerPoint 또는 LibreOffice가 설치되지 않았습니다.\n변환하려면 둘 중 하나가 필요합니다.", // IDS_PPT_ERR_NO_ENGINE
    L"출력 경로가 존재하지 않습니다.",   // IDS_PPT_ERR_OUTDIR
    L"변환 오류",                      // IDS_PPT_ERR_TITLE
    L"변환 중...",                         // IDS_PPT_STATUS_WORKING
    L"완료",                                   // IDS_PPT_STATUS_DONE
    L"실패",                                   // IDS_PPT_STATUS_FAIL
    L"변환",                                   // IDS_PPT_BTN_CONVERT
    L"목록에서 제거",         // IDS_PPT_MENU_REMOVE
    L"파일 위치 열기",        // IDS_PPT_MENU_OPEN_LOC
    L"파일 열기",                     // IDS_PPT_MENU_OPEN
    L"전체 삭제",                     // IDS_PPT_MENU_CLEAR_ALL
    L"목록과 변환 결과를 초기화하시겠습니까?", // IDS_PPT_CONFIRM_RESET
    L"초기화",                             // IDS_PPT_CONFIRM_TITLE
    L"Microsoft PowerPoint",                           // IDS_PPT_ENGINE_PPT
    L"LibreOffice",                                    // IDS_PPT_ENGINE_LIBRE

    L"파일을 선택하면 미리보기가 표시됩니다.", // IDS_WORD_TIP_PREVIEW
    L"파일을 선택하면 미리보기가 표시됩니다.", // IDS_PPT_TIP_PREVIEW

    L"요약",                           // IDS_PDF_SUMMARIZE_BTN
    L"Ollama로 요약 중...",   // IDS_PDF_SUMMARIZE_WORKING
    L"선택한 PDF를 AI로 요약합니다. (Ollama 필요)", // IDS_PDF_SUMMARIZE_TIP
    L"요약 결과를 메모장에서 엽니다.", // IDS_PDF_OPEN_NOTEPAD_TIP
    L"요약 결과가 여기에 표시됩니다.", // IDS_PDF_SUMMARY_TIP
    L"[ Ollama 설정 방법 ]\r\n"
    L"1. https://ollama.com 에서 Ollama 설치\r\n"
    L"2. 명령 프롬프트에서 모델 다운로드:\r\n"
    L"   ollama pull llama3.2:3b\r\n"
    L"3. 목록에서 PDF를 선택한 후 [요약] 클릭", // IDS_PDF_SUMMARY_GUIDE
};

static const wchar_t* s_en[IDS_APP_STRING_COUNT] = {
    // Common / Host (0-18)
    L"Image Convert",
    L"PDF Tools",
    L"MD Convert",
    L"&Help...",
    L"Displays the file path for conversion.",
    L"Check: all images \u2192 single multi-page PDF\nUncheck: each image \u2192 separate PDF file",
    L"Select files.\nMultiple selection supported | Drag & drop also works",
    L"Start convert/run\nClick again while running to stop",
    L"(Done / Failed / Total)\nDone: green  Failed: red  Total: default",
    L"Move selected item up",
    L"Move selected item down",
    L"Remove selected item from list.",
    L"Press F1 for help.",
    // IDS_HELP_TEXT
    L"ImgToPdf v2.1 \u2014 Multi-function File Converter\n"
    L"\n"
    L"[ Common Controls ]\n"
    L"[Browse]: Add files to the current tab.\n"
    L"[Convert/Run]: Execute the current tab's operation.\n"
    L"[\u25b2][\u25bc]: Reorder selected item  [Delete]: Remove selected item\n"
    L"Double-click list item \u2192 Open the file immediately.\n"
    L"Right-click list item \u2192 Remove \u00b7 Open Location \u00b7 Open File \u00b7 Clear All\n"
    L"\n"
    L"[ Tab 1: Image Convert ]\n"
    L"Convert JPG/PNG/BMP/TIFF/GIF \u2192 PDF.\n"
    L"Add files via drag & drop or the [Browse] button.\n"
    L"Double-click: Open file with system default image viewer.\n"
    L"Merge: Combine all images into a single multi-page PDF.\n"
    L"Output path: Leave blank to save in the same folder as the source.\n"
    L"\n"
    L"[ Tab 2: PDF Tools ]\n"
    L"Split: Split a PDF by page or by range.\n"
    L"Merge: Combine multiple PDFs into one (use \u25b2\u25bc to reorder).\n"
    L"Extract: Extract specified pages (e.g. 1,3,5-7,10).\n"
    L"Double-click: Open file with system default PDF viewer.\n"
    L"Output path: Leave blank to save in the same folder as the source.\n"
    L"AI Summary: Extract text via OCR, then generate a structured summary with Ollama(llama3.2:3b) (opens in Notepad).\n"
    L"\n"
    L"[ Tab 3: MD Convert ]\n"
    L"Convert Markdown \u2192 HTML / PDF / HTML+PDF.\n"
    L"HTML+PDF: Generates both HTML and PDF files simultaneously.\n"
    L"Double-click: Open MD file in Notepad.\n"
    L"Output path: Leave blank to save in the same folder as the source.\n"
    L"\n"
    L"[ Tab 4: Word Convert ]\n"
    L"Convert .doc/.docx \u2192 PDF.\n"
    L"Engine: Microsoft Word or LibreOffice (one must be installed).\n"
    L"Double-click: Open file with external app.\n"
    L"Output folder: Leave blank to save in the source folder.\n"
    L"\n"
    L"[ Tab 5: PPT Convert ]\n"
    L"Convert .ppt/.pptx \u2192 PDF.\n"
    L"Engine: Microsoft PowerPoint or LibreOffice (one must be installed).\n"
    L"Double-click: Open file with external app.\n"
    L"Output folder: Leave blank to save in the source folder.\n"
    L"\n"
    L"[ Tab Order ]\n"
    L"\u25c4 \u25ba buttons move the current tab left or right. Order persists after restart.\n"
    L"\n"
    L"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
    L"Author  jaeho\n"
    L"Contact jaeho9697@gmail.com",
    L"Help",
    L"Done",
    L"Error",
    L"Select Output Folder",
    L"All Files",

    // Tab1 - ImgConvert (19-32)
    L"File list\n\u2022 Add via drag & drop or [Browse]\n\u2022 Click: show preview\n\u2022 Delete key: remove selected",
    L"Click an image item in the list to show a preview.",
    L"Output folder path. Leave blank to save in the source folder.",
    L"Leave blank to save in the source folder.",
    L"Image Files",
    L"Output folder does not exist.\nCheck the path or leave blank to save in the source folder.",
    L"Output Folder Error",
    L"This will reset all list and progress.\nContinue?",
    L"Confirm Reset",
    L"Stop",
    L"Convert",
    L"Reset all list and progress.",
    L"Remove selected item from list.",
    L"Select an item to remove.\nAfter conversion, click to reset all.",

    // Tab2 - PdfTools (33-64)
    L"Filename",
    L"Pages",
    L"Output Name",
    L"Note",
    L"PDF file location to add",
    L"PDF Files",
    L"PDF file list\n\u2022 Add via drag & drop or [Browse]\n\u2022 Double-click: open with default PDF viewer\n\u2022 Delete key: remove selected",
    L"Split PDF by page or by range.",
    L"Merge multiple PDFs into one. (Use \u25b2\u25bc to reorder)",
    L"Extract specified pages from a PDF.",
    L"Split each page into a separate PDF.",
    L"Split PDF by specified ranges.",
    L"Range example: 1-3,5,7-9 (comma-separated)",
    L"Enter the filename for the merged PDF.",
    L"Page range to extract. e.g. 1,3,5-7,10",
    L"Save extracted pages as a single PDF.",
    L"Save each extracted page as a separate PDF.",
    L"Output folder path. Leave blank to save in the source folder.",
    L"Select output folder.",
    L"Working...",
    L"Done",
    L"Failed",
    L"An error occurred. Please check the file path.",
    L"Invalid page range. e.g. 1,3,5-7,10",
    L"Run",
    L"Running...",
    L"Remove from List",
    L"Open File Location",
    L"Open File",
    L"Clear All",
    L"Done",
    L"Failed",

    // Tab3 - MdConvert (65-101)
    L"Filename",
    L"Output Name",
    L"Note",
    L"MD file location",
    L"Markdown Files",
    L"MD file list\n\u2022 Add via drag & drop or [Browse]\n\u2022 Click: show preview\n\u2022 Double-click: open in Notepad\n\u2022 Delete key: remove selected",
    L"Click an MD file in the list to show the converted preview.",
    L"Convert Markdown to HTML file.",
    L"Convert Markdown to PDF file.",
    L"Generate both HTML and PDF files simultaneously.",
    L"Output folder path. Leave blank to save in the source folder.",
    L"Select output folder.",
    L"Output folder does not exist.\nCheck the path or leave blank to save in the source folder.",
    L"Output Folder Error",
    L"This will reset all list and progress.\nContinue?",
    L"Confirm Reset",
    L"Rendering...",
    L"Converting...",
    L"Waiting",
    L"Rendering",
    L"Converting",
    L"Done",
    L"Failed",
    L"Some files failed to convert",
    L"HTML+PDF conversion complete",
    L"PDF conversion complete",
    L"HTML conversion complete",
    L"Click a file in the list to preview.",
    L"Cannot read the file.",
    L"Convert",
    L"Reset all list and progress.",
    L"Remove selected item from list.",
    L"Select an item to remove.",
    L"Remove from List",
    L"Open File Location",
    L"Open in Notepad",
    L"Clear All",

    // FileListCtrl (102-112)
    L"Filename",
    L"Output Name",
    L"Note",
    L"Success",
    L"Failed",
    L"Converting...",
    L"_merged.pdf",
    L"Remove from List",
    L"Open File Location",
    L"Open File",
    L"Clear All",

    // Quality slider (113-114)
    L"Quality: %d%%",
    L"Output quality for non-JPEG images (10\u2013100)\nJPEG files are embedded losslessly (quality has no effect)",

    // PDF thumbnail (115)
    L"First page preview\nClick to enlarge",

    // Tab4 - WordConvert (116-139)
    L"Word Convert",                                   // IDS_TAB4_LABEL
    L"Filename",                                       // IDS_WORD_COL0
    L"Engine",                                         // IDS_WORD_COL1
    L"Status",                                         // IDS_WORD_COL2
    L"Word file path",                                 // IDS_WORD_CUE_PATH
    L"Word Document",                                  // IDS_WORD_FILTER_LABEL
    L"Word files (.doc/.docx) \u2014 drag & drop or use Browse", // IDS_WORD_TIP_LIST
    L"Output folder. Leave blank to save next to the source file.", // IDS_WORD_TIP_OUTPUT
    L"Select output folder",                           // IDS_WORD_TIP_BROWSE
    L"Microsoft Word or LibreOffice is not installed.\nOne of them is required for conversion.", // IDS_WORD_ERR_NO_ENGINE
    L"Output path does not exist.",                    // IDS_WORD_ERR_OUTDIR
    L"Conversion Error",                               // IDS_WORD_ERR_TITLE
    L"Converting...",                                  // IDS_WORD_STATUS_WORKING
    L"Done",                                           // IDS_WORD_STATUS_DONE
    L"Failed",                                         // IDS_WORD_STATUS_FAIL
    L"Convert",                                        // IDS_WORD_BTN_CONVERT
    L"Remove from List",                               // IDS_WORD_MENU_REMOVE
    L"Open File Location",                             // IDS_WORD_MENU_OPEN_LOC
    L"Open File",                                      // IDS_WORD_MENU_OPEN
    L"Clear All",                                      // IDS_WORD_MENU_CLEAR_ALL
    L"Reset the list and conversion results?",         // IDS_WORD_CONFIRM_RESET
    L"Reset",                                          // IDS_WORD_CONFIRM_TITLE
    L"Microsoft Word",                                 // IDS_WORD_ENGINE_WORD
    L"LibreOffice",                                    // IDS_WORD_ENGINE_LIBRE

    // Tab5 - PptConvert (140-162)
    L"PPT Convert",                                    // IDS_TAB5_LABEL
    L"Filename",                                       // IDS_PPT_COL0
    L"Engine",                                         // IDS_PPT_COL1
    L"Status",                                         // IDS_PPT_COL2
    L"PPT file path",                                  // IDS_PPT_CUE_PATH
    L"PPT files (.ppt/.pptx) — drag & drop or use Browse", // IDS_PPT_TIP_LIST
    L"Output folder. Leave blank to save next to the source file.", // IDS_PPT_TIP_OUTPUT
    L"Select output folder",                           // IDS_PPT_TIP_BROWSE
    L"Microsoft PowerPoint or LibreOffice is not installed.\nOne of them is required for conversion.", // IDS_PPT_ERR_NO_ENGINE
    L"Output path does not exist.",                    // IDS_PPT_ERR_OUTDIR
    L"Conversion Error",                               // IDS_PPT_ERR_TITLE
    L"Converting...",                                  // IDS_PPT_STATUS_WORKING
    L"Done",                                           // IDS_PPT_STATUS_DONE
    L"Failed",                                         // IDS_PPT_STATUS_FAIL
    L"Convert",                                        // IDS_PPT_BTN_CONVERT
    L"Remove from List",                               // IDS_PPT_MENU_REMOVE
    L"Open File Location",                             // IDS_PPT_MENU_OPEN_LOC
    L"Open File",                                      // IDS_PPT_MENU_OPEN
    L"Clear All",                                      // IDS_PPT_MENU_CLEAR_ALL
    L"Reset the list and conversion results?",         // IDS_PPT_CONFIRM_RESET
    L"Reset",                                          // IDS_PPT_CONFIRM_TITLE
    L"Microsoft PowerPoint",                           // IDS_PPT_ENGINE_PPT
    L"LibreOffice",                                    // IDS_PPT_ENGINE_LIBRE

    L"Select a file to see a preview.",                // IDS_WORD_TIP_PREVIEW
    L"Select a file to see a preview.",                // IDS_PPT_TIP_PREVIEW

    L"Summarize",                                      // IDS_PDF_SUMMARIZE_BTN
    L"Summarizing with Ollama...",                     // IDS_PDF_SUMMARIZE_WORKING
    L"Summarize selected PDF with AI (requires Ollama).", // IDS_PDF_SUMMARIZE_TIP
    L"Open the summary result in Notepad.",            // IDS_PDF_OPEN_NOTEPAD_TIP
    L"AI summary result appears here.",                // IDS_PDF_SUMMARY_TIP
    L"[ Ollama Setup ]\r\n"
    L"1. Install Ollama from https://ollama.com\r\n"
    L"2. Download the model in a command prompt:\r\n"
    L"   ollama pull llama3.2:3b\r\n"
    L"3. Select a PDF from the list and click [Summarize]", // IDS_PDF_SUMMARY_GUIDE
};

CString LS(AppStringId id)
{
    if ((UINT)id >= IDS_APP_STRING_COUNT) return _T("");
    const wchar_t* s = (g_lang == Lang::EN) ? s_en[id] : s_ko[id];
    return s ? CString(s) : CString(_T(""));
}

void SetLang(Lang lang)
{
    g_lang = lang;
    AfxGetApp()->WriteProfileInt(L"Settings", L"Lang", (int)lang);
}

void LoadSavedLang()
{
    int v = AfxGetApp()->GetProfileInt(L"Settings", L"Lang", 0);
    g_lang = (v == 1) ? Lang::EN : Lang::KO;
}

std::vector<TCHAR> BuildFilter(AppStringId labelId, LPCTSTR pattern)
{
    std::vector<TCHAR> buf;
    CString label   = LS(labelId);
    CString allLabel = LS(IDS_FILTER_ALL_LABEL);

    auto push = [&](LPCTSTR s)
    {
        while (*s) buf.push_back(*s++);
        buf.push_back(_T('\0'));
    };

    push(label);
    push(pattern);
    push(allLabel);
    push(_T("*.*"));
    buf.push_back(_T('\0'));
    return buf;
}
