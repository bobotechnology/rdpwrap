// Native Win32 configuration utility for RDP Wrapper.

#include <windows.h>
#include <winsvc.h>
#include <wtsapi32.h>
#include <shellapi.h>
#include <winver.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwctype>

// Control IDs
enum : int {
    IDC_ALLOW=100, IDC_SINGLE, IDC_CUSTOM, IDC_HIDE, IDC_PORT, IDC_NLA,
    IDC_SHADOW, IDC_APPLY, IDC_OK, IDC_CANCEL, IDC_LICENSE, IDC_UPDATE, IDC_RESTART,
    IDC_MSTSC, IDC_800, IDC_1024, IDC_1366, IDC_1920, IDC_WRAPPER, IDC_SERVICE,
    IDC_LISTENER, IDC_WRAPVER, IDC_TSVERSION, IDC_SUPPORT, TIMER_STATUS=1,
    IDC_TAB_CONTROL=200, IDC_TAB_BASIC, IDC_TAB_ADVANCED, IDC_TAB_DIAGNOSTICS,
    IDC_STATUS_WRAPPER, IDC_STATUS_SERVICE, IDC_STATUS_LISTENER,
    IDC_PORT_VALIDATION, IDC_HELP_PORT, IDC_HELP_AUTH, IDC_HELP_SHADOW,
    IDC_MSTSC_MODE
};

// Settings structure
struct Settings {
    bool allow=false, single=false, custom=false, hide=false;
    int port=3389, nla=0, shadow=0;
};

// Global variables
static HWND g_hwnd;
static Settings g_settings, g_saved;
static bool g_ready=false; // Set true after UI initialization completes
static HFONT g_uiFont=nullptr, g_headingFont=nullptr;
static HBRUSH g_backgroundBrush=nullptr;
static double g_uiScale=1.0;
static std::vector<HWND> g_headingControls;
static bool g_chinese=false;

static UINT windowDpi(HWND window) {
    using GetDpiForWindowFunction = UINT(WINAPI*)(HWND);
    const HMODULE user32 = GetModuleHandleW(L"user32.dll");
    const auto getDpiForWindow = user32
        ? reinterpret_cast<GetDpiForWindowFunction>(
              GetProcAddress(user32, "GetDpiForWindow"))
        : nullptr;
    if (getDpiForWindow) return getDpiForWindow(window);

    HDC device = GetDC(window);
    if (!device) return 96;
    const int dpi = GetDeviceCaps(device, LOGPIXELSX);
    ReleaseDC(window, device);
    return dpi > 0 ? static_cast<UINT>(dpi) : 96;
}

// Use Simplified Chinese on Chinese Windows. The command-line switches
// /lang=zh-CN and /lang=en-US can be used to override the system language.
static const wchar_t* tr(const wchar_t* english, const wchar_t* chinese) {
    return g_chinese ? chinese : english;
}

static void selectLanguage(const wchar_t* commandLine) {
    std::wstring args = commandLine ? commandLine : L"";
    std::transform(args.begin(), args.end(), args.begin(), towlower);
    if (args.find(L"/lang=zh") != std::wstring::npos || args.find(L"--lang=zh") != std::wstring::npos) {
        g_chinese = true;
    } else if (args.find(L"/lang=en") != std::wstring::npos || args.find(L"--lang=en") != std::wstring::npos) {
        g_chinese = false;
    } else {
        g_chinese = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE;
    }
}

// Modern UI subclass for better appearance
static LRESULT CALLBACK modernSubclass(HWND hwnd, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (message == WM_ERASEBKGND) {
        RECT r{};
        GetClientRect(hwnd, &r);
        FillRect((HDC)wp, &r, g_backgroundBrush);
        return 1;
    }
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLORBTN) {
        HWND child = (HWND)lp;
        SetBkMode((HDC)wp, TRANSPARENT);
        SetBkColor((HDC)wp, RGB(248, 249, 251));
        if (message == WM_CTLCOLORSTATIC) {
            HANDLE color = GetPropW(child, L"RDPColor");
            if (color) SetTextColor((HDC)wp, (COLORREF)(ULONG_PTR)color);
        }
        return (LRESULT)g_backgroundBrush;
    }
    return DefSubclassProc(hwnd, message, wp, lp);
}

