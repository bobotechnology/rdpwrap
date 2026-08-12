// Copyright 2017 Stas'M Corp.
// Copyright 2025-2026 bobo
// Licensed under the Apache License, Version 2.0.

#include <windows.h>
#include "installer_version.h"
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <winsvc.h>
#include <wininet.h>
#include <aclapi.h>
#include <sddl.h>
#include <versionhelpers.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cwctype>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr wchar_t kTermService[] = L"TermService";
constexpr wchar_t kDefaultIniUrl[] = L"https://api.rpyf.top/rdpwrap.ini";
constexpr wchar_t kDefaultArmIniUrl[] =
    L"https://api.rpyf.top/rdpwrap-arm-kb.ini";
constexpr size_t kMaximumIniBytes = 16 * 1024 * 1024;
constexpr DWORD kNetworkTimeoutMs = 15000;
constexpr DWORD kServiceTimeoutMs = 30000;
constexpr wchar_t kConsolePidArgument[] = L"--rdpw-console-pid=";
constexpr wchar_t kOutputPipeArgument[] = L"--rdpw-output-pipe=";
constexpr wchar_t kUpdaterTaskName[] = L"RDPWUpdater";

struct FileVersion {
    WORD major = 0;
    WORD minor = 0;
    WORD release = 0;
    WORD build = 0;
};

struct ExitCode final {
    DWORD value;
};

enum class NativeArchitecture {
    Unknown,
    X86,
    X64,
    Arm32,
    Arm64,
};

bool installed = false;
bool online = false;
std::string onlineIniContent;
BYTE arch = 0;
NativeArchitecture nativeArchitecture = NativeArchitecture::Unknown;
std::wstring wrapPath;
std::wstring termServicePath;
FileVersion fileVersion;
DWORD termServicePid = 0;
std::vector<std::wstring> sharedServices;

bool isArmArchitecture() {
    return nativeArchitecture == NativeArchitecture::Arm32 ||
           nativeArchitecture == NativeArchitecture::Arm64;
}

const wchar_t* configurationFileName() {
    return isArmArchitecture() ? L"rdpwrap-arm-kb.ini" : L"rdpwrap.ini";
}

const wchar_t* configurationResourceName() {
    return isArmArchitecture() ? L"CONFIG_ARM" : L"CONFIG";
}

const wchar_t* wrapperResourceName() {
    switch (nativeArchitecture) {
    case NativeArchitecture::X86: return L"RDPW32";
    case NativeArchitecture::X64: return L"RDPW64";
    case NativeArchitecture::Arm32: return L"RDPWARM";
    case NativeArchitecture::Arm64: return L"RDPWARM64";
    default: return L"";
    }
}

template <typename Function>
Function loadFunction(HMODULE module, const char* name) {
    FARPROC address = GetProcAddress(module, name);
    Function function = nullptr;
    static_assert(sizeof(function) == sizeof(address));
    std::memcpy(&function, &address, sizeof(function));
    return function;
}

[[noreturn]] void halt(DWORD code) { throw ExitCode{code}; }

std::wstring lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return value;
}

bool containsInsensitive(const std::wstring& value, const std::wstring& needle) {
    return lower(value).find(lower(needle)) != std::wstring::npos;
}

std::optional<std::wstring> readRegistryString(
    const wchar_t* subKey, const wchar_t* name);

std::wstring expandPath(std::wstring path) {
    const auto nativeProgramFiles = readRegistryString(
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", L"ProgramFilesDir");
    if (nativeProgramFiles && !nativeProgramFiles->empty()) {
        const std::wstring from = L"%ProgramFiles%";
        auto pos = lower(path).find(lower(from));
        if (pos != std::wstring::npos)
            path.replace(pos, from.size(), *nativeProgramFiles);
    }
#if !defined(_WIN64)
    // Use the documented virtual alias instead of disabling WOW64 file-system
    // redirection across a large block of unrelated installer operations.
    if (arch == 64) {
        const std::wstring from = L"%SystemRoot%\\System32";
        const std::wstring to = L"%SystemRoot%\\Sysnative";
        const auto pos = lower(path).find(lower(from));
        if (pos != std::wstring::npos) path.replace(pos, from.size(), to);
    }
#endif
    DWORD size = ExpandEnvironmentStringsW(path.c_str(), nullptr, 0);
    if (!size) return {};
    std::wstring result(size, L'\0');
    const DWORD written = ExpandEnvironmentStringsW(path.c_str(), result.data(), size);
    if (!written || written > size) return {};
    result.resize(written - 1);
    return result;
}

std::wstring executablePath() {
    std::wstring path(32768, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!size || size >= path.size()) return {};
    path.resize(size);
    return path;
}

std::wstring parentPath(std::wstring path) {
    while (path.size() > 3 && (path.back() == L'\\' || path.back() == L'/'))
        path.pop_back();
    const size_t separator = path.find_last_of(L"\\/");
    if (separator == std::wstring::npos) return {};
    if (separator == 2 && path.size() >= 3 && path[1] == L':') return path.substr(0, 3);
    return path.substr(0, separator);
}

std::wstring joinPath(const std::wstring& folder, const std::wstring& name) {
    if (folder.empty()) return name;
    if (folder.back() == L'\\' || folder.back() == L'/') return folder + name;
    return folder + L'\\' + name;
}

bool pathExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool directoryExists(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool samePath(std::wstring first, std::wstring second) {
    while (first.size() > 3 && (first.back() == L'\\' || first.back() == L'/'))
        first.pop_back();
    while (second.size() > 3 && (second.back() == L'\\' || second.back() == L'/'))
        second.pop_back();
    return _wcsicmp(first.c_str(), second.c_str()) == 0;
}

REGSAM registryView(REGSAM access) {
    return access | (arch == 64 ? KEY_WOW64_64KEY : 0);
}

std::optional<std::wstring> readRegistryString(const wchar_t* subKey, const wchar_t* name) {
    HKEY key = nullptr;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, registryView(KEY_QUERY_VALUE), &key);
    if (result != ERROR_SUCCESS) {
        SetLastError(result);
        return std::nullopt;
    }
    DWORD type = 0;
    DWORD bytes = 0;
    result = RegQueryValueExW(key, name, nullptr, &type, nullptr, &bytes);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        RegCloseKey(key);
        SetLastError(result != ERROR_SUCCESS ? result : ERROR_DATATYPE_MISMATCH);
        return std::nullopt;
    }
    if (bytes % sizeof(wchar_t) != 0) {
        RegCloseKey(key);
        SetLastError(ERROR_INVALID_DATA);
        return std::nullopt;
    }
    std::wstring value(bytes / sizeof(wchar_t) + 1, L'\0');
    result = RegQueryValueExW(key, name, nullptr, &type,
        reinterpret_cast<BYTE*>(value.data()), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        SetLastError(result);
        return std::nullopt;
    }
    value.resize(bytes / sizeof(wchar_t));
    while (!value.empty() && value.back() == L'\0') value.pop_back();
    return value;
}

std::optional<DWORD> readRegistryDword(const wchar_t* subKey, const wchar_t* name) {
    HKEY key = nullptr;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0,
        registryView(KEY_QUERY_VALUE), &key);
    if (result != ERROR_SUCCESS) {
        SetLastError(result);
        return std::nullopt;
    }
    DWORD type = 0;
    DWORD value = 0;
    DWORD bytes = sizeof(value);
    result = RegQueryValueExW(key, name, nullptr, &type,
        reinterpret_cast<BYTE*>(&value), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_DWORD || bytes != sizeof(value)) {
        SetLastError(result != ERROR_SUCCESS ? result : ERROR_DATATYPE_MISMATCH);
        return std::nullopt;
    }
    return value;
}

bool writeRegistryValue(const wchar_t* subKey, const wchar_t* name, DWORD type,
                        const void* data, DWORD bytes) {
    HKEY key = nullptr;
    DWORD disposition = 0;
    LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, nullptr, 0,
        registryView(KEY_SET_VALUE | KEY_CREATE_SUB_KEY), nullptr, &key, &disposition);
    if (result != ERROR_SUCCESS) {
        SetLastError(result);
        return false;
    }
    result = RegSetValueExW(key, name, 0, type, static_cast<const BYTE*>(data), bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) SetLastError(result);
    return result == ERROR_SUCCESS;
}

