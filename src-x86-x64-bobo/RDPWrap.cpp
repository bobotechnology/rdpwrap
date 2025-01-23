#include "stdafx.h"
#include "INIParser.hpp"
using namespace std;

#ifdef _WIN64
typedef unsigned long long PLATFORM_DWORD;
#pragma pack(push, 1)
struct FARJMP
{                   // x64 far jump | opcode | assembly
    BYTE MovOp;     // 48	mov rax, ptr
    BYTE MovRegArg; // B8
    DWORD64 MovArg; // PTR
    BYTE PushRaxOp; // 50	push rax
    BYTE RetOp;     // C3	retn
}; // 1+1+8+1+1=12
#pragma pack(pop)
/*
mov rax, ptr //48 b8 ptr
push rax;//50
retn;//c3
*/
#else
typedef unsigned long PLATFORM_DWORD;
#pragma pack(push, 1)
struct FARJMP
{                  // x86 far jump | opcode | assembly
    BYTE PushOp;   // 68	push ptr
    DWORD PushArg; // PTR
    BYTE RetOp;    // C3	retn
};
#pragma pack(pop)
/*
push ptr;//68 ptr
retn;//c3
*/
#endif

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[16];
    WORD Padding1;
    VS_FIXEDFILEINFO Value;
    WORD Padding2;
    WORD Children;
} VS_VERSIONINFO, *PVS_VERSIONINFO;

FARJMP Old_SLGetWindowsInformationDWORD, Stub_SLGetWindowsInformationDWORD;
SLGETWINDOWSINFORMATIONDWORD _SLGetWindowsInformationDWORD;

SERVICEMAIN _ServiceMain;
SVCHOSTPUSHSERVICEGLOBALS _SvchostPushServiceGlobals;
bool AlreadyHooked = false;
wchar_t LogFile[MAX_PATH] = L"\\rdpwrap.txt\0";
HMODULE hTermSrv;
HMODULE hSLC;
VS_VERSIONINFO *hResData;
size_t TermSrvBase;

INIParser parser;

void WriteToLog(LPSTR Text)
{
    DWORD dwBytesOfWritten;

    HANDLE hFile = CreateFile(LogFile, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    SetFilePointer(hFile, 0, 0, FILE_END);
    size_t len = strlen(Text);
    if (len > MAXDWORD)
    {
        return;
    }
    WriteFile(hFile, Text, (DWORD)len, &dwBytesOfWritten, NULL);
    CloseHandle(hFile);
}

HMODULE GetCurrentModule()
{
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)GetCurrentModule, &hModule);
    return hModule;
}

