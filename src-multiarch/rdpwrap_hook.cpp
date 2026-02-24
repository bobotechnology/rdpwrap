#include "stdafx.h"

#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

#include "rdpwrap_core.h"

#if defined(_M_ARM) || defined(_M_ARM64)
#define RDPWRAP_INI_FILE_NAME L"rdpwrap-arm-kb.ini"
#elif defined(_M_IX86) || defined(_M_X64)
#define RDPWRAP_INI_FILE_NAME L"rdpwrap.ini"
#else
#error Unsupported architecture for RDPWRAP_INI_FILE_NAME
#endif

namespace {

bool ResolvePatchBytes(const ini::Parser& parser,
                       const char* build_section,
                       const char* code_key,
                       char* patch_name,
                       size_t patch_name_size,
                       char* patch_buf,
                       BYTE* patch_size) {
  if (!patch_name || patch_name_size == 0 || !patch_buf || !patch_size) {
    return false;
  }

  patch_name[0] = '\0';
  *patch_size = 0;

  INIReadString(parser, build_section, code_key, "", patch_name,
                static_cast<DWORD>(patch_name_size));
  if (patch_name[0] == '\0') {
    return false;
  }

  BYTE parsed_size = 0;
  if (GetByteArrayFromIni(parser, "PatchCodes", patch_name, patch_buf, parsed_size,
                          255)) {
    *patch_size = parsed_size;
    return true;
  }

  if (GetByteArrayFromIni(parser, build_section, code_key, patch_buf, parsed_size,
                          255)) {
    *patch_size = parsed_size;
    return true;
  }

  return false;
}

void ApplyConfiguredPatch(const ini::Parser& parser,
                          const char* build_section,
                          const char* patch_enabled_key,
                          const char* patch_offset_key,
                          const char* patch_code_key,
                          const char* patch_label,
                          PLATFORM_DWORD module_base) {
  if (!GetBoolFromIni(parser, build_section, patch_enabled_key, false)) {
    return;
  }

  DWORD offset = INIReadDWordHex(parser, build_section, patch_offset_key, 0);
  if (offset == 0) {
    WriteLogFormat("Patch %s: missing offset\r\n", patch_label);
    return;
  }

  char patch_name[255] = {0};
  char patch_buf[255] = {0};
  BYTE patch_size = 0;
  if (!ResolvePatchBytes(parser, build_section, patch_code_key, patch_name,
                         _countof(patch_name), patch_buf, &patch_size) ||
      patch_size == 0) {
    WriteLogFormat("Patch %s: invalid code (%s)\r\n", patch_label,
                   patch_code_key);
    return;
  }

  SIZE_T bytes_written = 0;
  PLATFORM_DWORD patch_addr = module_base + offset;
  WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<LPVOID>(patch_addr),
                     patch_buf, patch_size, &bytes_written);
  FlushInstructionCache(GetCurrentProcess(),
                        reinterpret_cast<LPCVOID>(patch_addr), patch_size);

  WriteLogFormat("Patch %s: termsrv.dll+0x%X (%u bytes, code=%s)\r\n",
                 patch_label, offset, patch_size, patch_name);
}

}  // namespace

