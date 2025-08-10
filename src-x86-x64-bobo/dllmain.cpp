#include "stdafx.h"

extern HMODULE hTermSrv;

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        if (hTermSrv != NULL)
        {
            FreeLibrary(hTermSrv);
        }
        break;
    }
    return TRUE;
}