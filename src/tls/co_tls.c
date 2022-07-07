#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls.h>
#include <coldforce/tls/co_tls_log.h>

#ifdef CO_OS_WIN
#ifdef _USRDLL
#ifdef CO_CAN_USE_TLS
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif
#endif // _USRDLL
#endif // CO_OS_WIN

//---------------------------------------------------------------------------//
// tls
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

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