HRESULT WINAPI New_CSLQuery_Initialize() {
  DWORD* bServerSku = NULL;
  DWORD* bRemoteConnAllowed = NULL;
  DWORD* bFUSEnabled = NULL;
  DWORD* bAppServerAllowed = NULL;
  DWORD* bMultimonAllowed = NULL;
  DWORD* lMaxUserSessions = NULL;
  DWORD* ulMaxDebugSessions = NULL;
  DWORD* bInitialized = NULL;

  WriteToLog(">>> CSLQuery::Initialize\r\n");

  char sect[256] = {0};
  wsprintfA(sect, "%d.%d.%d.%d-SLInit", FV.wVersion.Major, FV.wVersion.Minor,
            FV.Release, FV.Build);

  if (g_IniParser->has_section(sect)) {
#ifdef _M_ARM64
    bServerSku = (DWORD*)(TermSrvBase +
                          INIReadDWordHex(*g_IniParser, sect, "bServerSku.arm64", 0));
    bRemoteConnAllowed = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bRemoteConnAllowed.arm64", 0));
    bFUSEnabled =
        (DWORD*)(TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bFUSEnabled.arm64", 0));
    bAppServerAllowed = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bAppServerAllowed.arm64", 0));
    bMultimonAllowed = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bMultimonAllowed.arm64", 0));
    lMaxUserSessions = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "lMaxUserSessions.arm64", 0));
    ulMaxDebugSessions =
        (DWORD*)(TermSrvBase +
                 INIReadDWordHex(*g_IniParser, sect, "ulMaxDebugSessions.arm64", 0));
    bInitialized =
        (DWORD*)(TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bInitialized.arm64", 0));
#else
    bServerSku =
        (DWORD*)(TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bServerSku.arm", 0));
    bRemoteConnAllowed = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bRemoteConnAllowed.arm", 0));
    bFUSEnabled =
        (DWORD*)(TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bFUSEnabled.arm", 0));
    bAppServerAllowed = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bAppServerAllowed.arm", 0));
    bMultimonAllowed = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bMultimonAllowed.arm", 0));
    lMaxUserSessions = (DWORD*)(
        TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "lMaxUserSessions.arm", 0));
    ulMaxDebugSessions =
        (DWORD*)(TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "ulMaxDebugSessions.arm", 0));
    bInitialized =
        (DWORD*)(TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "bInitialized.arm", 0));
#endif
  }

  if (bServerSku) {
    *bServerSku = INIReadDWordHex(*g_IniParser, "SLInit", "bServerSku", 1);
    WriteLogFormat("SLInit [0x%p] bServerSku = %d\r\n", bServerSku, *bServerSku);
  }
  if (bRemoteConnAllowed) {
    *bRemoteConnAllowed =
        INIReadDWordHex(*g_IniParser, "SLInit", "bRemoteConnAllowed", 1);
    WriteLogFormat("SLInit [0x%p] bRemoteConnAllowed = %d\r\n", bRemoteConnAllowed,
                   *bRemoteConnAllowed);
  }
  if (bFUSEnabled) {
    *bFUSEnabled = INIReadDWordHex(*g_IniParser, "SLInit", "bFUSEnabled", 1);
    WriteLogFormat("SLInit [0x%p] bFUSEnabled = %d\r\n", bFUSEnabled, *bFUSEnabled);
  }
  if (bAppServerAllowed) {
    *bAppServerAllowed =
        INIReadDWordHex(*g_IniParser, "SLInit", "bAppServerAllowed", 1);
    WriteLogFormat("SLInit [0x%p] bAppServerAllowed = %d\r\n", bAppServerAllowed,
                   *bAppServerAllowed);
  }
  if (bMultimonAllowed) {
    *bMultimonAllowed =
        INIReadDWordHex(*g_IniParser, "SLInit", "bMultimonAllowed", 1);
    WriteLogFormat("SLInit [0x%p] bMultimonAllowed = %d\r\n", bMultimonAllowed,
                   *bMultimonAllowed);
  }
  if (lMaxUserSessions) {
    *lMaxUserSessions = INIReadDWordHex(*g_IniParser, "SLInit", "lMaxUserSessions", 0);
    WriteLogFormat("SLInit [0x%p] lMaxUserSessions = %d\r\n", lMaxUserSessions,
                   *lMaxUserSessions);
  }
  if (ulMaxDebugSessions) {
    *ulMaxDebugSessions =
        INIReadDWordHex(*g_IniParser, "SLInit", "ulMaxDebugSessions", 0);
    WriteLogFormat("SLInit [0x%p] ulMaxDebugSessions = %d\r\n", ulMaxDebugSessions,
                   *ulMaxDebugSessions);
  }
  if (bInitialized) {
    *bInitialized = INIReadDWordHex(*g_IniParser, "SLInit", "bInitialized", 1);
    WriteLogFormat("SLInit [0x%p] bInitialized = %d\r\n", bInitialized, *bInitialized);
  }

  WriteToLog("<<< CSLQuery::Initialize\r\n");
  return S_OK;
}

