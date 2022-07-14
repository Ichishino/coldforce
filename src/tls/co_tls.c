#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls.h>

#ifdef CO_OS_WIN
#ifdef _USRDLL
#ifdef CO_CAN_USE_TLS
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif
#endif // _USRDLL
#endif // CO_OS_WIN
