// stdafx.h: 预编译头文件，用于在编译过程中包含预编译头文件的声明
// 此文件通常包含不经常修改的头文件和系统头文件
// 预编译头文件可以加快编译速度
//

#pragma once

#include "targetver.h"
            // 通过定义 WIN32_LEAN_AND_MEAN 减少 Windows 头文件的包含，加快编译速度
#define _CRT_SECURE_NO_WARNINGS


// Windows 系统头文件:
#include <windows.h>
#include <TlHelp32.h>


#include <string>
#include <cwchar>
#include <shlwapi.h>
// TODO: 在此处引用你的程序需要的其他头文件
// 例如，可以将一些不经常修改的头文件引用放在这里，而不是放在 stdafx.h 中


typedef VOID(WINAPI *SERVICEMAIN)(DWORD, LPTSTR *);
typedef VOID(WINAPI *SVCHOSTPUSHSERVICEGLOBALS)(VOID *);
typedef HRESULT(WINAPI *SLGETWINDOWSINFORMATIONDWORD)(PCWSTR, DWORD *);