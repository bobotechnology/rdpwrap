#include "stdafx.h"

#include "rdpwrap_core.h"

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) {
  WriteToLog(">>> ServiceMain\r\n");
  if (!AlreadyHooked) {
    Hook();
  }

  if (_ServiceMain != NULL) {
    _ServiceMain(dwArgc, lpszArgv);
  }
  WriteToLog("<<< ServiceMain\r\n");
}

void WINAPI SvchostPushServiceGlobals(void* lpGlobalData) {
  WriteToLog(">>> SvchostPushServiceGlobals\r\n");
  if (!AlreadyHooked) {
    Hook();
  }

  if (_SvchostPushServiceGlobals != NULL) {
    _SvchostPushServiceGlobals(lpGlobalData);
  }
  WriteToLog("<<< SvchostPushServiceGlobals\r\n");
}
