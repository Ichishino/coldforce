#ifndef CO_TLS_H_INCLUDED
#define CO_TLS_H_INCLUDED

#include <coldforce/core/co.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4090)
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_TLS_EXPORTS
#       define CO_TLS_API  __declspec(dllexport)
#   else
#       define CO_TLS_API  __declspec(dllimport)
#   endif
#else
#   define CO_TLS_API
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    SSL_CTX* ssl_ctx;

} co_tls_ctx_st;

CO_TLS_API bool co_tls_setup(void);
CO_TLS_API void co_tls_cleanup(void);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_H_INCLUDED