bool writeRegistryString(const wchar_t* key, const wchar_t* name,
                         const std::wstring& value, DWORD type = REG_SZ) {
    return writeRegistryValue(key, name, type, value.c_str(),
        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
}

bool writeRegistryDword(const wchar_t* key, const wchar_t* name, DWORD value) {
    return writeRegistryValue(key, name, REG_DWORD, &value, sizeof(value));
}

NativeArchitecture architectureFromMachine(USHORT machine) {
    switch (machine) {
    case IMAGE_FILE_MACHINE_I386: return NativeArchitecture::X86;
    case IMAGE_FILE_MACHINE_AMD64: return NativeArchitecture::X64;
    case IMAGE_FILE_MACHINE_ARMNT: return NativeArchitecture::Arm32;
    case IMAGE_FILE_MACHINE_ARM64: return NativeArchitecture::Arm64;
    default: return NativeArchitecture::Unknown;
    }
}

NativeArchitecture architectureFromProcessor(WORD processorArchitecture) {
    switch (processorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL: return NativeArchitecture::X86;
    case PROCESSOR_ARCHITECTURE_AMD64: return NativeArchitecture::X64;
    case PROCESSOR_ARCHITECTURE_ARM: return NativeArchitecture::Arm32;
    case PROCESSOR_ARCHITECTURE_ARM64: return NativeArchitecture::Arm64;
    default: return NativeArchitecture::Unknown;
    }
}

bool supportedArchitecture() {
    // GetNativeSystemInfo can report the emulated architecture to an x86/x64
    // process on ARM64. IsWow64Process2 reports the actual host machine.
    using IsWow64Process2Function = BOOL(WINAPI*)(HANDLE, USHORT*, USHORT*);
    const auto isWow64Process2 = loadFunction<IsWow64Process2Function>(
        GetModuleHandleW(L"kernel32.dll"), "IsWow64Process2");
    if (isWow64Process2) {
        USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
        USHORT nativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;
        if (isWow64Process2(GetCurrentProcess(), &processMachine, &nativeMachine))
            nativeArchitecture = architectureFromMachine(nativeMachine);
    }

    if (nativeArchitecture == NativeArchitecture::Unknown) {
        SYSTEM_INFO info{};
        GetNativeSystemInfo(&info);
        nativeArchitecture = architectureFromProcessor(info.wProcessorArchitecture);
    }

    arch = nativeArchitecture == NativeArchitecture::X86 ||
           nativeArchitecture == NativeArchitecture::Arm32 ? 32 :
           nativeArchitecture == NativeArchitecture::X64 ||
           nativeArchitecture == NativeArchitecture::Arm64 ? 64 : 0;
    return arch != 0;
}

void checkInstall() {
    constexpr wchar_t serviceKey[] = L"SYSTEM\\CurrentControlSet\\Services\\TermService";
    constexpr wchar_t parametersKey[] = L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters";
    auto host = readRegistryString(serviceKey, L"ImagePath");
    if (!host) {
        DWORD code = GetLastError();
        std::wcout << L"[-] OpenKeyReadOnly error (code " << code << L").\n";
        halt(code);
    }
    if (!containsInsensitive(*host, L"svchost.exe") && !containsInsensitive(*host, L"svchost -k")) {
        std::wcout << L"[-] TermService is hosted in a custom application (BeTwin, etc.) - unsupported.\n"
                   << L"[*] ImagePath: \"" << *host << L"\".\n";
        halt(ERROR_NOT_SUPPORTED);
    }
    auto serviceDll = readRegistryString(parametersKey, L"ServiceDll");
    if (!serviceDll) {
        DWORD code = GetLastError();
        std::wcout << L"[-] OpenKeyReadOnly error (code " << code << L").\n";
        halt(code);
    }
    if (!containsInsensitive(*serviceDll, L"termsrv.dll") &&
        !containsInsensitive(*serviceDll, L"rdpwrap.dll")) {
        std::wcout << L"[-] Another third-party TermService library is installed.\n"
                   << L"[*] ServiceDll: \"" << *serviceDll << L"\".\n";
        halt(ERROR_NOT_SUPPORTED);
    }
    termServicePath = *serviceDll;
    installed = containsInsensitive(termServicePath, L"rdpwrap.dll");
}

int serviceStartType(const std::wstring& name) {
    std::wcout << L"[*] Checking " << name << L"...\n";
    SC_HANDLE scm = OpenSCManagerW(nullptr, SERVICES_ACTIVE_DATABASEW, SC_MANAGER_CONNECT);
    if (!scm) return -1;
    SC_HANDLE service = OpenServiceW(scm, name.c_str(), SERVICE_QUERY_CONFIG);
    if (!service) { CloseServiceHandle(scm); return -1; }
    DWORD needed = 0;
    if (QueryServiceConfigW(service, nullptr, 0, &needed) ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER || needed == 0) {
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return -1;
    }
    std::vector<BYTE> buffer(needed);
    bool ok = QueryServiceConfigW(service,
        reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer.data()), needed, &needed) != FALSE;
    int result = ok ? static_cast<int>(
        reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer.data())->dwStartType) : -1;
    if (!ok) std::wcout << L"[-] QueryServiceConfig error (code " << GetLastError() << L").\n";
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return result;
}

void configureServiceStart(const std::wstring& name, DWORD startType) {
    std::wcout << L"[*] Configuring " << name << L"...\n";
    SC_HANDLE scm = OpenSCManagerW(nullptr, SERVICES_ACTIVE_DATABASEW, SC_MANAGER_CONNECT);
    if (!scm) { std::wcout << L"[-] OpenSCManager error (code " << GetLastError() << L").\n"; return; }
    SC_HANDLE service = OpenServiceW(scm, name.c_str(), SERVICE_CHANGE_CONFIG);
    if (!service) { std::wcout << L"[-] OpenService error (code " << GetLastError() << L").\n"; CloseServiceHandle(scm); return; }
    if (!ChangeServiceConfigW(service, SERVICE_NO_CHANGE, startType, SERVICE_NO_CHANGE,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
        std::wcout << L"[-] ChangeServiceConfig error (code " << GetLastError() << L").\n";
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}

bool startService(const std::wstring& name) {
    std::wcout << L"[*] Starting " << name << L"...\n";
    SC_HANDLE scm = OpenSCManagerW(nullptr, SERVICES_ACTIVE_DATABASEW, SC_MANAGER_CONNECT);
    if (!scm) { std::wcout << L"[-] OpenSCManager error (code " << GetLastError() << L").\n"; return false; }
    SC_HANDLE service = OpenServiceW(scm, name.c_str(), SERVICE_QUERY_STATUS | SERVICE_START);
    if (!service) { std::wcout << L"[-] OpenService error (code " << GetLastError() << L").\n"; CloseServiceHandle(scm); return false; }
    SERVICE_STATUS status{};
    bool ok = QueryServiceStatus(service, &status) != FALSE;
    if (!ok)
        std::wcout << L"[-] QueryServiceStatus error (code " << GetLastError() << L").\n";
    else if (status.dwCurrentState == SERVICE_RUNNING)
        std::wcout << L"[+] Service is already running.\n";
    else {
        ok = StartServiceW(service, 0, nullptr) != FALSE;
        if (!ok && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) ok = true;
        if (!ok) {
            std::wcout << L"[-] StartService error (code " << GetLastError() << L").\n";
        } else {
            DWORD waited = 0;
            do {
                Sleep(100);
                waited += 100;
                ok = QueryServiceStatus(service, &status) != FALSE;
            } while (ok && status.dwCurrentState == SERVICE_START_PENDING &&
                     waited < kServiceTimeoutMs);
            ok = ok && status.dwCurrentState == SERVICE_RUNNING;
            if (!ok) std::wcout << L"[-] Service did not reach the running state.\n";
        }
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ok;
}

std::vector<ENUM_SERVICE_STATUS_PROCESSW> enumerateServices(std::vector<BYTE>& storage) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, SERVICES_ACTIVE_DATABASEW,
        SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) halt(GetLastError());
    DWORD needed = 0, returned = 0, resume = 0;
    if (EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
        nullptr, 0, &needed, &returned, &resume, nullptr) ||
        GetLastError() != ERROR_MORE_DATA || needed == 0) {
        DWORD code = GetLastError();
        CloseServiceHandle(scm);
        halt(code ? code : ERROR_INVALID_DATA);
    }
    storage.resize(needed);
    for (;;) {
        resume = 0;
        returned = 0;
        if (EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                SERVICE_STATE_ALL, storage.data(), static_cast<DWORD>(storage.size()),
                &needed, &returned, &resume, nullptr)) break;
        const DWORD code = GetLastError();
        if (code != ERROR_MORE_DATA) {
            CloseServiceHandle(scm);
            halt(code);
        }
        const size_t grown = std::max<size_t>(needed, storage.size() * 2);
        if (grown > MAXDWORD) {
            CloseServiceHandle(scm);
            halt(ERROR_NOT_ENOUGH_MEMORY);
        }
        storage.resize(grown);
    }
    CloseServiceHandle(scm);
    auto entries = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(storage.data());
    return {entries, entries + returned};
}