void SetThreadsState(bool Resume)
{
    auto CurrTh = GetCurrentThreadId();
    auto CurrPr = GetCurrentProcessId();
    THREADENTRY32 Thread;

    auto h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        Thread.dwSize = sizeof(THREADENTRY32);
        Thread32First(h, &Thread);
        do
        {
            if (Thread.th32ThreadID != CurrTh && Thread.th32OwnerProcessID == CurrPr)
            {
                auto hThread = OpenThread(THREAD_SUSPEND_RESUME, false, Thread.th32ThreadID);
                if (hThread)
                {
                    if (Resume)
                        ResumeThread(hThread);
                    else
                        SuspendThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(h, &Thread));
        CloseHandle(h);
    }
}

string ws2s(const wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

bool OverrideSL(LPWSTR ValueName, DWORD *Value)
{
    string ValueNameStr = ws2s(ValueName);
    *Value = parser.getDWordHex("SLPolicy", ValueNameStr.c_str());
    if (*Value == 0)
    {
        return false;
    }
    return true;
}

HRESULT WINAPI New_SLGetWindowsInformationDWORD(PWSTR pwszValueName, DWORD *pdwValue)
{
    // wrapped SLGetWindowsInformationDWORD function
    // termsrv.dll will call this function instead of original SLC.dll

    // Override SL Policy

    extern FARJMP Old_SLGetWindowsInformationDWORD, Stub_SLGetWindowsInformationDWORD;
    extern SLGETWINDOWSINFORMATIONDWORD _SLGetWindowsInformationDWORD;

    char *Log;
    DWORD dw;
    SIZE_T bw;
    HRESULT Result;

    Log = new char[1024];
    wsprintfA(Log, "Policy query: %S\r\n", pwszValueName);
    WriteToLog(Log);
    delete[] Log;

    if (OverrideSL(pwszValueName, &dw))
    {
        *pdwValue = dw;

        Log = new char[1024];
        wsprintfA(Log, "Policy rewrite: %i\r\n", dw);
        WriteToLog(Log);
        delete[] Log;

        return S_OK;
    }

    WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD, &Old_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
    Result = _SLGetWindowsInformationDWORD(pwszValueName, pdwValue);
    if (Result == S_OK)
    {
        Log = new char[1024];
        wsprintfA(Log, "Policy result: %i\r\n", dw);
        WriteToLog(Log);
        delete[] Log;
    }
    else
    {
        WriteToLog("Policy request failed\r\n");
    }
    WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD, &Stub_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);

    return Result;
}
HRESULT __fastcall New_Win8SL(PWSTR pwszValueName, DWORD *pdwValue)
{
    // wrapped unexported function SLGetWindowsInformationDWORDWrapper in termsrv.dll
    // for Windows 8 support

    // Override SL Policy

    extern SLGETWINDOWSINFORMATIONDWORD _SLGetWindowsInformationDWORD;

    char *Log;
    DWORD dw;
    HRESULT Result;

    Log = new char[1024];
    wsprintfA(Log, "Policy query: %S\r\n", pwszValueName);
    WriteToLog(Log);
    delete[] Log;

    if (OverrideSL(pwszValueName, &dw))
    {
        *pdwValue = dw;

        Log = new char[1024];
        wsprintfA(Log, "Policy rewrite: %i\r\n", dw);
        WriteToLog(Log);
        delete[] Log;

        return S_OK;
    }

    Result = _SLGetWindowsInformationDWORD(pwszValueName, pdwValue);
    if (Result == S_OK)
    {
        Log = new char[1024];
        wsprintfA(Log, "Policy result: %i\r\n", dw);
        WriteToLog(Log);
        delete[] Log;
    }
    else
    {
        WriteToLog("Policy request failed\r\n");
    }

    return Result;
}

#ifndef _WIN64
HRESULT __fastcall New_Win8SL_CP(DWORD arg1, DWORD *pdwValue, PWSTR pwszValueName, DWORD arg4)
{
    // wrapped unexported function SLGetWindowsInformationDWORDWrapper in termsrv.dll
    // for Windows 8 Consumer Preview support

    return New_Win8SL(pwszValueName, pdwValue);
}
#endif

HRESULT WINAPI New_CSLQuery_Initialize()
{

    char *Log;
    DWORD *bServerSku = NULL;
    DWORD *bRemoteConnAllowed = NULL;
    DWORD *bFUSEnabled = NULL;
    DWORD *bAppServerAllowed = NULL;
    DWORD *bMultimonAllowed = NULL;
    DWORD *lMaxUserSessions = NULL;
    DWORD *ulMaxDebugSessions = NULL;
    DWORD *bInitialized = NULL;

    WriteToLog(">>> CSLQuery::Initialize\r\n");

    ostringstream oss;
    oss << HIWORD(hResData->Value.dwFileVersionMS) << "." << LOWORD(hResData->Value.dwFileVersionMS) << "." << HIWORD(hResData->Value.dwFileVersionLS) << "." << LOWORD(hResData->Value.dwFileVersionLS) << "-SLInit";

    string Sect = oss.str();

    if (parser.validateSection(Sect))
    {
#ifdef _WIN64
        bServerSku = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bServerSku.x64", 0));
        if (!bServerSku)
        {
            WriteToLog("Failed to get bServerSku\r\n");
            return E_FAIL;
        }
        bRemoteConnAllowed = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bRemoteConnAllowed.x64", 0));
        if (!bRemoteConnAllowed)
        {
            WriteToLog("Failed to get bRemoteConnAllowed\r\n");
            return E_FAIL;
        }
        bFUSEnabled = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bFUSEnabled.x64", 0));
        if (!bFUSEnabled)
        {
            WriteToLog("Failed to get bFUSEnabled\r\n");
            return E_FAIL;
        }
        bAppServerAllowed = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bAppServerAllowed.x64", 0));
        if (!bAppServerAllowed)
        {
            WriteToLog("Failed to get bAppServerAllowed\r\n");
            return E_FAIL;
        }
        bMultimonAllowed = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bMultimonAllowed.x64", 0));
        if (!bMultimonAllowed)
        {
            WriteToLog("Failed to get bMultimonAllowed\r\n");
            return E_FAIL;
        }
        lMaxUserSessions = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "lMaxUserSessions.x64", 0));
        if (!lMaxUserSessions)
        {
            WriteToLog("Failed to get lMaxUserSessions\r\n");
            return E_FAIL;
        }
        ulMaxDebugSessions = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "ulMaxDebugSessions.x64", 0));
        if (!ulMaxDebugSessions)
        {
            WriteToLog("Failed to get ulMaxDebugSessions\r\n");
            return E_FAIL;
        }
        bInitialized = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bInitialized.x64", 0));
        if (!bInitialized)
        {
            WriteToLog("Failed to get bInitialized\r\n");
            return E_FAIL;
        }