// Modernize UI with better styling
static void modernizeUi() {
    static bool done = false;
    if (done) return;
    done = true;

    UINT dpi = windowDpi(g_hwnd);
    g_uiScale = (double)dpi / 96.0;
    RECT original{};
    GetWindowRect(g_hwnd, &original);
    if (dpi != 96) {
        EnumChildWindows(g_hwnd, [](HWND child, LPARAM) -> BOOL {
            RECT r{};
            GetWindowRect(child, &r);
            MapWindowPoints(nullptr, g_hwnd, reinterpret_cast<POINT*>(&r), 2);
            MoveWindow(child, (int)(r.left * g_uiScale), (int)(r.top * g_uiScale),
                       (int)((r.right - r.left) * g_uiScale),
                       (int)((r.bottom - r.top) * g_uiScale), FALSE);
            return TRUE;
        }, 0);
        SetWindowPos(g_hwnd, nullptr, 0, 0,
                     (int)((original.right - original.left) * g_uiScale),
                     (int)((original.bottom - original.top) * g_uiScale),
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Create fonts
    LOGFONTW normal{};
    normal.lfHeight = -MulDiv(9, dpi, 72);
    wcscpy_s(normal.lfFaceName, L"Segoe UI");
    normal.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    g_uiFont = CreateFontIndirectW(&normal);

    normal.lfHeight = -MulDiv(11, dpi, 72);
    g_headingFont = CreateFontIndirectW(&normal);

    g_backgroundBrush = CreateSolidBrush(RGB(248, 249, 251));
    SetClassLongPtrW(g_hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)g_backgroundBrush);
    SetWindowSubclass(g_hwnd, modernSubclass, 1, 0);

    // Apply styling to all controls
    EnumChildWindows(g_hwnd, [](HWND child, LPARAM) -> BOOL {
        wchar_t cls[32]{};
        GetClassNameW(child, cls, 32);
        SetWindowTheme(child, L"Explorer", nullptr);

        if (_wcsicmp(cls, L"Button") == 0 && ((GetWindowLongPtrW(child, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX)) {
            // Hide group boxes, we'll use custom styling
            ShowWindow(child, SW_HIDE);
        } else {
            SendMessageW(child, WM_SETFONT, (WPARAM)g_uiFont, TRUE);
        }
        return TRUE;
    }, 0);
    for (HWND heading : g_headingControls)
        SendMessageW(heading, WM_SETFONT, (WPARAM)g_headingFont, TRUE);
}

// Expand environment variables
static std::wstring expandEnv(const std::wstring& value) {
    DWORD n = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
    if (!n) return value;
    std::wstring out(n, L'\0');
    DWORD written = ExpandEnvironmentStringsW(value.c_str(), out.data(), n);
    if (!written || written > n) return value;
    out.resize(written - 1);
    return out;
}

// Registry string read
static std::wstring regString(HKEY root, const wchar_t* path, const wchar_t* name) {
    HKEY key{};
    if (RegOpenKeyExW(root, path, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) return {};
    DWORD type = 0, bytes = 0;
    std::wstring out;
    if (RegQueryValueExW(key, name, nullptr, &type, nullptr, &bytes) == ERROR_SUCCESS &&
        bytes && (type == REG_SZ || type == REG_EXPAND_SZ)) {
        out.resize(bytes / sizeof(wchar_t) + 1, L'\0');
        DWORD capacity = bytes;
        if (RegQueryValueExW(key, name, nullptr, &type, (LPBYTE)out.data(), &capacity) == ERROR_SUCCESS)
            out.resize(wcsnlen(out.c_str(), out.size()));
        else
            out.clear();
    }
    RegCloseKey(key);
    return out;
}

// Registry DWORD read
static bool regDword(HKEY root, const wchar_t* path, const wchar_t* name, DWORD& value) {
    HKEY k{};
    DWORD t = 0, s = sizeof(value);
    bool ok = RegOpenKeyExW(root, path, 0, KEY_READ | KEY_WOW64_64KEY, &k) == ERROR_SUCCESS &&
              RegQueryValueExW(k, name, nullptr, &t, (LPBYTE)&value, &s) == ERROR_SUCCESS &&
              t == REG_DWORD && s == sizeof(value);
    if (k) RegCloseKey(k);
    return ok;
}

// Registry DWORD write
static bool setDword(const wchar_t* path, const wchar_t* name, DWORD v) {
    if (wcscmp(name, L"PortNumber") == 0) {
        v = std::clamp<DWORD>(v, 1, 65535);
        g_settings.port = (int)v;
    } else if (wcscmp(name, L"Shadow") == 0) {
        v = std::clamp<DWORD>(v, 0, 4);
        g_settings.shadow = (int)v;
    }

    HKEY k{};
    LSTATUS e = RegCreateKeyExW(HKEY_LOCAL_MACHINE, path, 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &k, nullptr);
    if (e == ERROR_SUCCESS) {
        e = RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE*)&v, sizeof(v));
        RegCloseKey(k);
    }
    if (e != ERROR_SUCCESS) {
        wchar_t message[256];
        swprintf_s(message, tr(L"Cannot write registry value %s (error %ld).",
                              L"无法写入注册表值 %s（错误 %ld）。"), name, e);
        MessageBoxW(g_hwnd, message, tr(L"Registry error", L"注册表错误"), MB_OK | MB_ICONERROR);
    }
    return e == ERROR_SUCCESS;
}

// Wait for service to reach expected state
static bool waitService(SC_HANDLE service, DWORD expected, DWORD timeoutMs) {
    DWORD start = GetTickCount();
    SERVICE_STATUS_PROCESS state{};
    DWORD needed = 0;
    do {
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE*)&state, sizeof(state), &needed))
            return false;
        if (state.dwCurrentState == expected) return true;
        Sleep(200);
    } while (GetTickCount() - start < timeoutMs);
    return false;
}

// Restart Terminal Service
static bool restartTermService() {
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!manager) return false;

    auto stop = [&](const wchar_t* name) {
        SC_HANDLE s = OpenServiceW(manager, name, SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!s) return GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST;
        SERVICE_STATUS status{};
        BOOL ok = ControlService(s, SERVICE_CONTROL_STOP, &status);
        DWORD error = ok ? ERROR_SUCCESS : GetLastError();
        bool result = (error == ERROR_SERVICE_NOT_ACTIVE) || ((error == ERROR_SUCCESS) && waitService(s, SERVICE_STOPPED, 30000));
        CloseServiceHandle(s);
        return result;
    };

    bool ok = stop(L"UmRdpService") && stop(L"TermService");
    SC_HANDLE term = OpenServiceW(manager, L"TermService", SERVICE_START | SERVICE_QUERY_STATUS);
    if (term) {
        BOOL started = StartServiceW(term, 0, nullptr);
        DWORD error = started ? ERROR_SUCCESS : GetLastError();
        ok = ok && (error == ERROR_SUCCESS || error == ERROR_SERVICE_ALREADY_RUNNING) && waitService(term, SERVICE_RUNNING, 30000);
        CloseServiceHandle(term);
    } else {
        ok = false;
    }
    CloseServiceHandle(manager);

    if (!ok) MessageBoxW(g_hwnd,
                         tr(L"Could not restart TermService through Service Control Manager.",
                            L"无法通过服务控制管理器重启 TermService。"),
                         tr(L"Service error", L"服务错误"), MB_OK | MB_ICONERROR);
    return ok;
}