void checkTermsrvProcess() {
    bool started = false;
    for (;;) {
        std::vector<BYTE> storage;
        auto services = enumerateServices(storage);
        auto found = std::find_if(services.begin(), services.end(), [](const auto& item) {
            return _wcsicmp(item.lpServiceName, kTermService) == 0;
        });
        if (found == services.end()) {
            std::wcout << L"[-] TermService not found.\n";
            halt(ERROR_SERVICE_DOES_NOT_EXIST);
        }
        termServicePid = found->ServiceStatusProcess.dwProcessId;
        std::wstring actualName = found->lpServiceName;
        if (!termServicePid) {
            if (started) halt(ERROR_SERVICE_NOT_ACTIVE);
            configureServiceStart(kTermService, SERVICE_AUTO_START);
            if (!startService(kTermService)) halt(ERROR_SERVICE_NOT_ACTIVE);
            started = true;
            continue;
        }
        std::wcout << L"[+] TermService found (pid " << termServicePid << L").\n";
        sharedServices.clear();
        for (const auto& service : services) {
            if (service.ServiceStatusProcess.dwProcessId == termServicePid &&
                _wcsicmp(service.lpServiceName, actualName.c_str()) != 0)
                sharedServices.emplace_back(service.lpServiceName);
        }
        if (sharedServices.empty()) std::wcout << L"[*] No shared services found.\n";
        else {
            std::wcout << L"[*] Shared services found: ";
            for (size_t i = 0; i < sharedServices.size(); ++i)
                std::wcout << (i ? L", " : L"") << sharedServices[i];
            std::wcout << L"\n";
        }
        return;
    }
}

bool addPrivilege(const wchar_t* privilege) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return false;
    TOKEN_PRIVILEGES privileges{};
    privileges.PrivilegeCount = 1;
    bool ok = LookupPrivilegeValueW(nullptr, privilege, &privileges.Privileges[0].Luid) != FALSE;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (ok) {
        SetLastError(ERROR_SUCCESS);
        ok = AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr) &&
             GetLastError() == ERROR_SUCCESS;
    }
    CloseHandle(token);
    return ok;
}

bool stopService(const wchar_t* name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE service = OpenServiceW(scm, name, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) { CloseServiceHandle(scm); return false; }
    SERVICE_STATUS status{};
    bool ok = QueryServiceStatus(service, &status) != FALSE;
    if (ok && status.dwCurrentState != SERVICE_STOPPED)
        ok = ControlService(service, SERVICE_CONTROL_STOP, &status) != FALSE;
    DWORD waited = 0;
    while (ok && status.dwCurrentState != SERVICE_STOPPED && waited < kServiceTimeoutMs) {
        Sleep(100);
        waited += 100;
        ok = QueryServiceStatus(service, &status) != FALSE;
    }
    ok = ok && status.dwCurrentState == SERVICE_STOPPED;
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ok;
}

void killProcess(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (!process) halt(GetLastError());
    if (!TerminateProcess(process, 0)) {
        DWORD code = GetLastError(); CloseHandle(process); halt(code);
    }
    const DWORD waitResult = WaitForSingleObject(process, kServiceTimeoutMs);
    CloseHandle(process);
    if (waitResult != WAIT_OBJECT_0)
        halt(waitResult == WAIT_TIMEOUT ? ERROR_TIMEOUT : ERROR_GEN_FAILURE);
}

bool execWait(const std::wstring& commandLine, bool reportFailure = true) {
    // CreateProcess requires a writable command-line buffer.
    std::wstring command = commandLine;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &startup, &process)) {
        std::wcout << L"[-] CreateProcess error (code: " << GetLastError() << L").\n";
        return false;
    }
    CloseHandle(process.hThread);
    const DWORD waitResult = WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = ERROR_GEN_FAILURE;
    const bool ok = waitResult == WAIT_OBJECT_0 &&
        GetExitCodeProcess(process.hProcess, &exitCode) && exitCode == 0;
    CloseHandle(process.hProcess);
    if (!ok && reportFailure)
        std::wcout << L"[-] Command failed (exit code " << exitCode << L").\n";
    return ok;
}

std::wstring installedInstallerPath() {
    return expandPath(L"%ProgramFiles%\\RDP Wrapper\\RDPWInst.exe");
}

bool deployUpdater() {
    const std::wstring source = executablePath();
    const std::wstring destination = installedInstallerPath();
    const std::wstring folder = parentPath(destination);
    if (!directoryExists(folder)) {
        const int result = SHCreateDirectoryExW(nullptr, folder.c_str(), nullptr);
        if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS &&
            result != ERROR_FILE_EXISTS) {
            std::wcout << L"[-] Failed to create updater directory (code "
                       << result << L").\n";
            return false;
        }
    }
    if (!samePath(source, destination) &&
        !CopyFileW(source.c_str(), destination.c_str(), FALSE)) {
        std::wcout << L"[-] Failed to deploy updater executable (code "
                   << GetLastError() << L").\n";
        return false;
    }

    const std::wstring schtasks =
        L"\"" + expandPath(L"%SystemRoot%\\System32\\schtasks.exe") + L"\"";
    const std::wstring command = schtasks +
        L" /CREATE /F /SC ONSTART /DELAY 0002:00 /TN \"" + kUpdaterTaskName +
        L"\" /TR \"\\\"" + destination +
        L"\\\" -w\" /RL HIGHEST /RU SYSTEM /NP";
    if (execWait(command)) return true;

    if (!samePath(source, destination)) DeleteFileW(destination.c_str());
    return false;
}