#else
        bServerSku = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bServerSku.x86", 0));
        if (!bServerSku)
        {
            WriteToLog("Failed to get bServerSku\r\n");
            return E_FAIL;
        }
        bRemoteConnAllowed = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bRemoteConnAllowed.x86", 0));
        if (!bRemoteConnAllowed)
        {
            WriteToLog("Failed to get bRemoteConnAllowed\r\n");
            return E_FAIL;
        }
        bFUSEnabled = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bFUSEnabled.x86", 0));
        if (!bFUSEnabled)
        {
            WriteToLog("Failed to get bFUSEnabled\r\n");
            return E_FAIL;
        }
        bAppServerAllowed = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bAppServerAllowed.x86", 0));
        if (!bAppServerAllowed)
        {
            WriteToLog("Failed to get bAppServerAllowed\r\n");
            return E_FAIL;
        }
        bMultimonAllowed = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bMultimonAllowed.x86", 0));
        if (!bMultimonAllowed)
        {
            WriteToLog("Failed to get bMultimonAllowed\r\n");
            return E_FAIL;
        }
        lMaxUserSessions = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "lMaxUserSessions.x86", 0));
        if (!lMaxUserSessions)
        {
            WriteToLog("Failed to get lMaxUserSessions\r\n");
            return E_FAIL;
        }
        ulMaxDebugSessions = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "ulMaxDebugSessions.x86", 0));
        if (!ulMaxDebugSessions)
        {
            WriteToLog("Failed to get ulMaxDebugSessions\r\n");
            return E_FAIL;
        }
        bInitialized = (DWORD *)(TermSrvBase + parser.getDWordHex(Sect, "bInitialized.x86", 0));
        if (!bInitialized)
        {
            WriteToLog("Failed to get bInitialized\r\n");
            return E_FAIL;
        }
#endif
    }

    if (bServerSku)
    {
        *bServerSku = parser.getDWordHex("SLInit", "bServerSku", 1);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] bServerSku = %d\r\n", bServerSku, *bServerSku);
        WriteToLog(Log);
        delete[] Log;
    }
    if (bRemoteConnAllowed)
    {
        *bRemoteConnAllowed = parser.getDWordHex("SLInit", "bRemoteConnAllowed", 1);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] bRemoteConnAllowed = %d\r\n", bRemoteConnAllowed, *bRemoteConnAllowed);
        WriteToLog(Log);
        delete[] Log;
    }
    if (bFUSEnabled)
    {
        *bFUSEnabled = parser.getDWordHex("SLInit", "bFUSEnabled", 1);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] bFUSEnabled = %d\r\n", bFUSEnabled, *bFUSEnabled);
        WriteToLog(Log);
        delete[] Log;
    }
    if (bAppServerAllowed)
    {
        *bAppServerAllowed = parser.getDWordHex("SLInit", "bAppServerAllowed", 1);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] bAppServerAllowed = %d\r\n", bAppServerAllowed, *bAppServerAllowed);
        WriteToLog(Log);
        delete[] Log;
    }
    if (bMultimonAllowed)
    {
        *bMultimonAllowed = parser.getDWordHex("SLInit", "bMultimonAllowed", 1);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] bMultimonAllowed = %d\r\n", bMultimonAllowed, *bMultimonAllowed);
        WriteToLog(Log);
        delete[] Log;
    }
    if (lMaxUserSessions)
    {
        *lMaxUserSessions = parser.getDWordHex("SLInit", "lMaxUserSessions", 0);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] lMaxUserSessions = %d\r\n", lMaxUserSessions, *lMaxUserSessions);
        WriteToLog(Log);
        delete[] Log;
    }
    if (ulMaxDebugSessions)
    {
        *ulMaxDebugSessions = parser.getDWordHex("SLInit", "ulMaxDebugSessions", 0);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] ulMaxDebugSessions = %d\r\n", ulMaxDebugSessions, *ulMaxDebugSessions);
        WriteToLog(Log);
        delete[] Log;
    }
    if (bInitialized)
    {
        *bInitialized = parser.getDWordHex("SLInit", "bInitialized", 1);

        Log = new char[1024];
        wsprintfA(Log, "SLInit [0x%p] bInitialized = %d\r\n", bInitialized, *bInitialized);
        WriteToLog(Log);
        delete[] Log;
    }
    WriteToLog("<<< CSLQuery::Initialize\r\n");
    return S_OK;
}

