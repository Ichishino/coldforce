#include <coldforce/core/co_std.h>

#include <coldforce/http/co_http.h>

#ifdef CO_OS_WIN

#include <windows.h>

//---------------------------------------------------------------------------//
// http (windows)
//---------------------------------------------------------------------------//

BOOL APIENTRY
DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    (void)hModule;
    (void)lpReserved;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    default:
        break;
    }

    return TRUE;
}

#endif // CO_OS_WIN