void removeUpdater() {
    const std::wstring schtasks =
        L"\"" + expandPath(L"%SystemRoot%\\System32\\schtasks.exe") + L"\"";
    execWait(schtasks + L" /DELETE /TN \"" + kUpdaterTaskName + L"\" /F", false);

    const std::wstring path = installedInstallerPath();
    const std::wstring folder = parentPath(path);
    if (pathExists(path)) {
        if (samePath(path, executablePath())) {
            if (!MoveFileExW(path.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT))
                std::wcout << L"[-] Failed to schedule updater removal (code "
                           << GetLastError() << L").\n";
        } else if (!DeleteFileW(path.c_str())) {
            std::wcout << L"[-] Failed to remove updater executable (code "
                       << GetLastError() << L").\n";
        }
    }
    if (!RemoveDirectoryW(folder.c_str()) && samePath(path, executablePath()))
        MoveFileExW(folder.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
}

void setWrapperDll() {
    constexpr wchar_t key[] = L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters";
    if (!writeRegistryString(key, L"ServiceDll", wrapPath, REG_EXPAND_SZ)) {
        std::wcout << L"[-] WriteExpandString error.\n"; halt(ERROR_ACCESS_DENIED);
    }
    if (arch == 64 && fileVersion.major == 6 && fileVersion.minor == 0) {
        if (!execWait(L"\"" + expandPath(L"%SystemRoot%\\System32\\reg.exe") +
            L"\" add HKLM\\SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters "
            L"/v ServiceDll /t REG_EXPAND_SZ /d \"" + wrapPath + L"\" /f"))
            halt(ERROR_GEN_FAILURE);
    }
}

void resetServiceDll() {
    if (!writeRegistryString(L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters",
        L"ServiceDll", L"%SystemRoot%\\System32\\termsrv.dll", REG_EXPAND_SZ)) {
        std::wcout << L"[-] Registry error (code " << GetLastError() << L").\n";
        halt(ERROR_ACCESS_DENIED);
    }
}

std::vector<BYTE> resourceBytes(const wchar_t* name) {
    HRSRC resource = FindResourceW(nullptr, name, RT_RCDATA);
    if (!resource) return {};
    HGLOBAL loaded = LoadResource(nullptr, resource);
    const auto* data = static_cast<const BYTE*>(LockResource(loaded));
    DWORD size = SizeofResource(nullptr, resource);
    return data ? std::vector<BYTE>(data, data + size) : std::vector<BYTE>{};
}

bool extractResource(const wchar_t* name, const std::wstring& destination) {
    auto bytes = resourceBytes(name);
    if (bytes.empty()) {
        std::wcout << L"[-] Failed to load resource " << name << L".\n";
        return false;
    }
    HANDLE file = CreateFileW(destination.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written = 0;
    const bool ok = file != INVALID_HANDLE_VALUE &&
        bytes.size() <= MAXDWORD &&
        WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
        written == static_cast<DWORD>(bytes.size()) && FlushFileBuffers(file);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (!ok) {
        DeleteFileW(destination.c_str());
        std::wcout << L"[-] Failed to extract file.\n[*] Resource name: " << name
                   << L"\n[*] Destination path: " << destination << L"\n";
        return false;
    }
    std::wcout << L"[+] Extracted " << name << L" -> " << destination << L"\n";
    return true;
}

std::wstring decodeText(const std::vector<BYTE>& bytes) {
    if (bytes.empty()) return {};
    size_t offset = bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF ? 3 : 0;
    UINT codePage = CP_UTF8;
    int size = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char*>(bytes.data() + offset), static_cast<int>(bytes.size() - offset), nullptr, 0);
    if (!size) {
        codePage = CP_ACP;
        size = MultiByteToWideChar(codePage, 0, reinterpret_cast<const char*>(bytes.data() + offset),
            static_cast<int>(bytes.size() - offset), nullptr, 0);
    }
    std::wstring result(size, L'\0');
    MultiByteToWideChar(codePage, 0, reinterpret_cast<const char*>(bytes.data() + offset),
        static_cast<int>(bytes.size() - offset), result.data(), size);
    return result;
}

std::wstring resourceText(const wchar_t* name) { return decodeText(resourceBytes(name)); }

bool downloadIni(std::string& content, const std::wstring& source) {
    content.clear();
    HINTERNET internet = InternetOpenW(L"RDP Wrapper Update", INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr, nullptr, 0);
    if (!internet) return false;
    DWORD timeout = kNetworkTimeoutMs;
    InternetSetOptionW(internet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(internet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(internet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    const std::wstring url = source.empty()
        ? std::wstring(isArmArchitecture() ? kDefaultArmIniUrl : kDefaultIniUrl)
        : source;
    HINTERNET request = InternetOpenUrlW(internet, url.c_str(), nullptr, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!request) { InternetCloseHandle(internet); return false; }
    if (_wcsnicmp(url.c_str(), L"http://", 7) == 0 ||
        _wcsnicmp(url.c_str(), L"https://", 8) == 0) {
        DWORD status = 0;
        DWORD statusBytes = sizeof(status);
        if (!HttpQueryInfoW(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                &status, &statusBytes, nullptr) || status < 200 || status >= 300) {
            InternetCloseHandle(request);
            InternetCloseHandle(internet);
            return false;
        }
    }
    char buffer[4096];
    DWORD count = 0;
    bool ok = true;
    do {
        if (!InternetReadFile(request, buffer, sizeof(buffer), &count)) { ok = false; break; }
        if (content.size() > kMaximumIniBytes - count) { ok = false; break; }
        content.append(buffer, count);
    } while (count);
    InternetCloseHandle(request);
    InternetCloseHandle(internet);
    return ok;
}

bool validIniContent(const std::string& content) {
    if (content.empty() || content.size() > kMaximumIniBytes ||
        content.find('\0') != std::string::npos) return false;
    const bool hasMain = content.find("[Main]") != std::string::npos;
    const bool hasPolicy = content.find("[SLPolicy]") != std::string::npos;
    const size_t updated = content.find("Updated=");
    if (!hasMain || !hasPolicy || updated == std::string::npos ||
        (updated != 0 && content[updated - 1] != '\n' && content[updated - 1] != '\r'))
        return false;
    const size_t value = updated + std::strlen("Updated=");
    if (value + 10 > content.size()) return false;
    for (size_t index = 0; index < 10; ++index) {
        const char character = content[value + index];
        if ((index == 4 || index == 7) ? character != '-' : !std::isdigit(
                static_cast<unsigned char>(character))) return false;
    }
    return value + 10 == content.size() || content[value + 10] == '\r' ||
           content[value + 10] == '\n';
}

std::optional<std::string> readValidatedIni(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return std::nullopt;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 ||
        size.QuadPart > static_cast<LONGLONG>(kMaximumIniBytes)) {
        CloseHandle(file);
        return std::nullopt;
    }
    std::string content(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    const bool ok = ReadFile(file, content.data(), static_cast<DWORD>(content.size()),
                             &read, nullptr) &&
                    read == static_cast<DWORD>(content.size());
    CloseHandle(file);
    if (!ok || !validIniContent(content)) return std::nullopt;
    return content;
}

void grantSidFullAccess(const std::wstring& path, const wchar_t* sidText) {
    PSID sid = nullptr;
    if (!ConvertStringSidToSidW(sidText, &sid)) return;
    EXPLICIT_ACCESSW access{};
    access.grfAccessPermissions = GENERIC_ALL;
    access.grfAccessMode = GRANT_ACCESS;
    access.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    access.Trustee.ptstrName = static_cast<LPWSTR>(sid);
    PACL oldAcl = nullptr;
    PACL acl = nullptr;
    PSECURITY_DESCRIPTOR securityDescriptor = nullptr;
    DWORD result = GetNamedSecurityInfoW(const_cast<LPWSTR>(path.c_str()),
        SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &oldAcl,
        nullptr, &securityDescriptor);
    if (result == ERROR_SUCCESS)
        result = SetEntriesInAclW(1, &access, oldAcl, &acl);
    if (result == ERROR_SUCCESS)
        result = SetNamedSecurityInfoW(const_cast<LPWSTR>(path.c_str()), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION, nullptr, nullptr, acl, nullptr);
    if (result != ERROR_SUCCESS)
        std::wcout << L"[-] Failed to grant file access (code " << result << L").\n";
    if (acl) LocalFree(acl);
    if (securityDescriptor) LocalFree(securityDescriptor);
    LocalFree(sid);
}

bool writeBytes(const std::wstring& path, const std::string& content) {
    const std::wstring temporary = path + L".tmp-" +
        std::to_wstring(GetCurrentProcessId()) + L"-" +
        std::to_wstring(GetTickCount64());
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written = 0;
    const bool ok = file != INVALID_HANDLE_VALUE && content.size() <= MAXDWORD &&
        WriteFile(file, content.data(), static_cast<DWORD>(content.size()),
                  &written, nullptr) &&
        written == static_cast<DWORD>(content.size()) && FlushFileBuffers(file);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (!ok) {
        DeleteFileW(temporary.c_str());
        return false;
    }
    if (!MoveFileExW(temporary.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary.c_str());
        return false;
    }
    return true;
}

void extractFiles() {
    const std::wstring folder = parentPath(expandPath(wrapPath));
    if (!directoryExists(folder)) {
        const int result = SHCreateDirectoryExW(nullptr, folder.c_str(), nullptr);
        if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS &&
            result != ERROR_FILE_EXISTS) {
            std::wcout << L"[-] ForceDirectories error.\n";
            halt(static_cast<DWORD>(result));
        }
        std::wcout << L"[+] Folder created: " << folder << L"\n";
        grantSidFullAccess(folder, L"S-1-5-18");
        grantSidFullAccess(folder, L"S-1-5-6");
    }

    const std::wstring iniDestination = joinPath(folder, configurationFileName());
    if (online) {
        if (!onlineIniContent.empty() && writeBytes(iniDestination, onlineIniContent))
            std::wcout << L"[+] Latest INI file -> " << iniDestination << L"\n";
        else {
            std::wcout << L"[-] Failed to get online INI file, using built-in.\n";
            online = false;
        }
    }
    if (!online) {
        const std::wstring adjacentIni =
            joinPath(parentPath(executablePath()), configurationFileName());
        if (pathExists(adjacentIni)) {
            const auto content = readValidatedIni(adjacentIni);
            if (content && writeBytes(iniDestination, *content)) {
                std::wcout << L"[+] Current INI file -> " << iniDestination << L"\n";
            } else {
                std::wcout << L"[-] Adjacent INI is invalid; using built-in configuration.\n";
                if (!extractResource(configurationResourceName(), iniDestination))
                    halt(ERROR_RESOURCE_DATA_NOT_FOUND);
            }
        } else if (!extractResource(configurationResourceName(), iniDestination)) {
            halt(ERROR_RESOURCE_DATA_NOT_FOUND);
        }
    }

    if (!extractResource(wrapperResourceName(), expandPath(wrapPath)))
        halt(ERROR_RESOURCE_DATA_NOT_FOUND);
    const wchar_t* clip = nullptr;
    const wchar_t* rfx = nullptr;
    if (!isArmArchitecture()) {
        if (fileVersion.major == 6 && fileVersion.minor == 0)
            clip = arch == 32 ? L"RDPCLIP6032" : L"RDPCLIP6064";
        if (fileVersion.major == 6 && fileVersion.minor == 1)
            clip = arch == 32 ? L"RDPCLIP6132" : L"RDPCLIP6164";
        if (fileVersion.major == 10 && fileVersion.minor == 0)
            rfx = arch == 32 ? L"RFXVMT32" : L"RFXVMT64";
    }
    std::wstring clipPath = expandPath(L"%SystemRoot%\\System32\\rdpclip.exe");
    std::wstring rfxPath = expandPath(L"%SystemRoot%\\System32\\rfxvmt.dll");
    if (clip && !pathExists(clipPath) && !extractResource(clip, clipPath))
        halt(ERROR_RESOURCE_DATA_NOT_FOUND);
    if (rfx && !pathExists(rfxPath) && !extractResource(rfx, rfxPath))
        halt(ERROR_RESOURCE_DATA_NOT_FOUND);
}

bool deleteFiles() {
    const std::wstring dll = expandPath(termServicePath);
    const std::wstring folder = parentPath(dll);
    bool ok = true;
    for (const std::wstring& file : {
            joinPath(folder, configurationFileName()),
            joinPath(folder, L"rdpwrap.txt"), dll,
            expandPath(L"%ProgramFiles%\\RDP Wrapper\\RDP_CnC.exe")}) {
        if (!pathExists(file)) continue;
        if (DeleteFileW(file.c_str()))
            std::wcout << L"[+] Removed file: " << file << L"\n";
        else {
            const DWORD code = GetLastError();
            std::wcout << L"[-] DeleteFile error (code " << code
                       << L") for file: " << file << L"\n";
            ok = false;
        }
    }
    const std::wstring installFolder = expandPath(L"%ProgramFiles%\\RDP Wrapper");
    if (samePath(folder, installFolder) && RemoveDirectoryW(folder.c_str())) {
        std::wcout << L"[+] Removed folder: " << folder << L"\n";
    }
    return ok;
}

bool getFileVersion(const std::wstring& path, FileVersion& version) {
    DWORD ignored = 0;
    DWORD size = GetFileVersionInfoSizeW(path.c_str(), &ignored);
    if (!size) return false;
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) return false;
    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &infoSize) ||
        !info || infoSize < sizeof(VS_FIXEDFILEINFO) ||
        info->dwSignature != VS_FFI_SIGNATURE) return false;
    // RDP Wrapper INI sections use the termsrv product-version tuple.
    version.major = HIWORD(info->dwProductVersionMS);
    version.minor = LOWORD(info->dwProductVersionMS);
    version.release = HIWORD(info->dwProductVersionLS);
    version.build = LOWORD(info->dwProductVersionLS);
    return true;
}

