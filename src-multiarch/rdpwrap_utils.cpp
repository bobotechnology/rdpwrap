#include "stdafx.h"

#include "rdpwrap_core.h"

#include <cstdarg>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

#include <tlhelp32.h>

#ifdef _MSC_VER
#pragma comment(lib, "Version.lib")
#endif

std::string IniGetRaw(const ini::Parser& parser,
                      const char* sect,
                      const char* key,
                      const char* def_val) {
  try {
    if (!parser.has_section(sect) || !parser.has_option(sect, key)) {
      return def_val ? def_val : "";
    }
    ini::OptionValue ov = parser.get_raw(sect, key);
    if (!ov.has_value()) {
      return def_val ? def_val : "";
    }
    return ov.value();
  } catch (...) {
    return def_val ? def_val : "";
  }
}

PLATFORM_DWORD INIReadDWordHex(const ini::Parser& parser,
                            const char* sect,
                            const char* key,
                            PLATFORM_DWORD def_val) {
  std::string val_str = IniGetRaw(parser, sect, key, "");
  if (val_str.empty()) {
    return def_val;
  }
  errno = 0;
  char* end = nullptr;
  const unsigned long long value = strtoull(val_str.c_str(), &end, 16);
  if (errno == ERANGE || !end || *end != '\0' ||
      value > (std::numeric_limits<PLATFORM_DWORD>::max)()) {
    return def_val;
  }
  return static_cast<PLATFORM_DWORD>(value);
}

void INIReadString(const ini::Parser& parser,
                   const char* sect,
                   const char* key,
                   const char* def_val,
                   char* ret,
                   DWORD ret_size) {
  std::string val_str = IniGetRaw(parser, sect, key, def_val ? def_val : "");
  strncpy_s(ret, ret_size, val_str.c_str(), _TRUNCATE);
}

namespace {
int HexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool QueryProductVersion(const wchar_t* filename, FILE_VERSION* file_version) {
  if (!filename || !*filename || !file_version) return false;

  DWORD ignored = 0;
  const DWORD size = GetFileVersionInfoSizeW(filename, &ignored);
  if (size == 0) return false;

  std::vector<BYTE> data(size);
  if (!GetFileVersionInfoW(filename, 0, size, data.data())) return false;

  VS_FIXEDFILEINFO* fixed = nullptr;
  UINT fixed_size = 0;
  if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&fixed),
                      &fixed_size) ||
      !fixed || fixed_size < sizeof(VS_FIXEDFILEINFO) ||
      fixed->dwSignature != VS_FFI_SIGNATURE) {
    return false;
  }

  // INI section names intentionally use ProductVersion only.  In particular,
  // current Windows 11 termsrv.dll builds can report a 6.2 FileVersion while
  // the supported configuration section is keyed by ProductVersion 10.0.
  file_version->dwVersion = fixed->dwProductVersionMS;
  file_version->Release = HIWORD(fixed->dwProductVersionLS);
  file_version->Build = LOWORD(fixed->dwProductVersionLS);
  return true;
}
}  // namespace

bool GetBoolFromIni(const ini::Parser& parser,
                    const char* sect,
                    const char* key,
                    bool def_val) {
  std::string val_str = IniGetRaw(parser, sect, key, "");
  if (val_str.empty()) {
    return def_val;
  }
  return (strtol(val_str.c_str(), nullptr, 10) != 0);
}

bool GetByteArrayFromIni(const ini::Parser& parser,
                         const char* sect,
                         const char* key,
                         char* out_buf,
                         BYTE& out_size,
                         BYTE max_len) {
  std::string raw = IniGetRaw(parser, sect, key, "");
  if (raw.empty()) {
    return false;
  }

  std::string hex;
  hex.reserve(raw.size());
  for (size_t i = 0; i < raw.size(); ++i) {
    char c = raw[i];
    if (c == ' ' || c == '\t' || c == ',' || c == '-') continue;
    hex.push_back(c);
  }

  if ((hex.size() % 2) != 0) return false;
  const size_t max_characters = static_cast<size_t>(max_len) * 2;
  if (hex.size() > max_characters) return false;

  for (size_t i = 0; i < hex.size(); i += 2) {
    int hi = HexNibble(hex[i]);
    int lo = HexNibble(hex[i + 1]);
    if (hi < 0 || lo < 0) return false;
    out_buf[i / 2] = static_cast<char>((hi << 4) | lo);
  }

  out_size = static_cast<BYTE>(hex.size() / 2);
  return true;
}

bool WideToAnsi(const wchar_t* src, char* dst, size_t dst_size) {
  if (!src || !dst || dst_size == 0) return false;
  size_t converted = 0;
  errno_t e = wcstombs_s(&converted, dst, dst_size, src, _TRUNCATE);
  return (e == 0 && converted > 0);
}