void Hook()
{
    extern FARJMP Old_SLGetWindowsInformationDWORD, Stub_SLGetWindowsInformationDWORD;
    extern SLGETWINDOWSINFORMATIONDWORD _SLGetWindowsInformationDWORD;
    extern HMODULE hSLC;
    extern HMODULE hTermSrv;
    extern VS_VERSIONINFO *hResData;

    AlreadyHooked = true;
    char *Log;
    wchar_t wIniFile[256];
    string sIniFile;
    SIZE_T bw;
    PLATFORM_DWORD SignPtr;
    FARJMP Jump;

#pragma comment(lib, "shlwapi.lib")

    GetModuleFileName(GetCurrentModule(), LogFile, MAX_PATH);
    for (size_t i = wcslen(LogFile); i > 0; i--)
    {
        if (LogFile[i - 1] == L'\\')
        {
            wmemset(&LogFile[i], 0x00, MAX_PATH - i);
            PathCombine(LogFile, LogFile, L"rdpwrap.txt");
            break;
        }
    }

    WriteToLog("Loading configuration...\r\n");

    GetModuleFileName(GetCurrentModule(), wIniFile, 255);
    for (size_t i = wcslen(wIniFile); i > 0; i--)
    {
        if (wIniFile[i] == '\\')
        {
            memset(&wIniFile[i + 1], 0x00, ((256 - (static_cast<size_t>(i) + 1))) * 2);
            memcpy(&wIniFile[i + 1], L"rdpwrap.ini", wcslen(L"rdpwrap.ini") * 2);
            break;
        }
    }
    int len = WideCharToMultiByte(CP_UTF8, 0, wIniFile, -1, NULL, 0, NULL, NULL);
    sIniFile.resize(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wIniFile, -1, &sIniFile[0], len, NULL, NULL);

    if (!parser.load(sIniFile))
    {
        WriteToLog("Error: Failed to load configuration\r\n");
        return;
    }

    Log = new char[1024];
    wsprintfA(Log, "Configuration file: %S\r\n", wIniFile);
    WriteToLog(Log);
    delete[] Log;

    WriteToLog("Initializing RDP Wrapper...\r\n");

    hTermSrv = LoadLibrary(L"termsrv.dll");
    if (hTermSrv == 0)
    {
        WriteToLog("Error: Failed to load Terminal Services library\r\n");
        return;
    }
    _ServiceMain = (SERVICEMAIN)GetProcAddress(hTermSrv, "ServiceMain");
    _SvchostPushServiceGlobals = (SVCHOSTPUSHSERVICEGLOBALS)GetProcAddress(hTermSrv, "SvchostPushServiceGlobals");

    Log = new char[1024];
    snprintf(Log, 1024,
             "Base addr:  0x%p\r\n"
             "SvcMain:    termsrv.dll+0x%llx\r\n"
             "SvcGlobals: termsrv.dll+0x%llx\r\n",
             (void *)hTermSrv,
             (unsigned long long)((PLATFORM_DWORD)_ServiceMain - (PLATFORM_DWORD)hTermSrv),
             (unsigned long long)((PLATFORM_DWORD)_SvchostPushServiceGlobals - (PLATFORM_DWORD)hTermSrv));

    WriteToLog(Log);
    delete[] Log;

    bool version = false;

    HRSRC hResInfo;
    hResInfo = FindResourceW(hTermSrv, MAKEINTRESOURCEW(1), MAKEINTRESOURCEW(16));
    if (hResInfo != NULL)
    {
        version = true;
    }
    else
    {
        version = false;
    }

    if (version)
    {
        hResData = (PVS_VERSIONINFO)LoadResource(hTermSrv, hResInfo);
        if (hResData == NULL)
        {
            version = false;
        }
    }

    if (!version)
    {
        WriteToLog("Error: Failed to get version information\r\n");
        return;
    }

    Log = new char[1024];
    snprintf(Log, 1024, "Version:    %d.%d.%d.%d\r\n", HIWORD(hResData->Value.dwFileVersionMS), LOWORD(hResData->Value.dwFileVersionMS),
             HIWORD(hResData->Value.dwFileVersionLS), LOWORD(hResData->Value.dwFileVersionLS));
    WriteToLog(Log);
    delete[] Log;

    WriteToLog("Freezing threads...\r\n");
    SetThreadsState(false);

    bool Bool;

    Bool = parser.getBool("Main", "SLPolicyHookNT60");
    if (hResData->Value.dwFileVersionMS == 0x00060000 && Bool)
    {
        // Windows Vista
        // uses SL Policy API (slc.dll)

        // load slc.dll and hook function
        hSLC = LoadLibrary(L"slc.dll");
        if (hSLC == 0)
        {
            WriteToLog("Error: Failed to load SL Policy library\r\n");
            return;
        }
        _SLGetWindowsInformationDWORD = (SLGETWINDOWSINFORMATIONDWORD)GetProcAddress(hSLC, "SLGetWindowsInformationDWORD");
        if (_SLGetWindowsInformationDWORD != INVALID_HANDLE_VALUE)
        {
            // rewrite original function to call our function (make hook)

            WriteToLog("Hook SLGetWindowsInformationDWORD\r\n");
#ifdef _WIN64
            Stub_SLGetWindowsInformationDWORD.MovOp = 0x48;
            Stub_SLGetWindowsInformationDWORD.MovRegArg = 0xB8;
            Stub_SLGetWindowsInformationDWORD.MovArg = (PLATFORM_DWORD)New_SLGetWindowsInformationDWORD;
            Stub_SLGetWindowsInformationDWORD.PushRaxOp = 0x50;
            Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#else
            Stub_SLGetWindowsInformationDWORD.PushOp = 0x68;
            Stub_SLGetWindowsInformationDWORD.PushArg = (PLATFORM_DWORD)New_SLGetWindowsInformationDWORD;
            Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#endif

            ReadProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD, &Old_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
            WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD, &Stub_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
        }
    }

    Bool = parser.getBool("Main", "SLPolicyHookNT61");

    if (hResData->Value.dwFileVersionMS == 0x00060001 && Bool)
    {
        // Windows 7
        // uses SL Policy API (slc.dll)

        // load slc.dll and hook function
        hSLC = LoadLibrary(L"slc.dll");
        if (hSLC == 0)
        {
            WriteToLog("Error: Failed to load SL Policy library\r\n");
            return;
        }
        _SLGetWindowsInformationDWORD = (SLGETWINDOWSINFORMATIONDWORD)GetProcAddress(hSLC, "SLGetWindowsInformationDWORD");
        if (_SLGetWindowsInformationDWORD != INVALID_HANDLE_VALUE)
        {
            // rewrite original function to call our function (make hook)

            WriteToLog("Hook SLGetWindowsInformationDWORD\r\n");
#ifdef _WIN64
            Stub_SLGetWindowsInformationDWORD.MovOp = 0x48;
            Stub_SLGetWindowsInformationDWORD.MovRegArg = 0xB8;
            Stub_SLGetWindowsInformationDWORD.MovArg = (PLATFORM_DWORD)New_SLGetWindowsInformationDWORD;
            Stub_SLGetWindowsInformationDWORD.PushRaxOp = 0x50;
            Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#else
            Stub_SLGetWindowsInformationDWORD.PushOp = 0x68;
            Stub_SLGetWindowsInformationDWORD.PushArg = (PLATFORM_DWORD)New_SLGetWindowsInformationDWORD;
            Stub_SLGetWindowsInformationDWORD.RetOp = 0xC3;
#endif

            ReadProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD, &Old_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
            WriteProcessMemory(GetCurrentProcess(), _SLGetWindowsInformationDWORD, &Stub_SLGetWindowsInformationDWORD, sizeof(FARJMP), &bw);
        }
    }
    if (hResData->Value.dwFileVersionMS == 0x00060002)
    {
        // Windows 8
        // uses SL Policy internal unexported function

        // load slc.dll and get function
        // (will be used on intercepting undefined values)
        hSLC = LoadLibrary(L"slc.dll");
        if (hSLC == 0)
        {
            WriteToLog("Error: Failed to load SL Policy library\r\n");
            return;
        }
        _SLGetWindowsInformationDWORD = (SLGETWINDOWSINFORMATIONDWORD)GetProcAddress(hSLC, "SLGetWindowsInformationDWORD");
    }
    if (hResData->Value.dwFileVersionMS == 0x00060003)
    {
        // Windows 8.1
        // uses SL Policy internal inline code
    }
    if (hResData->Value.dwFileVersionMS == 0x00060004)
    {
        // Windows 10
        // uses SL Policy internal inline code
    }

    stringstream ss;
    ss << HIWORD(hResData->Value.dwFileVersionMS) << "." << LOWORD(hResData->Value.dwFileVersionMS) << "." << HIWORD(hResData->Value.dwFileVersionLS) << "." << LOWORD(hResData->Value.dwFileVersionLS);

    auto Sect = ss.str();

    TermSrvBase = (PLATFORM_DWORD)hTermSrv;
#ifdef _WIN64
    Bool = parser.getBool(Sect, "LocalOnlyPatch.x64");
#else
    Bool = parser.getBool(Sect, "LocalOnlyPatch.x86");
#endif

    if (Bool)
    {
        WriteToLog("Patch CEnforcementCore::GetInstanceOfTSLicense\r\n");
#ifdef _WIN64
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "LocalOnlyOffset.x64"));
        string PatchName = parser.getString(Sect, "LocalOnlyCode.x64");