void checkTermsrvVersion() {
    if (!getFileVersion(expandPath(termServicePath), fileVersion)) {
        std::wcout << L"[-] Failed to read Terminal Services version.\n";
        halt(GetLastError());
    }
    std::wstringstream version;
    version << fileVersion.major << L'.' << fileVersion.minor << L'.'
            << fileVersion.release << L'.' << fileVersion.build;
    std::wcout << L"[*] Terminal Services version: " << version.str() << L"\n";
    if (fileVersion.major == 5 && fileVersion.minor == 1) {
        if (arch == 32) {
            std::wcout << L"[!] Windows XP is not supported.\n"
                          L"You may take a look at RDP Realtime Patch by Stas'M for Windows XP\n"
                          L"Link: http://stascorp.com/load/1-1-0-62\n";
        } else {
            std::wcout << L"[!] Windows XP 64-bit Edition is not supported.\n";
        }
        return;
    }
    if (fileVersion.major == 5 && fileVersion.minor == 2) {
        std::wcout << (arch == 32
            ? L"[!] Windows Server 2003 is not supported.\n"
            : L"[!] Windows Server 2003 or XP 64-bit Edition is not supported.\n");
        return;
    }
    int support = 0;
    if (fileVersion.major == 6 && fileVersion.minor == 0) {
        support = 1;
        if (arch == 32 && fileVersion.release == 6000 && fileVersion.build == 16386) {
            std::wcout << L"[!] This version of Terminal Services may crash on logon attempt.\n"
                          L"It's recommended to upgrade to Service Pack 1 or higher.\n";
        }
    }
    if (fileVersion.major == 6 && fileVersion.minor == 1) support = 1;
    std::wstring configText;
    if (online && !onlineIniContent.empty()) {
        const std::vector<BYTE> bytes(onlineIniContent.begin(), onlineIniContent.end());
        configText = decodeText(bytes);
    } else {
        const std::wstring adjacentIni =
            joinPath(parentPath(executablePath()), configurationFileName());
        const auto adjacentContent = readValidatedIni(adjacentIni);
        if (adjacentContent) {
            const std::vector<BYTE> bytes(adjacentContent->begin(), adjacentContent->end());
            configText = decodeText(bytes);
        } else {
            configText = resourceText(configurationResourceName());
        }
    }
    if (configText.find(L"[" + version.str() + L"]") != std::wstring::npos) support = 2;
    if (support == 2) std::wcout << L"[+] This version of Terminal Services is fully supported.\n";
    else {
        std::wcout << (support == 1
            ? L"[!] This version of Terminal Services is supported partially.\n"
            : L"[-] This version of Terminal Services is not supported.\n");
        if (support == 1)
            std::wcout << L"It means you may have some limitations such as only 2 concurrent sessions.\n";
        std::wcout << L"Try running \"update.bat\" or \"RDPWInst -w\" to download latest INI file.\n";
        std::wcout << L"If it doesn't help, send your termsrv.dll to project developer for support.\n";
    }
}

void checkTermsrvDependencies() {
    if (serviceStartType(L"CertPropSvc") == SERVICE_DISABLED)
        configureServiceStart(L"CertPropSvc", SERVICE_DEMAND_START);
    if (serviceStartType(L"SessionEnv") == SERVICE_DISABLED)
        configureServiceStart(L"SessionEnv", SERVICE_DEMAND_START);
}

