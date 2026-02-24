#include "stdafx.h"

#include "rdpwrap_core.h"

bool OverrideSL(LPWSTR value_name, DWORD* value) {
  if (!g_IniParser) return false;

  char value_name_ansi[256] = {0};
  if (!WideToAnsi(value_name, value_name_ansi, sizeof(value_name_ansi))) {
    return false;
  }

  std::string val_str = IniGetRaw(*g_IniParser, "SLPolicy", value_name_ansi, "");
  if (val_str.empty()) {
    *value = 0;
    return true;
  }

  *value = static_cast<DWORD>(strtoul(val_str.c_str(), nullptr, 10));
  return true;
}

HRESULT WINAPI New_SLGetWindowsInformationDWORD(PWSTR pwszValueName,
                                                DWORD* pdwValue) {
  DWORD dw = 0;
  SIZE_T bw = 0;

  WriteLogFormat("Policy query: %S\r\n", pwszValueName);

  if (OverrideSL(pwszValueName, &dw)) {
    *pdwValue = dw;
    WriteLogFormat("Policy rewrite: %i\r\n", dw);
    return S_OK;
  }

  WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                     &Old_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
  FlushInstructionCache(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                        sizeof(FARJMP));

  HRESULT result = _SLGetWindowsInformationDWORD(pwszValueName, pdwValue);
  if (result == S_OK) {
    WriteLogFormat("Policy result: %i\r\n", dw);
  } else {
    WriteToLog("Policy request failed\r\n");
  }

  WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                     &Stub_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
  FlushInstructionCache(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                        sizeof(FARJMP));

  return result;
}

HRESULT __fastcall New_Win8SL(PWSTR pwszValueName, DWORD* pdwValue) {
  DWORD dw = 0;

  WriteLogFormat("Policy query: %S\r\n", pwszValueName);

  if (OverrideSL(pwszValueName, &dw)) {
    *pdwValue = dw;
    WriteLogFormat("Policy rewrite: %i\r\n", dw);
    return S_OK;
  }

  HRESULT result = _SLGetWindowsInformationDWORD(pwszValueName, pdwValue);
  if (result == S_OK) {
    WriteLogFormat("Policy result: %i\r\n", dw);
  } else {
    WriteToLog("Policy request failed\r\n");
  }

  return result;
}

#if defined(_M_ARM) || defined(_M_ARM64)
HRESULT __fastcall New_Win8SL_CP(DWORD /*arg1*/,
                                 DWORD* pdwValue,
                                 PWSTR pwszValueName,
                                 DWORD /*arg4*/) {
  return New_Win8SL(pwszValueName, pdwValue);
}
#endif