// Run process with hidden window
static bool runProcess(const std::wstring& cmd) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(0);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        MessageBoxW(g_hwnd, tr(L"CreateProcess failed.", L"创建进程失败。"),
                    tr(L"Error", L"错误"), MB_ICONERROR);
        return false;
    }

    CloseHandle(pi.hThread);
    const DWORD wait = WaitForSingleObject(pi.hProcess, 30000);
    if (wait != WAIT_OBJECT_0) {
        if (wait == WAIT_TIMEOUT) {
            TerminateProcess(pi.hProcess, ERROR_TIMEOUT);
            WaitForSingleObject(pi.hProcess, 5000);
            MessageBoxW(g_hwnd, tr(L"The command timed out after 30 seconds.", L"命令执行超过 30 秒，已超时。"),
                        tr(L"Command timeout", L"命令超时"), MB_OK | MB_ICONERROR);
        } else {
            MessageBoxW(g_hwnd, tr(L"Could not wait for the command to finish.", L"无法等待命令执行完成。"),
                        tr(L"Command error", L"命令错误"), MB_OK | MB_ICONERROR);
        }
        CloseHandle(pi.hProcess);
        return false;
    }
    DWORD code = 1;
    if (!GetExitCodeProcess(pi.hProcess, &code)) code = GetLastError();
    CloseHandle(pi.hProcess);

    if (code != 0) MessageBoxW(g_hwnd,
                               tr(L"The command returned a non-zero exit code.", L"命令返回了非零退出代码。"),
                               tr(L"Command error", L"命令错误"), MB_OK | MB_ICONERROR);
    return code == 0;
}

// Execute command with special handling
static bool execWait(const std::wstring& cmd) {
    if (cmd.rfind(L"taskkill /F /T /FI \"SERVICES eq ", 0) == 0) return true;
    if (cmd == L"net start TermService") return restartTermService();

    return runProcess(cmd);
}

static bool updateWrapperFirewallPort(int port) {
    if (port < 1 || port > 65535) return false;

    const std::wstring netsh = L"\"" + expandEnv(L"%SystemRoot%\\System32\\netsh.exe") +
                               L"\" advfirewall firewall set rule name=";
    const std::wstring value = std::to_wstring(port);
    const bool tcp = runProcess(netsh + L"\"RDP Wrapper TCP 3389\" protocol=TCP new localport=" + value);
    const bool udp = runProcess(netsh + L"\"RDP Wrapper UDP 3389\" protocol=UDP new localport=" + value);
    return tcp && udp;
}

// Get file version
static std::wstring versionOf(const std::wstring& file, bool productVersion = false) {
    DWORD dummy = 0, n = GetFileVersionInfoSizeW(file.c_str(), &dummy);
    if (!n) return L"N/A";

    std::vector<BYTE> b(n);
    if (!GetFileVersionInfoW(file.c_str(), 0, n, b.data())) return L"N/A";

    VS_FIXEDFILEINFO* f = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(b.data(), L"\\", (void**)&f, &len) || !f || len < sizeof(VS_FIXEDFILEINFO) || f->dwSignature != VS_FFI_SIGNATURE)
        return L"N/A";

    DWORD ms = productVersion ? f->dwProductVersionMS : f->dwFileVersionMS;
    DWORD ls = productVersion ? f->dwProductVersionLS : f->dwFileVersionLS;
    wchar_t s[64];
    swprintf_s(s, L"%u.%u.%u.%u", HIWORD(ms), LOWORD(ms), HIWORD(ls), LOWORD(ls));
    return s;
}

// Get service state
static int serviceState() {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return -1;

    SC_HANDLE svc = OpenServiceW(scm, L"TermService", SERVICE_QUERY_STATUS);
    if (!svc) {
        CloseServiceHandle(scm);
        return -1;
    }

    SERVICE_STATUS_PROCESS s{};
    DWORD n = 0;
    bool ok = QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&s, sizeof(s), &n);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok ? (int)s.dwCurrentState : -1;
}

// Check if RDP listener is active
static bool listener() {
    WTS_SESSION_INFOW* info = nullptr;
    DWORD count = 0;
    bool found = false;

    if (WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, &count)) {
        for (DWORD i = 0; i < count; i++) {
            if (info[i].pWinStationName && info[i].State == WTSListen && _wcsicmp(info[i].pWinStationName, L"RDP-Tcp") == 0) {
                found = true;
                break;
            }
        }
        WTSFreeMemory(info);
    }
    return found;
}

