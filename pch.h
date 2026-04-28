#pragma once
#include "targetver.h"
#include "AppMessages.h"

#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxdialogex.h>
#include <afxbutton.h>
#include <shlobj.h>
#include <shellapi.h>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <fstream>
#include <algorithm>
#include <functional>

// WinRT (Windows.Data.Pdf)
#include <wrl.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <Windows.Data.Pdf.h>
#include <Windows.Storage.h>
#include <Windows.Storage.Streams.h>
#include <Windows.Graphics.Imaging.h>
#include <Windows.Media.Ocr.h>
#pragma comment(lib, "runtimeobject.lib")
#include <shcore.h>
#pragma comment(lib, "shcore.lib")