void WriteToLog(const char* text) {
  if (text == nullptr) {
    return;
  }

  DWORD bytes_written = 0;
  HANDLE file_handle = CreateFileW(LogFile, FILE_APPEND_DATA,
                                   FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                                   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (file_handle == INVALID_HANDLE_VALUE) {
    return;
  }

  WriteFile(file_handle, text, static_cast<DWORD>(strlen(text)),
            &bytes_written, NULL);
  CloseHandle(file_handle);
}

void WriteLogFormat(const char* format, ...) {
  char buffer[2048] = {0};
  va_list args;
  va_start(args, format);
  vsnprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
  va_end(args);
  WriteToLog(buffer);
}

HMODULE GetCurrentModule() {
  HMODULE h_module = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                    reinterpret_cast<LPCWSTR>(GetCurrentModule), &h_module);
  return h_module;
}

bool GetModuleCodeSectionInfo(HMODULE h_module,
                              PLATFORM_DWORD* base_addr,
                              PLATFORM_DWORD* base_size) {
  if (!h_module || !base_addr || !base_size) return false;

  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(h_module);
  if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) return false;
  const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
      reinterpret_cast<const BYTE*>(h_module) + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage == 0)
    return false;

  *base_addr = reinterpret_cast<PLATFORM_DWORD>(h_module);
  *base_size = static_cast<PLATFORM_DWORD>(nt->OptionalHeader.SizeOfImage);
  return true;
}

void SetThreadsState(bool resume) {
  HANDLE h = NULL;
  HANDLE h_thread = NULL;
  DWORD curr_thread_id = GetCurrentThreadId();
  DWORD curr_proc_id = GetCurrentProcessId();
  THREADENTRY32 thread = {};

  h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (h != INVALID_HANDLE_VALUE) {
    thread.dwSize = sizeof(THREADENTRY32);
    Thread32First(h, &thread);
    do {
      if (thread.th32ThreadID != curr_thread_id &&
          thread.th32OwnerProcessID == curr_proc_id) {
        h_thread = OpenThread(THREAD_SUSPEND_RESUME, false, thread.th32ThreadID);
        if (h_thread != NULL) {
          if (resume) {
            ResumeThread(h_thread);
          } else {
            SuspendThread(h_thread);
          }
          CloseHandle(h_thread);
        }
      }
    } while (Thread32Next(h, &thread));
    CloseHandle(h);
  }
}

bool PatchMemoryWrite(LPVOID addr, LPCVOID data, SIZE_T size) {
  if (!addr || !data || size == 0) return false;

  HANDLE hProc = GetCurrentProcess();
  DWORD oldProtect = 0;

  if (!VirtualProtectEx(hProc, addr, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    WriteLogFormat("PatchMemoryWrite: VirtualProtect failed at 0x%p (error %lu)\r\n",
                   addr, GetLastError());
    return false;
  }

  SIZE_T bytesWritten = 0;
  BOOL ok = WriteProcessMemory(hProc, addr, data, size, &bytesWritten);

  DWORD restored = 0;
  VirtualProtectEx(hProc, addr, size, oldProtect, &restored);

  if (!ok || bytesWritten != size) {
    WriteLogFormat("PatchMemoryWrite: WriteProcessMemory failed at 0x%p (%lu/%llu bytes, error %lu)\r\n",
                   addr, (ULONG_PTR)bytesWritten, (ULONGLONG)size, GetLastError());
    return false;
  }

  FlushInstructionCache(hProc, addr, size);
  return true;
}

bool PatchMemoryRead(LPVOID addr, LPVOID buf, SIZE_T size) {
  if (!addr || !buf || size == 0) return false;

  SIZE_T bytesRead = 0;
  if (!ReadProcessMemory(GetCurrentProcess(), addr, buf, size, &bytesRead) ||
      bytesRead != size) {
    WriteLogFormat("PatchMemoryRead: ReadProcessMemory failed at 0x%p (%lu/%llu bytes, error %lu)\r\n",
                   addr, (ULONG_PTR)bytesRead, (ULONGLONG)size, GetLastError());
    return false;
  }

  return true;
}

BOOL __stdcall GetModuleVersion(LPCWSTR lptstrModuleName,
                                FILE_VERSION* file_version) {
  if (!lptstrModuleName || !file_version) return false;
  HMODULE module = GetModuleHandleW(lptstrModuleName);
  if (!module) return false;

  std::vector<wchar_t> path(32768);
  const DWORD length = GetModuleFileNameW(module, path.data(),
                                          static_cast<DWORD>(path.size()));
  if (length == 0 || length >= path.size()) return false;
  return QueryProductVersion(path.data(), file_version);
}

BOOL __stdcall GetFileVersion(LPCWSTR lptstrFilename, FILE_VERSION* file_version) {
  return QueryProductVersion(lptstrFilename, file_version);
}
