#include "stdafx.h"

#include "rdpwrap_core.h"

FARJMP Old_SLGetWindowsInformationDWORD = {};
FARJMP Stub_SLGetWindowsInformationDWORD = {};
SLGETWINDOWSINFORMATIONDWORD _SLGetWindowsInformationDWORD = nullptr;

ini::Parser* g_IniParser = nullptr;
wchar_t LogFile[256] = L"rdpwrap.txt";
HMODULE hTermSrv = nullptr;
HMODULE hSLC = nullptr;
PLATFORM_DWORD TermSrvBase = 0;
FILE_VERSION FV = {};
SERVICEMAIN _ServiceMain = nullptr;
SVCHOSTPUSHSERVICEGLOBALS _SvchostPushServiceGlobals = nullptr;
bool AlreadyHooked = false;
