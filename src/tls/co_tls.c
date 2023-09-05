#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls.h>
#include <coldforce/tls/co_tls_log.h>

#ifdef CO_OS_WIN
#   ifdef _USRDLL
#       ifdef CO_USE_WOLFSSL
#           pragma comment(lib, "wolfssl.lib")
#           pragma comment(lib, "ws2_32.lib")
#       elif defined(CO_USE_OPENSSL)
#           pragma comment(lib, "libssl.lib")
#           pragma comment(lib, "libcrypto.lib")
#       endif // CO_USE_WOLFSSL
#   endif // _USRDLL
#   ifdef CO_USE_WOLFSSL
#       pragma message ("co_tls: Use wolfSSL")
#   elif defined(CO_USE_OPENSSL)
#       pragma message ("co_tls: Use OpenSSL")
#   else
#       pragma message ("co_tls: *** Use Nothing")
#   endif //
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
    static bool once = false;

    if (!once)
    {
        once = true;
    }
    else
    {
        return true;
    }

#ifdef CO_USE_WOLFSSL
    co_tls_log_info(NULL, NULL, NULL, "Use wolfSSL");
#elif defined(CO_USE_OPENSSL)
    co_tls_log_info(NULL, NULL, NULL, "Use OpenSSL");
#else
    co_tls_log_info(NULL, NULL, NULL, "Use ** nothing **");
#endif

#ifdef CO_USE_OPENSSL_COMPATIBLE

    int result = OPENSSL_init_ssl(0, NULL);

    if (result == 1)
    {
        atexit(co_tls_cleanup);

        return true;
    }

    return false;

#else

    return true;

#endif // CO_USE_OPENSSL_COMPATIBLE
}

void
co_tls_cleanup(
    void
)
{
    static bool once = false;

    if (!once)
    {
        once = true;
    }
    else
    {
        return;
    }

#ifdef CO_USE_OPENSSL_COMPATIBLE
    OPENSSL_cleanup();
#endif
}
