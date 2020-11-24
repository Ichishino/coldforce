#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_win.h>

#ifdef CO_OS_WIN

#include <coldforce/net/co_tcp_win.h>

#pragma comment(lib, "ws2_32.lib")

//---------------------------------------------------------------------------//
// network (windows)
//---------------------------------------------------------------------------//

bool
co_win_net_setup(
    void
)
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return false;
    }

    if (!co_win_tcp_load_functions())
    {
        return false;
    }

    return true;
}

void
co_win_net_cleanup(
    void
)
{
    WSACleanup();
}

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