void configureTerminalServicesRegistry(bool enable) {
    if (!writeRegistryDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server",
        L"fDenyTSConnections", enable ? 0 : 1)) halt(ERROR_ACCESS_DENIED);
    if (!enable) return;
    if (!writeRegistryDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Licensing Core",
            L"EnableConcurrentSessions", 1) ||
        !writeRegistryDword(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
            L"AllowMultipleTSSessions", 1)) halt(ERROR_ACCESS_DENIED);

    constexpr wchar_t addins[] = L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns";
    HKEY key = nullptr;
    bool existed = RegOpenKeyExW(HKEY_LOCAL_MACHINE, addins, 0, registryView(KEY_READ), &key) == ERROR_SUCCESS;
    if (key) RegCloseKey(key);
    if (!existed) {
        if (!writeRegistryString(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\Clip Redirector", L"Name", L"RDPClip") ||
            !writeRegistryDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\Clip Redirector", L"Type", 3) ||
            !writeRegistryString(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\DND Redirector", L"Name", L"RDPDND") ||
            !writeRegistryDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\DND Redirector", L"Type", 3) ||
            !writeRegistryDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\Dynamic VC", L"Type", 0xFFFFFFFF))
            halt(ERROR_ACCESS_DENIED);
    }
}

void configureFirewall(bool enable) {
    const std::wstring netsh = L"\"" + expandPath(L"%SystemRoot%\\System32\\netsh.exe") + L"\" ";
    bool ok = false;
    if (enable) {
        DWORD port = readRegistryDword(
            L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp",
            L"PortNumber").value_or(3389);
        if (port < 1 || port > 65535) port = 3389;
        const std::wstring localPort = std::to_wstring(port);
        // Make retries idempotent instead of accumulating duplicate rules.
        execWait(netsh + L"advfirewall firewall delete rule name=\"RDP Wrapper TCP 3389\"", false);
        execWait(netsh + L"advfirewall firewall delete rule name=\"RDP Wrapper UDP 3389\"", false);
        ok = execWait(netsh + L"advfirewall firewall add rule name=\"RDP Wrapper TCP 3389\" dir=in protocol=tcp localport=" + localPort + L" profile=any action=allow") &&
             execWait(netsh + L"advfirewall firewall add rule name=\"RDP Wrapper UDP 3389\" dir=in protocol=udp localport=" + localPort + L" profile=any action=allow");
        if (!ok) {
            execWait(netsh + L"advfirewall firewall delete rule name=\"RDP Wrapper TCP 3389\"", false);
            execWait(netsh + L"advfirewall firewall delete rule name=\"RDP Wrapper UDP 3389\"", false);
        }
    } else {
        const bool tcp = execWait(netsh + L"advfirewall firewall delete rule name=\"RDP Wrapper TCP 3389\"");
        const bool udp = execWait(netsh + L"advfirewall firewall delete rule name=\"RDP Wrapper UDP 3389\"");
        ok = tcp && udp;
    }
    if (!ok) halt(ERROR_GEN_FAILURE);
}

bool iniDate(const std::wstring* filename, const std::string& content, int& date) {
    std::string data = content;
    if (filename) {
        HANDLE file = CreateFileW(filename->c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        LARGE_INTEGER size{};
        if (file == INVALID_HANDLE_VALUE || !GetFileSizeEx(file, &size) ||
            size.QuadPart <= 0 ||
            size.QuadPart > static_cast<LONGLONG>(kMaximumIniBytes)) {
            if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
            std::wcout << L"[-] Failed to read INI file.\n";
            return false;
        }
        data.assign(static_cast<size_t>(size.QuadPart), '\0');
        DWORD read = 0;
        const bool ok = ReadFile(file, data.data(), static_cast<DWORD>(data.size()),
                                 &read, nullptr) &&
                        read == static_cast<DWORD>(data.size());
        CloseHandle(file);
        if (!ok) {
            std::wcout << L"[-] Failed to read INI file.\n";
            return false;
        }
    }
    std::istringstream lines(data);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind("Updated=", 0) == 0) {
            std::string value = line.substr(8);
            value.erase(std::remove(value.begin(), value.end(), '-'), value.end());
            try { date = std::stoi(value); return true; }
            catch (...) {
                std::wcout << L"[-] Wrong INI date format.\n";
                return false;
            }
        }
    }
    std::wcout << L"[-] Failed to check INI date.\n";
    return false;
}

void checkUpdate(const std::wstring& source) {
    auto formattedDate = [](int date) {
        std::wostringstream text;
        text << date / 10000 << L'.' << std::setfill(L'0') << std::setw(2)
             << (date / 100) % 100 << L'.' << std::setw(2) << date % 100;
        return text.str();
    };
    const std::wstring iniPath =
        joinPath(parentPath(expandPath(termServicePath)), configurationFileName());
    int oldDate = 0, newDate = 0;
    if (!iniDate(&iniPath, {}, oldDate)) halt(ERROR_ACCESS_DENIED);
    std::wcout << L"[*] Current update date: " << formattedDate(oldDate) << L"\n";
    std::string content;
    if (!source.empty() && _wcsnicmp(source.c_str(), L"https://", 8) != 0)
        std::wcout << L"[!] The custom INI source is not protected by HTTPS.\n";
    if (!downloadIni(content, source) || !validIniContent(content)) {
        std::wcout << L"[-] Failed to download or validate the latest INI.\n";
        halt(ERROR_ACCESS_DENIED);
    }
    if (!iniDate(nullptr, content, newDate)) halt(ERROR_ACCESS_DENIED);
    std::wcout << L"[*] Latest update date:  " << formattedDate(newDate) << L"\n";
    if (newDate == oldDate) { std::wcout << L"[*] Everything is up to date.\n"; return; }
    if (newDate < oldDate) {
        std::wcout << L"[*] Your INI file is newer than public file. Are you a developer? :)\n";
        return;
    }
    std::wcout << L"[+] New update is available, updating...\n";
    checkTermsrvProcess();
    std::wcout << L"[*] Terminating service...\n";
    addPrivilege(SE_DEBUG_NAME);
    bool umStopped = stopService(L"UmRdpService");
    bool termStopped = stopService(kTermService);
    if (!umStopped) std::wcout << L"[-] Failed to stop UmRdpService.\n";
    if (!termStopped) std::wcout << L"[-] Failed to stop TermService.\n";
    if (umStopped && termStopped)
        std::wcout << L"[+] UmRdpService stopped successfully.\n"
                      L"[+] TermService stopped successfully.\n";
    if (!termStopped) {
        if (umStopped) startService(L"UmRdpService");
        halt(ERROR_SERVICE_REQUEST_TIMEOUT);
    }
    bool servicesOk = true;
    for (const auto& service : sharedServices)
        servicesOk = startService(service) && servicesOk;
    Sleep(500);
    std::wcout << L"[*] Saving new INI file to " << iniPath << L"\n";
    const bool saved = writeBytes(iniPath, content);
    if (saved) std::wcout << L"[+] INI file saved successfully.\n";
    const bool termStarted = startService(kTermService);
    if (!saved) halt(ERROR_ACCESS_DENIED);
    if (!servicesOk || !termStarted) halt(ERROR_SERVICE_REQUEST_TIMEOUT);
    std::wcout << L"[+] Update completed.\n";
}

void restartKnownTermService(bool waitAfterStart) {
    std::wcout << L"[*] Terminating service...\n";
    addPrivilege(SE_DEBUG_NAME);
    killProcess(termServicePid);
    Sleep(1000);
    bool ok = true;
    for (const auto& service : sharedServices)
        ok = startService(service) && ok;
    Sleep(500);
    ok = startService(kTermService) && ok;
    if (waitAfterStart) Sleep(500);
    if (!ok) halt(ERROR_SERVICE_REQUEST_TIMEOUT);
}

void restartTermService() {
    checkTermsrvProcess();
    restartKnownTermService(false);
}

void usage() {
    std::wcout << L"USAGE:\nRDPWInst.exe [-l|-i[-s][-o]|-w[url]|-u[-k]|-r]\n\n"
        L"-l          display the license agreement\n"
        L"-i          install wrapper to Program Files folder (default)\n"
        L"-i -s       install wrapper to System32 folder\n"
        L"-i -o       online install mode (loads latest INI file)\n"
        L"-w          get latest update for INI file\n"
        L"-w URL      get latest update for INI file from custom source\n"
        L"-u          uninstall wrapper and disable Remote Desktop\n"
        L"-u -k       uninstall wrapper and keep Remote Desktop/firewall settings\n"
        L"-r          force restart Terminal Services\n";
}

bool validCommandArguments(const std::wstring& command, int argc, wchar_t* argv[]) {
    if (command == L"-l" || command == L"-r") return argc == 2;
    if (command == L"-u")
        return argc == 2 || (argc == 3 && std::wstring(argv[2]) == L"-k");
    if (command == L"-w") return argc == 2 || (argc == 3 && *argv[2] != L'\0');
    if (command != L"-i" || argc > 4) return false;
    bool system32 = false;
    bool onlineMode = false;
    for (int index = 2; index < argc; ++index) {
        const std::wstring option = argv[index];
        if (option == L"-s" && !system32) system32 = true;
        else if (option == L"-o" && !onlineMode) onlineMode = true;
        else return false;
    }
    return true;
}

void printBanner() {
    std::wcout << L"RDP Wrapper Library v" RDPWRAP_VERSION_W L"\n"
                  L"Copyright (C) Stas'M Corp. 2017\n"
                  L"Edited by bobo 2026\n\n";
}

bool isProcessElevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    TOKEN_ELEVATION elevation{};
    DWORD bytes = 0;
    const BOOL ok = GetTokenInformation(token, TokenElevation, &elevation,
        sizeof(elevation), &bytes);
    CloseHandle(token);
    return ok && elevation.TokenIsElevated != 0;
}

class WidePipeBuffer final : public std::wstreambuf {
public:
    explicit WidePipeBuffer(HANDLE pipe) : pipe_(pipe) {}

protected:
    int_type overflow(int_type value) override {
        if (traits_type::eq_int_type(value, traits_type::eof()))
            return traits_type::not_eof(value);
        const wchar_t character = traits_type::to_char_type(value);
        return write(&character, 1) == 1 ? value : traits_type::eof();
    }

    std::streamsize xsputn(const wchar_t* text, std::streamsize count) override {
        return write(text, count);
    }

    int sync() override { return FlushFileBuffers(pipe_) ? 0 : -1; }

private:
    std::streamsize write(const wchar_t* text, std::streamsize count) {
        std::streamsize completed = 0;
        while (completed < count) {
            const DWORD characters = static_cast<DWORD>(std::min<std::streamsize>(
                count - completed, MAXDWORD / sizeof(wchar_t)));
            DWORD bytes = 0;
            if (!WriteFile(pipe_, text + completed, characters * sizeof(wchar_t),
                    &bytes, nullptr) || bytes % sizeof(wchar_t) != 0)
                break;
            completed += bytes / sizeof(wchar_t);
            if (!bytes) break;
        }
        return completed;
    }

