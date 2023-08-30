#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls.h>

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