#else
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "LocalOnlyOffset.x86"));
        string PatchName = parser.getString(Sect, "LocalOnlyCode.x86");
#endif

        if (!PatchName.empty())
        {
            string Patchcode = parser.getString("PatchCodes", PatchName);
            if (!Patchcode.empty())
            {
                auto Patch = parser.hexToMachineCode(Patchcode);
                if (SignPtr > TermSrvBase)
                {
                    WriteProcessMemory(GetCurrentProcess(), (LPVOID)SignPtr, Patch.data(), Patch.size(), &bw);
                }
            }
        }
        else
        {
            Bool = false;
        }
    }

#ifdef _WIN64
    Bool = parser.getBool(Sect, "SingleUserPatch.x64");
#else
    Bool = parser.getBool(Sect, "SingleUserPatch.x86");
#endif

    if (Bool)
    {
        WriteToLog("Patch CSessionArbitrationHelper::IsSingleSessionPerUserEnabled or CUtils::IsSingleSessionPerUser\r\n");

#ifdef _WIN64
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "SingleUserOffset.x64"));
        string PatchName = parser.getString(Sect, "SingleUserCode.x64");
#else
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "SingleUserOffset.x86"));
        string PatchName = parser.getString(Sect, "SingleUserCode.x86");
#endif

        if (!PatchName.empty())
        {
            string Patchcode = parser.getString("PatchCodes", PatchName);
            if (!Patchcode.empty())
            {
                auto Patch = parser.hexToMachineCode(Patchcode);
                if (SignPtr > TermSrvBase)
                {
                    WriteProcessMemory(GetCurrentProcess(), (LPVOID)SignPtr, Patch.data(), Patch.size(), &bw);
                }
            }
        }
    }

