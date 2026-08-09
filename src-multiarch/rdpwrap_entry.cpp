#include "stdafx.h"

#include "rdpwrap_core.h"

extern "C" void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) {
  WriteToLog(">>> ServiceMain\r\n");
  if (InterlockedCompareExchange(&AlreadyHooked, 1, 0) == 0) {
    Hook();
  }

  if (_ServiceMain != NULL) {
    _ServiceMain(dwArgc, lpszArgv);
  }
  WriteToLog("<<< ServiceMain\r\n");
}

extern "C" void WINAPI SvchostPushServiceGlobals(void* lpGlobalData) {
  WriteToLog(">>> SvchostPushServiceGlobals\r\n");
  if (InterlockedCompareExchange(&AlreadyHooked, 1, 0) == 0) {
    Hook();
  }

  if (_SvchostPushServiceGlobals != NULL) {
    _SvchostPushServiceGlobals(lpGlobalData);
  }
  WriteToLog("<<< SvchostPushServiceGlobals\r\n");
}
