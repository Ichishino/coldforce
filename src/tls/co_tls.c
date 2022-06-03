#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls.h>

#ifdef CO_CAN_USE_TLS
#include <openssl/conf.h>
#include <openssl/crypto.h>
#else
#pragma message("[CO_TLS] error: 'OpenSSL' not found.")
#endif

#ifdef CO_OS_WIN

#include <windows.h>

//---------------------------------------------------------------------------//
// tls (windows)
//---------------------------------------------------------------------------//

#ifdef _USRDLL

#ifdef CO_CAN_USE_TLS
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif

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
#ifdef CO_CAN_USE_TLS
    // TODO
    OPENSSL_init_ssl(0, NULL);

    return true;
#else

    printf("[CO_TLS] error: 'OpenSSL' not found.\n");

    return false;
#endif
}

void
co_tls_cleanup(
    void
)
{
#ifdef CO_CAN_USE_TLS
    // TODO
    ERR_free_strings();
#endif
}