#ifdef _WIN64
    Bool = parser.getBool(Sect, "DefPolicyPatch.x64");
#else
    Bool = parser.getBool(Sect, "DefPolicyPatch.x86");
#endif
    if (Bool)
    {
        WriteToLog("Patch CDefPolicy::Query\r\n");
#ifdef _WIN64
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "DefPolicyOffset.x64"));
        string PatchName = parser.getString(Sect, "DefPolicyCode.x64");
#else
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "DefPolicyOffset.x86"));
        string PatchName = parser.getString(Sect, "DefPolicyCode.x86");
#endif
        if (!PatchName.empty())
        {
            string Patchcode = parser.getString("PatchCodes", PatchName);
            if (!Patchcode.empty())
            {
                auto Patch = parser.hexToMachineCode(Patchcode);

                if (SignPtr > TermSrvBase)
                {
                    WriteProcessMemory(GetCurrentProcess(), (LPVOID)SignPtr, Patch.data(), Patch.size(), &bw);
                }
            }
        }
    }

#ifdef _WIN64
    Bool = parser.getBool(Sect, "SLPolicyInternal.x64");
#else
    Bool = parser.getBool(Sect, "SLPolicyInternal.x86");

#endif
    if (Bool)
    {
        WriteToLog("Hook SLGetWindowsInformationDWORDWrapper\r\n");
        string FuncName;
#ifdef _WIN64
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "SLPolicyOffset.x64"));
        Jump.MovOp = 0x48;
        Jump.MovRegArg = 0xB8;
        Jump.MovArg = (PLATFORM_DWORD)New_Win8SL;
        Jump.PushRaxOp = 0x50;
        Jump.RetOp = 0xC3;

        FuncName = parser.getString(Sect, "SLPolicyFunc.x64", "New_Win8SL");

        if (FuncName == "New_Win8SL")
        {
            Jump.MovArg = (PLATFORM_DWORD)New_Win8SL;
        }
