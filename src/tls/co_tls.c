#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls.h>

#include <openssl/conf.h>
#include <openssl/crypto.h>

#ifdef CO_OS_WIN

#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#include <windows.h>

//---------------------------------------------------------------------------//
// tls (windows)
//---------------------------------------------------------------------------//

#ifdef _USRDLL

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

#endif // _USRDLL

#endif // CO_OS_WIN

bool
co_tls_setup(
    void
)
{
    // TODO
    OPENSSL_init_ssl(0, NULL);

    return true;
}

void
co_tls_cleanup(
    void
)
{
    // TODO
    ERR_free_strings();
}
