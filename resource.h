//{{NO_DEPENDENCIES}}
// resource.h - ImgToPdf resource ID definitions

#pragma once

// Icon
#define IDI_IMGTOPDF                101

// Dialog
#define IDD_IMGTOPDF_DIALOG         102

// Controls
#define IDC_EDIT_PATH               1001
#define IDC_CHECK_MERGE             1002
#define IDC_BTN_BROWSE              1003
#define IDC_BTN_CONVERT             1004
#define IDC_PROGRESS                1005
#define IDC_STATIC_PROGRESS_TXT     1006
#define IDC_LIST_FILES              1007
#define IDC_STATIC_PREVIEW          1008
#define IDC_LBL_PATH                1009
#define IDC_BTN_MOVE_UP             1010
#define IDC_BTN_MOVE_DOWN           1011
#define IDC_BTN_CLEAR               1012

// 시스템 메뉴 사용자 정의 항목 (0xF000 미만이어야 함)
#define IDM_HELP_USAGE              0x1001

// ImageList indices (used in FileListCtrl)
#define IMG_IDX_NONE                0
#define IMG_IDX_RUNNING             1
#define IMG_IDX_SUCCESS             2
#define IMG_IDX_FAIL                3

// Thread -> Dialog custom messages
#define WM_CONVERT_PROGRESS         (WM_USER + 1)
#define WM_CONVERT_DONE             (WM_USER + 2)
#define WM_LIST_ENTRIES_CHANGED     (WM_USER + 3)  // FileListCtrl -> Dialog: 항목 추가/삭제 완료

// Next default values for new objects
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE    103
#define _APS_NEXT_COMMAND_VALUE     32771
#define _APS_NEXT_CONTROL_VALUE     1009
#define _APS_NEXT_SYMED_VALUE       101
#endif
#endif
