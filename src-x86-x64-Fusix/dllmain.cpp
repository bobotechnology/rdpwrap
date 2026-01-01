// dllmain.cpp: main source file for the DLL application.
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	// Avoid unreferenced parameter warnings
	(void)hModule;
	(void)ul_reason_for_call;
	(void)lpReserved;

	return true;
}