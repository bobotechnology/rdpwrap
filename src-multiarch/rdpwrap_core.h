#ifndef RDPWRAP_CORE_H_
#define RDPWRAP_CORE_H_

#include <windows.h>

#include <string>

#include "cpp_configparser/include/ini/parser.hpp"

typedef HRESULT(WINAPI* SLGETWINDOWSINFORMATIONDWORD)(PWSTR pwszValueName,
                                                      DWORD* pdwValue);
typedef VOID(WINAPI* SERVICEMAIN)(DWORD dwArgc, LPTSTR* lpszArgv);
typedef VOID(WINAPI* SVCHOSTPUSHSERVICEGLOBALS)(PVOID lpGlobalData);

struct INI_VAR_STRING {
  char Name[255];
  char Value[255];
};

struct INI_VAR_BYTEARRAY {
  char Name[255];
  BYTE ArraySize;
  char Value[255];
};

typedef struct {
  union {
    struct {
      WORD Minor;
      WORD Major;
    } wVersion;
    DWORD dwVersion;
  };
  WORD Release;
  WORD Build;
} FILE_VERSION;

#if defined(_M_X64)
typedef unsigned long long PLATFORM_DWORD;
#pragma pack(push, 1)
struct FARJMP {
  BYTE MovOp;      // 48 mov rax, ptr
  BYTE MovRegArg;  // B8
  DWORD64 MovArg;  // PTR
  BYTE PushRaxOp;  // 50 push rax
  BYTE RetOp;      // C3 retn
};
#pragma pack(pop)
static_assert(sizeof(FARJMP) == 12, "x64 FARJMP size must be 12 bytes");
static_assert(offsetof(FARJMP, MovArg) == 2,
              "x64 FARJMP target must start at byte 2");
static_assert(alignof(FARJMP) == 1, "x64 FARJMP must be byte-packed");
#elif defined(_M_IX86)
typedef unsigned long PLATFORM_DWORD;
#pragma pack(push, 1)
struct FARJMP {
  BYTE PushOp;    // 68 push ptr
  DWORD PushArg;  // PTR
  BYTE RetOp;     // C3 retn
};
#pragma pack(pop)
static_assert(sizeof(FARJMP) == 6, "x86 FARJMP size must be 6 bytes");
static_assert(offsetof(FARJMP, PushArg) == 1,
              "x86 FARJMP target must start at byte 1");
static_assert(alignof(FARJMP) == 1, "x86 FARJMP must be byte-packed");
#elif defined(_M_ARM64)
typedef unsigned long long PLATFORM_DWORD;
#pragma pack(push, 1)
struct FARJMP {
  DWORD LdrOp;     // 0x58000050 ldr x16, #8
  DWORD BrOp;      // 0xD61F0200 br x16
  DWORD64 Target;  // PTR
};
#pragma pack(pop)
static_assert(sizeof(FARJMP) == 16, "ARM64 FARJMP size must be 16 bytes");
static_assert(offsetof(FARJMP, Target) == 8,
              "ARM64 FARJMP target must start at byte 8");
static_assert(alignof(FARJMP) == 1, "ARM64 FARJMP must be byte-packed");
#elif defined(_M_ARM)
typedef unsigned long PLATFORM_DWORD;
#pragma pack(push, 1)
struct FARJMP {
  WORD LdrOp;    // 0x4800 ldr r0, [pc, #offset]
  WORD BlxOp;    // 0x4700 blx r0
  DWORD Target;  // PTR
};
#pragma pack(pop)
static_assert(sizeof(FARJMP) == 8, "ARM32 FARJMP size must be 8 bytes");
static_assert(offsetof(FARJMP, Target) == 4,
              "ARM32 FARJMP target must start at byte 4");
static_assert(alignof(FARJMP) == 1, "ARM32 FARJMP must be byte-packed");
#else
#error Unsupported architecture.
#endif

extern FARJMP Old_SLGetWindowsInformationDWORD;
extern FARJMP Stub_SLGetWindowsInformationDWORD;
extern SLGETWINDOWSINFORMATIONDWORD _SLGetWindowsInformationDWORD;

extern ini::Parser* g_IniParser;
extern wchar_t LogFile[256];
extern HMODULE hTermSrv;
extern HMODULE hSLC;
extern PLATFORM_DWORD TermSrvBase;
extern FILE_VERSION FV;
extern SERVICEMAIN _ServiceMain;
extern SVCHOSTPUSHSERVICEGLOBALS _SvchostPushServiceGlobals;
extern bool AlreadyHooked;

std::string IniGetRaw(const ini::Parser& parser,
                      const char* sect,
                      const char* key,
                      const char* def_val);
DWORD INIReadDWordHex(const ini::Parser& parser,
                      const char* sect,
                      const char* key,
                      PLATFORM_DWORD def_val);
void INIReadString(const ini::Parser& parser,
                   const char* sect,
                   const char* key,
                   const char* def_val,
                   char* ret,
                   DWORD ret_size);
bool GetBoolFromIni(const ini::Parser& parser,
                    const char* sect,
                    const char* key,
                    bool def_val);
bool GetByteArrayFromIni(const ini::Parser& parser,
                         const char* sect,
                         const char* key,
                         char* out_buf,
                         BYTE& out_size,
                         BYTE max_len);
bool WideToAnsi(const wchar_t* src, char* dst, size_t dst_size);

void WriteToLog(const char* text);
void WriteLogFormat(const char* format, ...);

HMODULE GetCurrentModule();
bool GetModuleCodeSectionInfo(HMODULE hModule,
                              PLATFORM_DWORD* base_addr,
                              PLATFORM_DWORD* base_size);
void SetThreadsState(bool resume);
BOOL __stdcall GetModuleVersion(LPCWSTR lptstrModuleName,
                                FILE_VERSION* file_version);
BOOL __stdcall GetFileVersion(LPCWSTR lptstrFilename, FILE_VERSION* file_version);

bool OverrideSL(LPWSTR value_name, DWORD* value);
HRESULT WINAPI New_SLGetWindowsInformationDWORD(PWSTR pwszValueName,
                                                DWORD* pdwValue);
HRESULT __fastcall New_Win8SL(PWSTR pwszValueName, DWORD* pdwValue);
#if defined(_M_ARM) || defined(_M_ARM64)
HRESULT __fastcall New_Win8SL_CP(DWORD arg1,
                                 DWORD* pdwValue,
                                 PWSTR pwszValueName,
                                 DWORD arg4);
#endif

HRESULT WINAPI New_CSLQuery_Initialize();
void Hook();

#endif  // RDPWRAP_CORE_H_