// Check RDP Wrapper status
static int wrapper(std::wstring& path) {
    path.clear();
    std::wstring host = regString(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\TermService", L"ImagePath");
    if (host.empty()) return -1;

    std::wstring hostLow = host;
    std::transform(hostLow.begin(), hostLow.end(), hostLow.begin(), towlower);
    if (hostLow.find(L"svchost.exe") == std::wstring::npos) return 2;

    std::wstring dll = regString(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters", L"ServiceDll");
    if (dll.empty()) return -1;

    dll = expandEnv(dll);
    std::wstring low = dll;
    std::transform(low.begin(), low.end(), low.begin(), towlower);

    if (low.find(L"termsrv.dll") == std::wstring::npos && low.find(L"rdpwrap.dll") == std::wstring::npos) return 2;
    if (low.find(L"rdpwrap.dll") != std::wstring::npos) {
        path = dll;
        return 1;
    }
    return 0;
}

// Resolve the RDP Wrapper installation directory from the ServiceDll registry value.
// Falls back to "C:\Program Files\RDP Wrapper" if the registry entry is unavailable.
static std::wstring wrapperDir() {
    std::wstring dll = regString(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters", L"ServiceDll");
    if (!dll.empty()) {
        dll = expandEnv(dll);
        size_t slash = dll.find_last_of(L"\\/");
        if (slash != std::wstring::npos) return dll.substr(0, slash);
    }
    return L"C:\\Program Files\\RDP Wrapper";
}

// Read settings from registry
static void readSettings() {
    DWORD v;
    g_settings = Settings{};

    if (regDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", L"fDenyTSConnections", v))
        g_settings.allow = v == 0;
    if (regDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", L"fSingleSessionPerUser", v))
        g_settings.single = v != 0;
    if (regDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", L"HonorLegacySettings", v))
        g_settings.custom = v != 0;
    if (regDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"PortNumber", v))
        g_settings.port = (int)v;
    if (regDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"SecurityLayer", v)) {
        DWORD u = 0;
        regDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"UserAuthentication", u);
        g_settings.nla = (v == 2 && u == 1) ? 2 : (v == 1 ? 1 : 0);
    }
    if (regDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"Shadow", v))
        g_settings.shadow = (int)v;
    if (regDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", L"dontdisplaylastusername", v))
        g_settings.hide = v != 0;

    g_saved = g_settings;
}

// Write settings to registry
static bool writeSettings() {
    bool registryOk = true;
    registryOk = setDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", L"fDenyTSConnections", !g_settings.allow) && registryOk;
    registryOk = setDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", L"fSingleSessionPerUser", g_settings.single) && registryOk;
    registryOk = setDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", L"HonorLegacySettings", g_settings.custom) && registryOk;
    registryOk = setDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"PortNumber", g_settings.port) && registryOk;

    DWORD sec = g_settings.nla == 2 ? 2 : g_settings.nla;
    DWORD auth = g_settings.nla == 2 ? 1 : 0;
    registryOk = setDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"SecurityLayer", sec) && registryOk;
    registryOk = setDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"UserAuthentication", auth) && registryOk;
    registryOk = setDword(L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", L"Shadow", g_settings.shadow) && registryOk;
    registryOk = setDword(L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Terminal Services", L"Shadow", g_settings.shadow) && registryOk;
    registryOk = setDword(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", L"dontdisplaylastusername", g_settings.hide) && registryOk;

    if (!registryOk) return false;

    if (g_saved.port != g_settings.port && !updateWrapperFirewallPort(g_settings.port)) {
        MessageBoxW(g_hwnd,
                    tr(L"The RDP port was saved, but one or more RDP Wrapper firewall rules could not be updated.",
                       L"RDP 端口已保存，但一个或多个 RDP Wrapper 防火墙规则更新失败。"),
                    tr(L"Firewall warning", L"防火墙警告"), MB_OK | MB_ICONWARNING);
    }

    g_saved = g_settings;
    EnableWindow(GetDlgItem(g_hwnd, IDC_APPLY), FALSE);
    return true;
}

// Helper to add controls
static HWND add(const wchar_t* cls, const wchar_t* text, DWORD style, int id, int x, int y, int w, int h) {
    HWND child = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                               (int)(x * g_uiScale), (int)(y * g_uiScale),
                               (int)(w * g_uiScale), (int)(h * g_uiScale),
                               g_hwnd, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    if (g_uiFont) SendMessageW(child, WM_SETFONT, (WPARAM)g_uiFont, TRUE);
    return child;
}

static HWND addHeading(const wchar_t* text, int x, int y, int w) {
    HWND heading = add(L"STATIC", text, SS_LEFT, 0, x, y, w, 24);
    g_headingControls.push_back(heading);
    return heading;
}

// Check support level
static int supportLevel(const std::wstring& wrapperPath, const std::wstring& tsVersion) {
    if (wrapperPath.empty()) return -1;

    std::wstring ini = wrapperPath;
    size_t slash = ini.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return 0;

    ini.resize(slash + 1);
    ini += L"rdpwrap.ini";

    std::wifstream f(ini.c_str());
    if (!f) return 0;

    std::wstring line;
    while (std::getline(f, line)) {
        // Trim whitespace for robust matching
        size_t first = line.find_first_not_of(L" \t\r\n");
        size_t last = line.find_last_not_of(L" \t\r\n");
        if (first == std::wstring::npos) continue;
        std::wstring trimmed = line.substr(first, last - first + 1);
        // Exact match against "[version]" section header
        if (trimmed.length() >= 2 && trimmed.front() == L'[' && trimmed.back() == L']') {
            std::wstring section = trimmed.substr(1, trimmed.length() - 2);
            if (section == tsVersion) return 2;
        }
    }

    if (tsVersion.rfind(L"6.0.", 0) == 0 || tsVersion.rfind(L"6.1.", 0) == 0) return 1;
    return 0;
}

// Set status color
static void setStatusColor(int id, COLORREF color) {
    modernizeUi();
    HWND c = GetDlgItem(g_hwnd, id);
    SetPropW(c, L"RDPColor", (HANDLE)(ULONG_PTR)color);
    InvalidateRect(c, nullptr, TRUE);
}

// Update status display
static void status() {
    std::wstring p;
    int w = wrapper(p);

    // Update status indicators with better styling
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_WRAPPER), w < 0 ? tr(L"Unknown", L"未知") :
                   w == 0 ? tr(L"Not installed", L"未安装") :
                   w == 1 ? tr(L"Installed", L"已安装") : tr(L"3rd-party", L"第三方组件"));
    setStatusColor(IDC_WRAPPER, w == 1 ? RGB(0, 150, 0) : (w == 2 ? RGB(190, 0, 0) : RGB(100, 100, 100)));

    int s = serviceState();
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_SERVICE), s == SERVICE_RUNNING ? tr(L"Running", L"正在运行") :
                  s == SERVICE_STOPPED ? tr(L"Stopped", L"已停止") :
                  s == SERVICE_START_PENDING ? tr(L"Starting...", L"正在启动…") :
                  s == SERVICE_STOP_PENDING ? tr(L"Stopping...", L"正在停止…") :
                  s < 0 ? tr(L"Unknown", L"未知") : tr(L"Pending", L"等待中"));
    setStatusColor(IDC_SERVICE, s == SERVICE_RUNNING ? RGB(0, 150, 0) : (s == SERVICE_STOPPED ? RGB(190, 0, 0) : RGB(100, 100, 100)));

    bool listening = listener();
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_LISTENER), listening ? tr(L"Listening", L"正在监听") : tr(L"Not listening", L"未监听"));
    setStatusColor(IDC_LISTENER, listening ? RGB(0, 150, 0) : RGB(190, 0, 0));

    std::wstring wrapVer = p.empty() ? L"N/A" : versionOf(p);
    std::wstring tsVer = versionOf(expandEnv(L"%SystemRoot%\\System32\\termsrv.dll"), true);

    if (g_chinese && wrapVer == L"N/A") wrapVer = L"无法获取";
    if (g_chinese && tsVer == L"N/A") tsVer = L"无法获取";

    SetWindowTextW(GetDlgItem(g_hwnd, IDC_WRAPVER), wrapVer.c_str());
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_TSVERSION), tsVer.c_str());

    int level = supportLevel(p, tsVer);
    HWND label = GetDlgItem(g_hwnd, IDC_SUPPORT);
    if (!label) label = add(L"STATIC", L"", SS_CENTER, IDC_SUPPORT, 20, 163, 240, 18);

    SetWindowTextW(label, level < 0 ? tr(L"Unknown", L"未知") :
                   level == 2 ? tr(L"Fully supported", L"完全支持") :
                   level == 1 ? tr(L"Partial support", L"部分支持") : tr(L"Not supported", L"不支持"));
    setStatusColor(IDC_SUPPORT, level == 2 ? RGB(0, 150, 0) : (level == 1 ? RGB(140, 110, 0) : RGB(190, 0, 0)));
}