#else
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "SLPolicyOffset.x86"));
        Jump.PushOp = 0x68;
        Jump.PushArg = (PLATFORM_DWORD)New_Win8SL;
        Jump.RetOp = 0xC3;

        FuncName = parser.getString(Sect, "SLPolicyFunc.x86", "New_Win8SL");

        if (FuncName == "New_Win8SL")
        {
            Jump.PushArg = (PLATFORM_DWORD)New_Win8SL;
        }
        if (FuncName == "New_Win8SL_CP")
        {
            Jump.PushArg = (PLATFORM_DWORD)New_Win8SL_CP;
        }
#endif
        if (SignPtr > TermSrvBase)
        {
            WriteProcessMemory(GetCurrentProcess(), (LPVOID)SignPtr, &Jump, sizeof(FARJMP), &bw);
        }
    }

#ifdef _WIN64
    Bool = parser.getBool(Sect, "SLInitHook.x64");
#else
    Bool = parser.getBool(Sect, "SLInitHook.x86");
#endif
    if (Bool)
    {
        WriteToLog("Hook CSLQuery::Initialize\r\n");
        string FuncName;

#ifdef _WIN64
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "SLInitOffset.x64"));
        Jump.MovOp = 0x48;
        Jump.MovRegArg = 0xB8;
        Jump.MovArg = (PLATFORM_DWORD)New_CSLQuery_Initialize;
        Jump.PushRaxOp = 0x50;
        Jump.RetOp = 0xC3;

        FuncName = parser.getString(Sect, "SLInitFunc.x64", "New_CSLQuery_Initialize");

        if (FuncName == "New_CSLQuery_Initialize")
        {
            Jump.MovArg = (PLATFORM_DWORD)New_CSLQuery_Initialize;
        }

#else
        SignPtr = (PLATFORM_DWORD)(TermSrvBase + parser.getDWordHex(Sect, "SLInitOffset.x86"));
        Jump.PushOp = 0x68;
        Jump.PushArg = (PLATFORM_DWORD)New_CSLQuery_Initialize;
        Jump.RetOp = 0xC3;

        FuncName = parser.getString(Sect, "SLInitFunc.x86", "New_CSLQuery_Initialize");

        if (FuncName == "New_CSLQuery_Initialize")
        {
            Jump.PushArg = (PLATFORM_DWORD)New_CSLQuery_Initialize;
        }
#endif

        if (SignPtr > TermSrvBase)
        {
            WriteProcessMemory(GetCurrentProcess(), (LPVOID)SignPtr, &Jump, sizeof(FARJMP), &bw);
        }
    }

    WriteToLog("Resuming threads...\r\n");
    SetThreadsState(true);
    return;
}
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    WriteToLog(">>> ServiceMain\r\n");
    if (!AlreadyHooked)
        Hook();

    if (_ServiceMain != NULL)
        _ServiceMain(dwArgc, lpszArgv);
    WriteToLog("<<< ServiceMain\r\n");
}

void WINAPI SvchostPushServiceGlobals(void *lpGlobalData)
{
    WriteToLog(">>> SvchostPushServiceGlobals\r\n");
    if (!AlreadyHooked)
        Hook();

    if (_SvchostPushServiceGlobals != NULL)
        _SvchostPushServiceGlobals(lpGlobalData);
    WriteToLog("<<< SvchostPushServiceGlobals\r\n");
}