    HANDLE pipe_;
};

bool redirectOutputToPipe(const std::wstring& pipeName) {
    if (pipeName.empty()) return false;
    HANDLE pipe = CreateFileW(pipeName.c_str(), GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return false;

    // Deliberately keep this buffer alive until process teardown. All normal
    // installer output uses the two wide streams below.
    auto* buffer = new WidePipeBuffer(pipe);
    std::wcout.rdbuf(buffer);
    std::wcerr.rdbuf(buffer);
    std::wcout.clear();
    std::wcerr.clear();
    return true;
}

bool connectOutputPipe(HANDLE pipe, HANDLE childProcess) {
    OVERLAPPED connection{};
    connection.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!connection.hEvent) return false;

    bool connected = ConnectNamedPipe(pipe, &connection) != FALSE;
    if (!connected) {
        const DWORD error = GetLastError();
        if (error == ERROR_PIPE_CONNECTED) {
            connected = true;
        } else if (error == ERROR_IO_PENDING) {
            HANDLE handles[] = {connection.hEvent, childProcess};
            const DWORD wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0) {
                DWORD transferred = 0;
                connected = GetOverlappedResult(pipe, &connection, &transferred, FALSE) != FALSE;
            } else {
                CancelIoEx(pipe, &connection);
                WaitForSingleObject(connection.hEvent, INFINITE);
            }
        }
    }
    CloseHandle(connection.hEvent);
    return connected;
}

bool readOutputPipe(HANDLE pipe, BYTE* buffer, DWORD capacity, DWORD& count) {
    count = 0;
    OVERLAPPED operation{};
    operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!operation.hEvent) return false;
    BOOL ok = ReadFile(pipe, buffer, capacity, &count, &operation);
    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        if (WaitForSingleObject(operation.hEvent, INFINITE) == WAIT_OBJECT_0)
            ok = GetOverlappedResult(pipe, &operation, &count, FALSE);
    }
    CloseHandle(operation.hEvent);
    return ok != FALSE;
}

void forwardPipeToOriginalOutput(HANDLE pipe, HANDLE childProcess) {
    if (!connectOutputPipe(pipe, childProcess)) return;

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode = 0;
    const bool isConsole = output != nullptr && output != INVALID_HANDLE_VALUE &&
        GetConsoleMode(output, &consoleMode) != FALSE;
    std::vector<BYTE> pending;
    pending.reserve(8192);
    BYTE bytes[8192];
    DWORD count = 0;
    while (readOutputPipe(pipe, bytes, sizeof(bytes), count) && count) {
        pending.insert(pending.end(), bytes, bytes + count);
        const size_t characters = pending.size() / sizeof(wchar_t);
        if (!characters) continue;
        std::vector<wchar_t> text(characters);
        std::memcpy(text.data(), pending.data(), characters * sizeof(wchar_t));
        if (isConsole) {
            size_t offset = 0;
            while (offset < characters) {
                DWORD written = 0;
                if (!WriteConsoleW(output, text.data() + offset,
                        static_cast<DWORD>(characters - offset), &written, nullptr) || !written)
                    break;
                offset += written;
            }
        } else if (output != nullptr && output != INVALID_HANDLE_VALUE) {
            const int required = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                static_cast<int>(characters), nullptr, 0, nullptr, nullptr);
            std::string utf8(static_cast<size_t>(std::max(required, 0)), '\0');
            if (required > 0) {
                WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(characters),
                    utf8.data(), required, nullptr, nullptr);
                size_t offset = 0;
                while (offset < utf8.size()) {
                    DWORD written = 0;
                    if (!WriteFile(output, utf8.data() + offset,
                            static_cast<DWORD>(utf8.size() - offset), &written, nullptr) || !written)
                        break;
                    offset += written;
                }
            }
        }
        pending.erase(pending.begin(), pending.begin() + characters * sizeof(wchar_t));
    }
}

// Quote one argv element according to the CommandLineToArgvW parsing rules.
std::wstring quoteArgument(const std::wstring& argument) {
    if (!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring::npos)
        return argument;

    std::wstring quoted = L"\"";
    size_t backslashes = 0;
    for (wchar_t ch : argument) {
        if (ch == L'\\') {
            ++backslashes;
        } else if (ch == L'\"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(L'\"');
            backslashes = 0;
        } else {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
            quoted.push_back(ch);
        }
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'\"');
    return quoted;
}