// Launch MSTSC with arguments
static void launch(const wchar_t* args) {
    HINSTANCE result = ShellExecuteW(g_hwnd, L"open", L"mstsc.exe", args, nullptr, SW_SHOW);
    if ((INT_PTR)result <= 32)
        MessageBoxW(g_hwnd, tr(L"Could not start mstsc.exe.", L"无法启动 mstsc.exe。"),
                    tr(L"Launch error", L"启动错误"), MB_OK | MB_ICONERROR);
}

// Validate port input
static bool validatePort(int port) {
    return port >= 1 && port <= 65535;
}

// Show validation feedback
static void showValidationFeedback(bool isValid, const wchar_t* message) {
    HWND feedback = GetDlgItem(g_hwnd, IDC_PORT_VALIDATION);
    if (!feedback) return;

    SetWindowTextW(feedback, message);
    setStatusColor(IDC_PORT_VALIDATION, isValid ? RGB(0, 150, 0) : RGB(190, 0, 0));
    ShowWindow(feedback, SW_SHOW);
}

// Window procedure
static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_CTLCOLORSTATIC) {
        HWND c = (HWND)lp;
        HANDLE p = GetPropW(c, L"RDPColor");
        if (p) {
            SetTextColor((HDC)wp, (COLORREF)(ULONG_PTR)p);
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
    }

    if (m == WM_COMMAND) {
        int id = LOWORD(wp);

        // Apply/OK button
        if (id == IDC_APPLY || id == IDC_OK) {
            g_settings.allow = IsDlgButtonChecked(h, IDC_ALLOW) == BST_CHECKED;
            g_settings.single = IsDlgButtonChecked(h, IDC_SINGLE) == BST_CHECKED;
            g_settings.custom = IsDlgButtonChecked(h, IDC_CUSTOM) == BST_CHECKED;
            g_settings.hide = IsDlgButtonChecked(h, IDC_HIDE) == BST_CHECKED;

            wchar_t b[16];
            GetWindowTextW(GetDlgItem(h, IDC_PORT), b, 16);
            int port = _wtoi(b);

            if (!validatePort(port)) {
                showValidationFeedback(false, tr(L"Port must be between 1 and 65535", L"端口必须介于 1 和 65535 之间"));
                return 0;
            }

            showValidationFeedback(true, tr(L"Port is valid", L"端口有效"));
            g_settings.port = std::clamp(port, 1, 65535);
            g_settings.nla = (int)SendMessageW(GetDlgItem(h, IDC_NLA), CB_GETCURSEL, 0, 0);
            g_settings.shadow = (int)SendMessageW(GetDlgItem(h, IDC_SHADOW), CB_GETCURSEL, 0, 0);

            if (writeSettings() && id == IDC_OK) DestroyWindow(h);
        }
        // Cancel button
        else if (id == IDC_CANCEL) {
            SendMessageW(h, WM_CLOSE, 0, 0);
        }
        // License button
        else if (id == IDC_LICENSE) {
            MessageBoxW(h,
                        tr(L"RDP_CnC\n\nCopyright 2017-2026 Stas'M Corp., sebaxakerhtc and bobo.\nLicensed under the Apache License, Version 2.0.\n\nYou may obtain a copy of the License at\nhttp://www.apache.org/licenses/LICENSE-2.0",
                           L"RDP_CnC\n\n版权所有 2017-2026 Stas'M Corp.、sebaxakerhtc 和 bobo。\n基于 Apache License 2.0 版授权。\n\n许可证全文：\nhttp://www.apache.org/licenses/LICENSE-2.0"),
                        tr(L"License", L"许可协议"), MB_OK | MB_ICONINFORMATION);
        }
        // MSTSC buttons
        else if (id == IDC_MSTSC) {
            int mode = (int)SendMessageW(GetDlgItem(h, IDC_MSTSC_MODE), CB_GETCURSEL, 0, 0);
            const wchar_t* args[] = {
                L"/v:127.0.0.2 /f /prompt", L"/v:127.0.0.2 /w:800 /h:600 /prompt",
                L"/v:127.0.0.2 /w:1024 /h:768 /prompt", L"/v:127.0.0.2 /w:1366 /h:768 /prompt",
                L"/v:127.0.0.2 /w:1920 /h:1080 /prompt"};
            launch(args[std::clamp(mode, 0, 4)]);
        }
        else if (id == IDC_800) launch(L"/v:127.0.0.2 /w:800 /h:600 /prompt");
        else if (id == IDC_1024) launch(L"/v:127.0.0.2 /w:1024 /h:768 /prompt");
        else if (id == IDC_1366) launch(L"/v:127.0.0.2 /w:1366 /h:768 /prompt");
        else if (id == IDC_1920) launch(L"/v:127.0.0.2 /w:1920 /h:1080 /prompt");
        // Update button
        else if (id == IDC_UPDATE) {
            std::wstring inst = wrapperDir();
            inst += L"\\RDPWInst.exe";
            if (GetFileAttributesW(inst.c_str()) == INVALID_FILE_ATTRIBUTES)
                MessageBoxW(h, tr(L"RDPWInst.exe not found", L"未找到 RDPWInst.exe"),
                            tr(L"Error", L"错误"), MB_ICONERROR);
            else
                execWait(L"\"" + inst + L"\" -w");
            status();
        }
        // Restart button
        else if (id == IDC_RESTART) {
            if (MessageBoxW(h, tr(L"Are you sure you want to restart Terminal Server?", L"确定要重启远程桌面服务吗？"),
                            tr(L"Warning", L"警告"), MB_YESNO | MB_ICONWARNING) == IDYES) {
                execWait(L"taskkill /F /T /FI \"SERVICES eq UmTermService\"");
                execWait(L"net start TermService");
                status();
            }
        }
        // Port input validation
        else if (id == IDC_PORT && HIWORD(wp) == EN_CHANGE) {
            wchar_t b[16];
            GetWindowTextW(GetDlgItem(h, IDC_PORT), b, 16);
            int port = _wtoi(b);

            if (port == 0 && wcslen(b) > 0) {
                showValidationFeedback(false, tr(L"Port must be a number", L"端口必须是数字"));
            } else if (!validatePort(port)) {
                showValidationFeedback(false, tr(L"Port must be between 1 and 65535", L"端口必须介于 1 和 65535 之间"));
            } else if (port == 3389) {
                showValidationFeedback(true, tr(L"Using default RDP port", L"当前使用默认 RDP 端口"));
            } else {
                showValidationFeedback(true, tr(L"Port is valid", L"端口有效"));
            }
        }
        // Help buttons
        else if (id == IDC_HELP_PORT) {
            MessageBoxW(h,
                        tr(L"RDP Port: The TCP port used for Remote Desktop connections.\n\nDefault: 3389\nRange: 1-65535\n\nChanging the port may require firewall configuration.",
                           L"RDP 端口：远程桌面连接使用的 TCP 端口。\n\n默认值：3389\n范围：1-65535\n\n更改端口后可能需要配置防火墙。"),
                        tr(L"Port Help", L"端口帮助"), MB_OK | MB_ICONINFORMATION);
        }
        else if (id == IDC_HELP_AUTH) {
            MessageBoxW(h,
                        tr(L"Authentication Mode:\n\n- GUI Authentication: Simple but less secure\n- Default RDP Authentication: Balanced security and ease of use\n- Network Level Authentication: Most secure, requires client support",
                           L"连接安全模式：\n\n- 传统 RDP 模式：使用传统 RDP 加密，兼容性较好但安全性较低\n- 自动协商模式：自动选择客户端支持的最安全方式\n- 网络级别身份验证 (NLA)：建立会话前验证用户身份，安全性最高，建议使用"),
                        tr(L"Authentication Help", L"连接安全模式帮助"), MB_OK | MB_ICONINFORMATION);
        }
        else if (id == IDC_HELP_SHADOW) {
            MessageBoxW(h,
                        tr(L"Remote Desktop Shadowing:\n\n- No shadowing: Users cannot be shadowed\n- Full control: Administrators can take control of sessions\n- View only: Administrators can only view sessions",
                           L"远程控制远程桌面服务用户会话：\n\n- 不允许远程控制\n- 经用户授权或不经用户授权完全控制\n- 经用户授权或不经用户授权查看会话"),
                        tr(L"Shadowing Help", L"远程控制帮助"), MB_OK | MB_ICONINFORMATION);
        }
        if (g_ready && (id == IDC_ALLOW || id == IDC_SINGLE || id == IDC_CUSTOM ||
                        id == IDC_HIDE || id == IDC_PORT || id == IDC_NLA || id == IDC_SHADOW))
            EnableWindow(GetDlgItem(h, IDC_APPLY), TRUE);
    }
    else if (m == WM_TIMER && wp == TIMER_STATUS) {
        status();
        return 0;
    }
    else if (m == WM_CLOSE) {
        if (IsWindowEnabled(GetDlgItem(h, IDC_APPLY)) &&
            MessageBoxW(h, tr(L"Settings are not saved. Do you want to exit?", L"设置尚未保存，确定要退出吗？"),
                        tr(L"Warning", L"警告"),
                        MB_YESNO | MB_ICONWARNING) != IDYES)
            return 0;
        DestroyWindow(h);
        return 0;
    }
    // Child controls are initialized after CreateWindowW returns.
    else if (m == WM_CREATE) {
        return 0;
    }
    // Window destruction — release GDI objects and subclass
    else if (m == WM_DESTROY) {
        // Remove window properties (RDPColor) from all status controls
        int statusIds[] = {IDC_WRAPPER, IDC_SERVICE, IDC_LISTENER, IDC_SUPPORT, IDC_PORT_VALIDATION};
        for (int sid : statusIds) {
            HWND ctrl = GetDlgItem(g_hwnd, sid);
            if (ctrl) RemovePropW(ctrl, L"RDPColor");
        }
        // Remove the subclass before deleting the brush it references
        RemoveWindowSubclass(g_hwnd, modernSubclass, 1);
        if (g_uiFont) { DeleteObject(g_uiFont); g_uiFont = nullptr; }
        if (g_headingFont) { DeleteObject(g_headingFont); g_headingFont = nullptr; }
        if (g_backgroundBrush) { DeleteObject(g_backgroundBrush); g_backgroundBrush = nullptr; }
        PostQuitMessage(0);
    }

    return DefWindowProcW(h, m, wp, lp);
}

