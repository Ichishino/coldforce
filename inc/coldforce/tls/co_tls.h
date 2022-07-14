#ifndef CO_TLS_H_INCLUDED
#define CO_TLS_H_INCLUDED

#include <coldforce/core/co.h>

#if defined __has_include
#   if __has_include(<openssl/ssl.h>)
#       define CO_CAN_USE_TLS       1
#   endif
#else
#   define CO_CAN_USE_TLS           1
#endif // __has_include

#ifdef CO_CAN_USE_TLS

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4090)
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // CO_CAN_USE_TLS

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_TLS_EXPORTS
#       define CO_TLS_API  __declspec(dllexport)
#   else
#       define CO_TLS_API
#   endif
#else
#   define CO_TLS_API
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_TLS_ERROR_HANDSHAKE_FAILED   -4001

#ifdef CO_CAN_USE_TLS

typedef struct
{
    SSL_CTX* ssl_ctx;

} co_tls_ctx_st;

#else

typedef struct
{
    void* unused;

} co_tls_ctx_st;

#endif // CO_CAN_USE_TLS

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_H_INCLUDED
