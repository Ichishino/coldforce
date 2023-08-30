#ifndef CO_TLS_H_INCLUDED
#define CO_TLS_H_INCLUDED

#include <coldforce/core/co.h>

#ifdef CO_USE_WOLFSSL

// wolfssl(openssl compatible)

#define OPENSSL_EXTRA
#define WC_NO_HARDEN

#include <wolfssl/openssl/ssl.h>

#define CO_USE_OPENSSL

#elif defined(CO_USE_OPENSSL) || !defined(CO_NO_TLS)

// openssl(default)

#ifndef CO_USE_OPENSSL
#ifdef __has_include
#   if __has_include(<openssl/ssl.h>)
#       define CO_USE_OPENSSL
#   endif
#else
#   define CO_USE_OPENSSL
#endif
#endif // !CO_USE_OPENSSL

#ifdef CO_USE_OPENSSL
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4090)
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // CO_USE_OPENSSL

#endif //

#if defined(CO_USE_OPENSSL) || defined(CO_USE_WOLFSSL)
#define CO_USE_TLS
#endif

#ifdef CO_USE_OPENSSL
typedef SSL      CO_SSL_T;
typedef SSL_CTX  CO_SSL_CTX_T;
typedef BIO      CO_BIO_T;
typedef X509     CO_X509_T;
#else
typedef struct { void* unused; } CO_SSL_T;
typedef struct { void* unused; } CO_SSL_CTX_T;
typedef struct { void* unused; } CO_BIO_T;
typedef struct { void* unused; } CO_X509_T;
#endif

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

typedef struct
{
    CO_SSL_CTX_T* ssl_ctx;

} co_tls_ctx_st;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_H_INCLUDED