// Main entry point
#define wWinMain rdpMain
int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR commandLine, int n) {
    selectLanguage(commandLine);

    INITCOMMONCONTROLSEX ic{sizeof(ic), ICC_TAB_CLASSES};
    if (!InitCommonControlsEx(&ic)) return 11;

    WNDCLASSW wc{};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"RDP_CnC";
    RegisterClassW(&wc);

    g_hwnd = CreateWindowW(wc.lpszClassName,
                          tr(L"Remote Desktop Protocol Configuration", L"RDP Wrapper 配置工具"),
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                          0, 0, 570, 480, nullptr, nullptr, hi, nullptr);
    if (!g_hwnd) return 12;

    addHeading(tr(L"System status", L"系统状态"), 16, 10, 180);
    add(L"STATIC", tr(L"Wrapper", L"RDP Wrapper"), 0, 0, 16, 39, 115, 20);
    add(L"STATIC", tr(L"Service", L"远程桌面服务"), 0, 0, 154, 39, 115, 20);
    add(L"STATIC", tr(L"Listener", L"RDP 监听器"), 0, 0, 292, 39, 115, 20);
    add(L"STATIC", tr(L"Support", L"版本支持"), 0, 0, 430, 39, 120, 20);
    add(L"STATIC", tr(L"Unknown", L"未知"), 0, IDC_WRAPPER, 16, 62, 115, 20);
    add(L"STATIC", tr(L"Unknown", L"未知"), 0, IDC_SERVICE, 154, 62, 115, 20);
    add(L"STATIC", tr(L"Unknown", L"未知"), 0, IDC_LISTENER, 292, 62, 115, 20);
    add(L"STATIC", tr(L"Unknown", L"未知"), 0, IDC_SUPPORT, 430, 62, 120, 20);
    add(L"STATIC", tr(L"Wrapper version", L"RDP Wrapper 版本："), 0, 0, 16, 88, 130, 20);
    add(L"STATIC", tr(L"N/A", L"无法获取"), 0, IDC_WRAPVER, 154, 88, 115, 20);
    add(L"STATIC", tr(L"termsrv version", L"termsrv.dll 版本："), 0, 0, 292, 88, 130, 20);
    add(L"STATIC", tr(L"N/A", L"无法获取"), 0, IDC_TSVERSION, 430, 88, 120, 20);
    add(L"STATIC", L"", SS_ETCHEDHORZ, 0, 16, 112, 538, 1);

    addHeading(tr(L"Remote Desktop", L"远程桌面设置"), 16, 116, 220);
    add(L"STATIC", tr(L"RDP port", L"RDP 端口"), 0, 0, 16, 148, 80, 22);
    add(L"EDIT", L"3389", WS_BORDER | ES_NUMBER, IDC_PORT, 100, 147, 140, 22);
    add(L"BUTTON", L"?", BS_PUSHBUTTON, IDC_HELP_PORT, 248, 147, 22, 22);
    add(L"STATIC", L"", 0, IDC_PORT_VALIDATION, 16, 176, 250, 20);
    add(L"BUTTON", tr(L"Enable Remote Desktop", L"启用远程桌面"), BS_AUTOCHECKBOX, IDC_ALLOW, 16, 202, 250, 26);
    add(L"BUTTON", tr(L"Single session per user", L"将用户限制到单独的会话"), BS_AUTOCHECKBOX, IDC_SINGLE, 16, 232, 250, 26);
    add(L"BUTTON", tr(L"Hide users on logon screen", L"不显示上次登录的用户名"), BS_AUTOCHECKBOX, IDC_HIDE, 16, 262, 260, 26);

    addHeading(tr(L"Session policy", L"会话设置"), 295, 116, 220);
    add(L"STATIC", tr(L"Authentication mode", L"连接安全模式"), 0, 0, 295, 148, 170, 22);
    add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_BORDER, IDC_NLA, 295, 172, 220, 110);
    add(L"BUTTON", L"?", BS_PUSHBUTTON, IDC_HELP_AUTH, 526, 172, 22, 22);
    add(L"STATIC", tr(L"Shadow mode", L"远程控制"), 0, 0, 295, 210, 150, 22);
    add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_BORDER, IDC_SHADOW, 295, 234, 220, 150);
    add(L"BUTTON", L"?", BS_PUSHBUTTON, IDC_HELP_SHADOW, 526, 234, 22, 22);
    add(L"BUTTON", tr(L"Allow unlisted RemoteApps", L"允许启动未发布的 RemoteApp"), BS_AUTOCHECKBOX, IDC_CUSTOM, 295, 270, 250, 26);

    add(L"STATIC", L"", SS_ETCHEDHORZ, 0, 16, 302, 538, 1);
    addHeading(tr(L"Maintenance", L"维护"), 16, 313, 190);
    add(L"BUTTON", tr(L"Update INI", L"更新 INI 配置"), BS_PUSHBUTTON, IDC_UPDATE, 16, 345, 110, 30);
    add(L"BUTTON", tr(L"Restart service", L"重启服务"), BS_PUSHBUTTON, IDC_RESTART, 136, 345, 130, 30);
    add(L"STATIC", tr(L"Local connection test", L"本地连接测试"), 0, 0, 295, 318, 170, 22);
    HWND mstscMode = add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_BORDER,
                         IDC_MSTSC_MODE, 295, 345, 150, 150);
    add(L"BUTTON", tr(L"Connect", L"连接"), BS_PUSHBUTTON, IDC_MSTSC, 463, 345, 90, 22);
    for (const wchar_t* mode : {tr(L"Fullscreen", L"全屏"), L"800 x 600", L"1024 x 768",
                                L"1366 x 768", L"1920 x 1080"})
        SendMessageW(mstscMode, CB_ADDSTRING, 0, (LPARAM)mode);
    SendMessageW(mstscMode, CB_SETCURSEL, 0, 0);

    add(L"STATIC", L"", SS_ETCHEDHORZ, 0, 16, 390, 538, 1);
    add(L"BUTTON", tr(L"License", L"许可协议"), BS_PUSHBUTTON, IDC_LICENSE, 16, 407, 90, 30);
    add(L"BUTTON", tr(L"Apply", L"应用"), BS_DEFPUSHBUTTON, IDC_APPLY, 336, 407, 100, 30);
    add(L"BUTTON", tr(L"Close", L"关闭"), BS_PUSHBUTTON, IDC_CANCEL, 446, 407, 108, 30);

    // Initialize controls
    SendMessageW(GetDlgItem(g_hwnd, IDC_NLA), CB_ADDSTRING, 0, (LPARAM)tr(L"GUI Authentication Only", L"传统 RDP 模式"));
    SendMessageW(GetDlgItem(g_hwnd, IDC_NLA), CB_ADDSTRING, 0, (LPARAM)tr(L"Default RDP Authentication", L"自动协商模式"));
    SendMessageW(GetDlgItem(g_hwnd, IDC_NLA), CB_ADDSTRING, 0, (LPARAM)tr(L"Network Level Authentication", L"网络级别身份验证 (NLA)"));
    SendMessageW(GetDlgItem(g_hwnd, IDC_NLA), CB_SETCURSEL, 1, 0);

    SendMessageW(GetDlgItem(g_hwnd, IDC_SHADOW), CB_ADDSTRING, 0, (LPARAM)tr(L"Disable Shadowing", L"不允许远程控制"));
    SendMessageW(GetDlgItem(g_hwnd, IDC_SHADOW), CB_ADDSTRING, 0, (LPARAM)tr(L"Full access with user's permission", L"经用户授权完全控制"));
    SendMessageW(GetDlgItem(g_hwnd, IDC_SHADOW), CB_ADDSTRING, 0, (LPARAM)tr(L"Full access without permission", L"不经用户授权完全控制"));
    SendMessageW(GetDlgItem(g_hwnd, IDC_SHADOW), CB_ADDSTRING, 0, (LPARAM)tr(L"View only with user's permission", L"经用户授权查看会话"));
    SendMessageW(GetDlgItem(g_hwnd, IDC_SHADOW), CB_ADDSTRING, 0, (LPARAM)tr(L"View only without permission", L"不经用户授权查看会话"));

    readSettings();
    CheckDlgButton(g_hwnd, IDC_ALLOW, g_settings.allow ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_SINGLE, g_settings.single ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_CUSTOM, g_settings.custom ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_HIDE, g_settings.hide ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(g_hwnd, IDC_PORT, std::clamp(g_settings.port, 1, 65535), FALSE);
    SendMessageW(GetDlgItem(g_hwnd, IDC_NLA), CB_SETCURSEL, std::clamp(g_settings.nla, 0, 2), 0);
    SendMessageW(GetDlgItem(g_hwnd, IDC_SHADOW), CB_SETCURSEL, std::clamp(g_settings.shadow, 0, 4), 0);

    modernizeUi();
    status();
    EnableWindow(GetDlgItem(g_hwnd, IDC_APPLY), FALSE);
    g_ready = true;
    SetTimer(g_hwnd, TIMER_STATUS, 1000, nullptr);

    // Show window
    ShowWindow(g_hwnd, n);
    UpdateWindow(g_hwnd);

    // Message loop
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
