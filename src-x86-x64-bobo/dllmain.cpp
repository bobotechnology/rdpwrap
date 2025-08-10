#include "stdafx.h"

extern HMODULE hTermSrv;

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_DETACH && hTermSrv != NULL)
    {
        FreeLibrary(hTermSrv);
    }
    return TRUE;
}