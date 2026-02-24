#include "stdafx.h"

#include "rdpwrap_core.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <tlhelp32.h>

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

DWORD INIReadDWordHex(const ini::Parser& parser,
                      const char* sect,
                      const char* key,
                      PLATFORM_DWORD def_val) {
  std::string val_str = IniGetRaw(parser, sect, key, "");
  if (val_str.empty()) {
    return static_cast<DWORD>(def_val);
  }
  return static_cast<DWORD>(strtoul(val_str.c_str(), nullptr, 16));
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
  if (hex.size() > (max_len * 2)) hex.resize(max_len * 2);

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
  PIMAGE_DOS_HEADER p_dos_header;
  PIMAGE_FILE_HEADER p_file_header;
  PIMAGE_OPTIONAL_HEADER p_optional_header;

  if (h_module == NULL) return false;

  p_dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(h_module);
  p_file_header = reinterpret_cast<PIMAGE_FILE_HEADER>(
      reinterpret_cast<PBYTE>(h_module) + p_dos_header->e_lfanew + 4);
  p_optional_header = reinterpret_cast<PIMAGE_OPTIONAL_HEADER>(p_file_header + 1);

  *base_addr = reinterpret_cast<PLATFORM_DWORD>(h_module);
  *base_size = static_cast<PLATFORM_DWORD>(p_optional_header->SizeOfCode);

  if (*base_addr <= 0 || *base_size <= 0) return false;
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

BOOL __stdcall GetModuleVersion(LPCWSTR lptstrModuleName,
                                FILE_VERSION* file_version) {
  typedef struct {
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[16];
    WORD Padding1;
    VS_FIXEDFILEINFO Value;
    WORD Padding2;
    WORD Children;
  } VS_VERSIONINFO;

  HMODULE h_mod = GetModuleHandle(lptstrModuleName);
  if (!h_mod) {
    return false;
  }

  HRSRC h_resource_info = FindResourceW(h_mod, reinterpret_cast<LPCWSTR>(1),
                                        reinterpret_cast<LPCWSTR>(0x10));
  if (!h_resource_info) {
    return false;
  }

  VS_VERSIONINFO* version_info =
      reinterpret_cast<VS_VERSIONINFO*>(LoadResource(h_mod, h_resource_info));
  if (!version_info) {
    return false;
  }

  file_version->dwVersion = version_info->Value.dwFileVersionMS;
  file_version->Release =
      static_cast<WORD>(version_info->Value.dwFileVersionLS >> 16);
  file_version->Build = static_cast<WORD>(version_info->Value.dwFileVersionLS);

  return true;
}

BOOL __stdcall GetFileVersion(LPCWSTR lptstrFilename, FILE_VERSION* file_version) {
  typedef struct {
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[16];
    WORD Padding1;
    VS_FIXEDFILEINFO Value;
    WORD Padding2;
    WORD Children;
  } VS_VERSIONINFO;

  HMODULE h_file = LoadLibraryExW(lptstrFilename, NULL, LOAD_LIBRARY_AS_DATAFILE);
  if (!h_file) {
    return false;
  }

  HRSRC h_resource_info = FindResourceW(h_file, reinterpret_cast<LPCWSTR>(1),
                                        reinterpret_cast<LPCWSTR>(0x10));
  if (!h_resource_info) {
    return false;
  }

  VS_VERSIONINFO* version_info =
      reinterpret_cast<VS_VERSIONINFO*>(LoadResource(h_file, h_resource_info));
  if (!version_info) {
    return false;
  }

  file_version->dwVersion = version_info->Value.dwFileVersionMS;
  file_version->Release =
      static_cast<WORD>(version_info->Value.dwFileVersionLS >> 16);
  file_version->Build = static_cast<WORD>(version_info->Value.dwFileVersionLS);

  return true;
}