void Hook() {
  AlreadyHooked = true;

  wchar_t configFile[256] = {0x00};
  wchar_t modulePath[256] = {0x00};
  wchar_t moduleDir[256] = {0x00};

  GetModuleFileNameW(GetCurrentModule(), modulePath, _countof(modulePath));
  wcscpy_s(moduleDir, modulePath);
  PathRemoveFileSpecW(moduleDir);

  PathCombineW(LogFile, moduleDir, L"rdpwrap.txt");
  WriteToLog("Loading configuration...\r\n");
  PathCombineW(configFile, moduleDir, RDPWRAP_INI_FILE_NAME);

  WriteLogFormat("Configuration file: %S\r\n", configFile);

  char configAnsi[MAX_PATH * 3] = {0};
  WideToAnsi(configFile, configAnsi, sizeof(configAnsi));

  ini::ParseOptions parseOptions;
  parseOptions.interpolation = ini::InterpolationMode::None;
  parseOptions.strict = false;
  parseOptions.empty_lines_in_values = true;
  g_IniParser = new ini::Parser(parseOptions);

  try {
    g_IniParser->read_file(configAnsi);
  } catch (...) {
    WriteToLog("Error: Failed to load configuration\r\n");
    return;
  }

  SIZE_T bw = 0;
  WORD ver = 0;
  PLATFORM_DWORD termSrvSize = 0;
  PLATFORM_DWORD signPtr = 0;
  FARJMP jump = {};

  WriteToLog("Initializing RDP Wrapper...\r\n");

  hTermSrv = LoadLibrary(L"termsrv.dll");
  if (hTermSrv == 0) {
    WriteToLog("Error: Failed to load Terminal Services library\r\n");
    return;
  }

  _ServiceMain = (SERVICEMAIN)GetProcAddress(hTermSrv, "ServiceMain");
  _SvchostPushServiceGlobals =
      (SVCHOSTPUSHSERVICEGLOBALS)GetProcAddress(hTermSrv, "SvchostPushServiceGlobals");

  WriteLogFormat(
      "Base addr:  0x%p\r\n"
      "SvcMain:    termsrv.dll+0x%llX\r\n"
      "SvcGlobals: termsrv.dll+0x%llX\r\n",
      hTermSrv,
      static_cast<unsigned long long>((PLATFORM_DWORD)_ServiceMain -
                                      (PLATFORM_DWORD)hTermSrv),
      static_cast<unsigned long long>((PLATFORM_DWORD)_SvchostPushServiceGlobals -
                                      (PLATFORM_DWORD)hTermSrv));

  if (GetModuleVersion(L"termsrv.dll", &FV)) {
    ver = (BYTE)FV.wVersion.Minor | ((BYTE)FV.wVersion.Major << 8);
  }

  if (ver == 0) {
    WriteToLog("Error: Failed to detect Terminal Services version\r\n");
    return;
  }

  WriteLogFormat("Version:    %d.%d.%d.%d\r\n", FV.wVersion.Major,
                 FV.wVersion.Minor, FV.Release, FV.Build);

  WriteToLog("Freezing threads...\r\n");
  SetThreadsState(false);

  bool enabled = true;
  bool boolValue = true;

  boolValue = GetBoolFromIni(*g_IniParser, "Main", "SLPolicyHookNT60", true);
  if ((ver == 0x0600) && boolValue) {
    hSLC = LoadLibrary(L"slc.dll");
    _SLGetWindowsInformationDWORD =
        (SLGETWINDOWSINFORMATIONDWORD)GetProcAddress(hSLC,
                                                     "SLGetWindowsInformationDWORD");
    if (_SLGetWindowsInformationDWORD != INVALID_HANDLE_VALUE) {
      WriteToLog("Hook SLGetWindowsInformationDWORD\r\n");
#ifdef _M_ARM64
      Stub_SLGetWindowsInformationDWORD.LdrOp = 0x58000050;
      Stub_SLGetWindowsInformationDWORD.BrOp = 0xD61F0200;
      Stub_SLGetWindowsInformationDWORD.Target =
          (DWORD64)New_SLGetWindowsInformationDWORD;
#elif defined(_M_ARM)
      Stub_SLGetWindowsInformationDWORD.LdrOp = 0x4800;
      Stub_SLGetWindowsInformationDWORD.BlxOp = 0x4700;
      Stub_SLGetWindowsInformationDWORD.Target =
          (DWORD)New_SLGetWindowsInformationDWORD;
#elif defined(_M_X64)
      Stub_SLGetWindowsInformationDWORD.MovOp = 0x48;
      Stub_SLGetWindowsInformationDWORD.MovRegArg = 0xB8;
      Stub_SLGetWindowsInformationDWORD.MovArg =
          (DWORD64)New_SLGetWindowsInformationDWORD;
      Stub_SLGetWindowsInformationDWORD.PushRaxOp = 0x50;
      Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#elif defined(_M_IX86)
      Stub_SLGetWindowsInformationDWORD.PushOp = 0x68;
      Stub_SLGetWindowsInformationDWORD.PushArg =
          (DWORD)New_SLGetWindowsInformationDWORD;
      Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#else
#error Unsupported architecture
#endif
      ReadProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                        &Old_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
      WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                         &Stub_SLGetWindowsInformationDWORD, sizeof(FARJMP),
                         &bw);
      FlushInstructionCache(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                            sizeof(FARJMP));
    }
  }

  boolValue = GetBoolFromIni(*g_IniParser, "Main", "SLPolicyHookNT61", true);
  if ((ver == 0x0601) && boolValue) {
    hSLC = LoadLibrary(L"slc.dll");
    _SLGetWindowsInformationDWORD =
        (SLGETWINDOWSINFORMATIONDWORD)GetProcAddress(hSLC,
                                                     "SLGetWindowsInformationDWORD");
    if (_SLGetWindowsInformationDWORD != INVALID_HANDLE_VALUE) {
      WriteToLog("Hook SLGetWindowsInformationDWORD\r\n");
#ifdef _M_ARM64
      Stub_SLGetWindowsInformationDWORD.LdrOp = 0x58000050;
      Stub_SLGetWindowsInformationDWORD.BrOp = 0xD61F0200;
      Stub_SLGetWindowsInformationDWORD.Target =
          (DWORD64)New_SLGetWindowsInformationDWORD;
#elif defined(_M_ARM)
      Stub_SLGetWindowsInformationDWORD.LdrOp = 0x4800;
      Stub_SLGetWindowsInformationDWORD.BlxOp = 0x4700;
      Stub_SLGetWindowsInformationDWORD.Target =
          (DWORD)New_SLGetWindowsInformationDWORD;
#elif defined(_M_X64)
      Stub_SLGetWindowsInformationDWORD.MovOp = 0x48;
      Stub_SLGetWindowsInformationDWORD.MovRegArg = 0xB8;
      Stub_SLGetWindowsInformationDWORD.MovArg =
          (DWORD64)New_SLGetWindowsInformationDWORD;
      Stub_SLGetWindowsInformationDWORD.PushRaxOp = 0x50;
      Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#elif defined(_M_IX86)
      Stub_SLGetWindowsInformationDWORD.PushOp = 0x68;
      Stub_SLGetWindowsInformationDWORD.PushArg =
          (DWORD)New_SLGetWindowsInformationDWORD;
      Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#else
#error Unsupported architecture
#endif
      ReadProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                        &Old_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
      WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                         &Stub_SLGetWindowsInformationDWORD, sizeof(FARJMP),
                         &bw);
      FlushInstructionCache(GetCurrentProcess(), _SLGetWindowsInformationDWORD,
                            sizeof(FARJMP));
    }
  }

  if (ver == 0x0602) {
    hSLC = LoadLibrary(L"slc.dll");
    _SLGetWindowsInformationDWORD =
        (SLGETWINDOWSINFORMATIONDWORD)GetProcAddress(hSLC,
                                                     "SLGetWindowsInformationDWORD");
  }

  char sect[256] = {0};
  wsprintfA(sect, "%d.%d.%d.%d", FV.wVersion.Major, FV.wVersion.Minor,
            FV.Release, FV.Build);

  if (g_IniParser->has_section(sect) &&
      GetModuleCodeSectionInfo(hTermSrv, &TermSrvBase, &termSrvSize)) {
#if defined(_M_ARM64)
    ApplyConfiguredPatch(*g_IniParser, sect, "LocalOnlyPatch.arm64",
                         "LocalOnlyOffset.arm64", "LocalOnlyCode.arm64",
                         "LocalOnly", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "SingleUserPatch.arm64",
                         "SingleUserOffset.arm64", "SingleUserCode.arm64",
                         "SingleUser", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "DefPolicyPatch.arm64",
                         "DefPolicyOffset.arm64", "DefPolicyCode.arm64",
                         "DefPolicy", TermSrvBase);
#elif defined(_M_ARM)
    ApplyConfiguredPatch(*g_IniParser, sect, "LocalOnlyPatch.arm",
                         "LocalOnlyOffset.arm", "LocalOnlyCode.arm",
                         "LocalOnly", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "SingleUserPatch.arm",
                         "SingleUserOffset.arm", "SingleUserCode.arm",
                         "SingleUser", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "DefPolicyPatch.arm",
                         "DefPolicyOffset.arm", "DefPolicyCode.arm",
                         "DefPolicy", TermSrvBase);
#elif defined(_M_X64)
    ApplyConfiguredPatch(*g_IniParser, sect, "LocalOnlyPatch.x64",
                         "LocalOnlyOffset.x64", "LocalOnlyCode.x64",
                         "LocalOnly", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "SingleUserPatch.x64",
                         "SingleUserOffset.x64", "SingleUserCode.x64",
                         "SingleUser", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "DefPolicyPatch.x64",
                         "DefPolicyOffset.x64", "DefPolicyCode.x64",
                         "DefPolicy", TermSrvBase);
#elif defined(_M_IX86)
    ApplyConfiguredPatch(*g_IniParser, sect, "LocalOnlyPatch.x86",
                         "LocalOnlyOffset.x86", "LocalOnlyCode.x86",
                         "LocalOnly", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "SingleUserPatch.x86",
                         "SingleUserOffset.x86", "SingleUserCode.x86",
                         "SingleUser", TermSrvBase);
    ApplyConfiguredPatch(*g_IniParser, sect, "DefPolicyPatch.x86",
                         "DefPolicyOffset.x86", "DefPolicyCode.x86",
                         "DefPolicy", TermSrvBase);
#else
#error Unsupported architecture
#endif
#ifdef _M_ARM64
    enabled = GetBoolFromIni(*g_IniParser, sect, "SLPolicyInternal.arm64", false);
#else
    enabled = GetBoolFromIni(*g_IniParser, sect, "SLPolicyInternal.arm", false);
#endif
    if (enabled) {
      WriteToLog("Hook SLGetWindowsInformationDWORDWrapper\r\n");
#ifdef _M_ARM64
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLPolicyOffset.arm64", 0));
      jump.LdrOp = 0x58000050;
      jump.BrOp = 0xD61F0200;
      jump.Target = (DWORD64)New_Win8SL;
#elif defined(_M_ARM)
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLPolicyOffset.arm", 0));
      jump.LdrOp = 0x4800;
      jump.BlxOp = 0x4700;
      jump.Target = (DWORD)New_Win8SL;
#elif defined(_M_X64)
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLPolicyOffset.x64", 0));
      jump.MovOp = 0x48;
      jump.MovRegArg = 0xB8;
      jump.MovArg = (DWORD64)New_Win8SL;
      jump.PushRaxOp = 0x50;
      jump.RetOp = 0xC3;
#elif defined(_M_IX86)
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLPolicyOffset.x86", 0));
      jump.PushOp = 0x68;
      jump.PushArg = (DWORD)New_Win8SL;
      jump.RetOp = 0xC3;
#else
#error Unsupported architecture
#endif
      if (signPtr > TermSrvBase) {
        WriteProcessMemory(GetCurrentProcess(), (LPVOID)signPtr, &jump,
                           sizeof(FARJMP), &bw);
        FlushInstructionCache(GetCurrentProcess(), (LPCVOID)signPtr,
                              sizeof(FARJMP));
      }
    }

#ifdef _M_ARM64
    enabled = GetBoolFromIni(*g_IniParser, sect, "SLInitHook.arm64", false);
#else
    enabled = GetBoolFromIni(*g_IniParser, sect, "SLInitHook.arm", false);
#endif
    if (enabled) {
      WriteToLog("Hook CSLQuery::Initialize\r\n");
#ifdef _M_ARM64
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLInitOffset.arm64", 0));
      jump.LdrOp = 0x58000050;
      jump.BrOp = 0xD61F0200;
      jump.Target = (DWORD64)New_CSLQuery_Initialize;
#elif defined(_M_ARM)
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLInitOffset.arm", 0));
      jump.LdrOp = 0x4800;
      jump.BlxOp = 0x4700;
      jump.Target = (DWORD)New_CSLQuery_Initialize;
#elif defined(_M_X64)
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLInitOffset.x64", 0));
      jump.MovOp = 0x48;
      jump.MovRegArg = 0xB8;
      jump.MovArg = (DWORD64)New_CSLQuery_Initialize;
      jump.PushRaxOp = 0x50;
      jump.RetOp = 0xC3;
#elif defined(_M_IX86)
      signPtr = (PLATFORM_DWORD)(
          TermSrvBase + INIReadDWordHex(*g_IniParser, sect, "SLInitOffset.x86", 0));
      jump.PushOp = 0x68;
      jump.PushArg = (DWORD)New_CSLQuery_Initialize;
      jump.RetOp = 0xC3;
#else
#error Unsupported architecture
#endif
      if (signPtr > TermSrvBase) {
        WriteProcessMemory(GetCurrentProcess(), (LPVOID)signPtr, &jump,
                           sizeof(FARJMP), &bw);
        FlushInstructionCache(GetCurrentProcess(), (LPCVOID)signPtr,
                              sizeof(FARJMP));
      }
    }
  }

  WriteToLog("Resumimg threads...\r\n");
  SetThreadsState(true);
}