int relaunchElevated(int argc, wchar_t* argv[]) {
    GUID pipeId{};
    wchar_t pipeIdText[40]{};
    const std::wstring pipeSuffix = SUCCEEDED(CoCreateGuid(&pipeId)) &&
        StringFromGUID2(pipeId, pipeIdText, 40) > 0
        ? std::wstring(pipeIdText) : std::to_wstring(GetTickCount64());
    const std::wstring pipeName = L"\\\\.\\pipe\\RDPWInst-" +
        std::to_wstring(GetCurrentProcessId()) + L"-" + pipeSuffix;
    HANDLE outputPipe = CreateNamedPipeW(pipeName.c_str(),
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, 65536, 65536, 0, nullptr);

    std::wstring parameters;
    for (int index = 1; index < argc; ++index) {
        if (!parameters.empty()) parameters.push_back(L' ');
        parameters += quoteArgument(argv[index]);
    }
    if (!parameters.empty()) parameters.push_back(L' ');
    parameters += kConsolePidArgument;
    parameters += std::to_wstring(GetCurrentProcessId());
    if (outputPipe != INVALID_HANDLE_VALUE) {
        parameters.push_back(L' ');
        parameters += quoteArgument(std::wstring(kOutputPipeArgument) + pipeName);
    }

    const std::wstring path = executablePath();
    SHELLEXECUTEINFOW execute{};
    execute.cbSize = sizeof(execute);
    // SEE_MASK_NO_CONSOLE makes the elevated console application inherit this
    // process's console instead of opening a second console window.
    execute.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    execute.lpVerb = L"runas";
    execute.lpFile = path.c_str();
    execute.lpParameters = parameters.c_str();
    // The UAC broker can briefly allocate a temporary console before the
    // elevated process gets a chance to AttachConsole. Keep that temporary
    // window hidden; reconnectToConsole will bind output to the caller's CMD.
    execute.nShow = SW_HIDE;

    std::wcout.flush();
    std::wcerr.flush();
    const HRESULT comResult = CoInitializeEx(nullptr,
        COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shellOk = ShellExecuteExW(&execute) != FALSE;
    const DWORD shellError = shellOk ? ERROR_SUCCESS : GetLastError();
    if (SUCCEEDED(comResult)) CoUninitialize();
    if (!shellOk) {
        if (outputPipe != INVALID_HANDLE_VALUE) CloseHandle(outputPipe);
        return static_cast<int>(shellError);
    }
    if (!execute.hProcess) {
        if (outputPipe != INVALID_HANDLE_VALUE) CloseHandle(outputPipe);
        return ERROR_INVALID_HANDLE;
    }

    if (outputPipe != INVALID_HANDLE_VALUE) {
        forwardPipeToOriginalOutput(outputPipe, execute.hProcess);
        CloseHandle(outputPipe);
    }

    const DWORD waitResult = WaitForSingleObject(execute.hProcess, INFINITE);
    DWORD exitCode = ERROR_GEN_FAILURE;
    if (waitResult != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(execute.hProcess, &exitCode))
        exitCode = GetLastError();
    CloseHandle(execute.hProcess);
    return static_cast<int>(exitCode);
}

void reconnectToConsole(DWORD processId) {
    if (!processId) return;

    DWORD processIds[64]{};
    const DWORD count = GetConsoleProcessList(processIds,
        static_cast<DWORD>(std::size(processIds)));
    bool attachedToRequestedConsole = false;
    for (DWORD index = 0; index < std::min<DWORD>(count,
             static_cast<DWORD>(std::size(processIds))); ++index) {
        if (processIds[index] == processId) {
            attachedToRequestedConsole = true;
            break;
        }
    }

    // The UAC broker can ignore console inheritance for a runas launch. Drop
    // that temporary console and explicitly attach to the still-running
    // unelevated launcher, which remains blocked in WaitForSingleObject.
    if (!attachedToRequestedConsole) {
        FreeConsole();
        if (!AttachConsole(processId)) return;
    }

    // Always rebind: even when SEE_MASK_NO_CONSOLE attached the correct console
    // object, the UAC broker may not carry usable standard handles across the
    // integrity boundary (especially when the launch uses SW_HIDE).
    FILE* stream = nullptr;
#ifdef _MSC_VER
    const errno_t inputResult = freopen_s(&stream, "CONIN$", "r", stdin);
    const errno_t outputResult = freopen_s(&stream, "CONOUT$", "w", stdout);
    const errno_t errorResult = freopen_s(&stream, "CONOUT$", "w", stderr);
    (void)inputResult;
    (void)outputResult;
    (void)errorResult;
#else
    stream = freopen("CONIN$", "r", stdin);
    stream = freopen("CONOUT$", "w", stdout);
    stream = freopen("CONOUT$", "w", stderr);
    (void)stream;
#endif
    std::wcin.clear();
    std::wcout.clear();
    std::wcerr.clear();
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    try {
        std::vector<wchar_t*> effectiveArguments;
        effectiveArguments.reserve(static_cast<size_t>(argc));
        DWORD consoleProcessId = 0;
        std::wstring outputPipeName;
        const size_t consoleMarkerLength = std::wcslen(kConsolePidArgument);
        const size_t pipeMarkerLength = std::wcslen(kOutputPipeArgument);
        for (int index = 0; index < argc; ++index) {
            if (index > 0 && std::wcsncmp(argv[index], kConsolePidArgument,
                    consoleMarkerLength) == 0) {
                wchar_t* end = nullptr;
                const unsigned long value = std::wcstoul(
                    argv[index] + consoleMarkerLength, &end, 10);
                if (end && *end == L'\0') consoleProcessId = static_cast<DWORD>(value);
            } else if (index > 0 && std::wcsncmp(argv[index], kOutputPipeArgument,
                           pipeMarkerLength) == 0) {
                outputPipeName = argv[index] + pipeMarkerLength;
            } else {
                effectiveArguments.push_back(argv[index]);
            }
        }
        if (!redirectOutputToPipe(outputPipeName))
            reconnectToConsole(consoleProcessId);
        argc = static_cast<int>(effectiveArguments.size());
        argv = effectiveArguments.data();

        // Keep the manifest asInvoker so Windows can preserve the caller's
        // console, then elevate every invocation through the same runas path.
        if (!isProcessElevated()) return relaunchElevated(argc, argv);
        if (argc < 2) { printBanner(); usage(); return 0; }
        std::wstring command = argv[1];
        if (command != L"-l" && command != L"-i" && command != L"-w" &&
            command != L"-u" && command != L"-r") {
            printBanner(); usage(); return 0;
        }
        if (!validCommandArguments(command, argc, argv)) {
            printBanner();
            std::wcout << L"[-] Invalid command-line arguments.\n\n";
            usage();
            return ERROR_INVALID_PARAMETER;
        }
        if (command == L"-l") {
            printBanner(); std::wcout << resourceText(L"license"); return 0;
        }
        printBanner();
        if (!IsWindowsVistaOrGreater()) {
            std::wcout << L"[-] Unsupported Windows version:\n"
                          L"  only >= 6.0 (Vista, Server 2008 and newer) are supported.\n";
            return 0;
        }
        if (!supportedArchitecture()) {
            std::wcout << L"[-] Unsupported processor architecture.\n"; return 0;
        }
        checkInstall();

        if (command == L"-i") {
            if (installed) { std::wcout << L"[*] RDP Wrapper Library is already installed.\n"; halt(ERROR_INVALID_FUNCTION); }
            std::wcout << L"[*] Notice to user:\n"
                          L"  - By using all or any portion of this software, you are agreeing\n"
                          L"  to be bound by all the terms and conditions of the license agreement.\n"
                          L"  - To read the license agreement, run the installer with -l parameter.\n"
                          L"  - If you do not agree to any terms of the license agreement,\n"
                          L"  do not use the software.\n";
            std::wcout << L"[*] Installing...\n";
            bool system32 = false;
            for (int index = 2; index < argc; ++index)
                if (std::wstring(argv[index]) == L"-s") system32 = true;
            wrapPath = system32 ? L"%SystemRoot%\\system32\\rdpwrap.dll"
                                : L"%ProgramFiles%\\RDP Wrapper\\rdpwrap.dll";
            online = (argc > 2 && std::wstring(argv[2]) == L"-o") ||
                     (argc > 3 && std::wstring(argv[3]) == L"-o");
            onlineIniContent.clear();
            if (online) {
                std::wcout << L"[*] Downloading latest INI file...\n";
                if (!downloadIni(onlineIniContent, L"") ||
                    !validIniContent(onlineIniContent)) {
                    std::wcout << L"[-] Failed to get online INI file, using built-in.\n";
                    online = false;
                }
            }
            checkTermsrvVersion();
            checkTermsrvProcess();
            std::wcout << L"[*] Extracting files...\n";
            extractFiles();
            std::wcout << L"[*] Configuring service library...\n";
            const std::wstring originalServiceDll = termServicePath;
            setWrapperDll();
            try {
                std::wcout << L"[*] Checking dependencies...\n";
                checkTermsrvDependencies();
                // CheckTermsrvProcess above captured the PID and shared-service
                // list. Preserve the established sequence instead of rediscovering it.
                restartKnownTermService(true);
                std::wcout << L"[*] Configuring registry...\n";
                configureTerminalServicesRegistry(true);
                std::wcout << L"[*] Configuring firewall...\n";
                configureFirewall(true);
            } catch (...) {
                const std::exception_ptr failure = std::current_exception();
                std::wcout << L"[!] Installation failed; restoring the original ServiceDll...\n";
                if (!writeRegistryString(
                        L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters",
                        L"ServiceDll", originalServiceDll, REG_EXPAND_SZ)) {
                    std::wcout << L"[-] Rollback could not restore ServiceDll.\n";
                } else {
                    try {
                        checkTermsrvProcess();
                        restartKnownTermService(false);
                        std::wcout << L"[+] ServiceDll rollback completed.\n";
                    } catch (...) {
                        std::wcout << L"[-] ServiceDll was restored, but Terminal Services could not be restarted.\n";
                    }
                }
                std::rethrow_exception(failure);
            }
            std::wcout << L"[*] Deploying automatic updater...\n";
            if (!deployUpdater()) {
                std::wcout << L"[!] Wrapper installed, but automatic updater deployment failed.\n";
                halt(ERROR_ACCESS_DENIED);
            }
            std::wcout << L"[+] Successfully installed.\n";
        } else if (command == L"-u") {
            if (!installed) { std::wcout << L"[*] RDP Wrapper Library is not installed.\n"; halt(ERROR_INVALID_FUNCTION); }
            std::wcout << L"[*] Uninstalling...\n";
            checkTermsrvProcess();
            std::wcout << L"[*] Resetting service library...\n";
            resetServiceDll();
            std::wcout << L"[*] Terminating service...\n";
            addPrivilege(SE_DEBUG_NAME);
            killProcess(termServicePid);
            Sleep(1000);
            std::wcout << L"[*] Removing files...\n";
            const bool filesRemoved = deleteFiles();
            bool servicesRestarted = true;
            for (const auto& service : sharedServices)
                servicesRestarted = startService(service) && servicesRestarted;
            Sleep(500);
            servicesRestarted = startService(kTermService) && servicesRestarted;
            Sleep(500);
            if (!(argc > 2 && std::wstring(argv[2]) == L"-k")) {
                std::wcout << L"[*] Configuring registry...\n";
                configureTerminalServicesRegistry(false);
                std::wcout << L"[*] Configuring firewall...\n";
                configureFirewall(false);
            }
            if (!filesRemoved) halt(ERROR_ACCESS_DENIED);
            if (!servicesRestarted) halt(ERROR_SERVICE_REQUEST_TIMEOUT);
            std::wcout << L"[*] Removing automatic updater...\n";
            removeUpdater();
            std::wcout << L"[+] Successfully uninstalled.\n";
        } else if (command == L"-w") {
            if (!installed) { std::wcout << L"[*] RDP Wrapper Library is not installed.\n"; halt(ERROR_INVALID_FUNCTION); }
            std::wcout << L"[*] Checking for updates...\n";
            checkUpdate(argc > 2 ? argv[2] : L"");
        } else if (command == L"-r") {
            std::wcout << L"[*] Restarting...\n";
            restartTermService();
            std::wcout << L"[+] Done.\n";
        }
        return 0;
    } catch (const ExitCode& exit) {
        std::wcerr << L"[-] Operation failed (code " << exit.value << L").\n";
        return static_cast<int>(exit.value);
    } catch (const std::exception& error) {
        const std::string message = error.what();
        std::wcerr << L"[-] " << std::wstring(message.begin(), message.end()) << L'\n';
        return ERROR_GEN_FAILURE;
    }
}
