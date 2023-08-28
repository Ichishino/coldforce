#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls.h>

#ifdef CO_OS_WIN
#ifdef _USRDLL
#ifdef CO_USE_OPENSSL
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#else
#pragma message ("<<co_tls>> **** OpenSSL was not found.")
#endif // CO_USE_OPENSSL
#endif // _USRDLL
#endif // CO_OS_WIN

