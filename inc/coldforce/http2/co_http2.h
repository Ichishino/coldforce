#ifndef CO_HTTP2_H_INCLUDED
#define CO_HTTP2_H_INCLUDED

#include <coldforce/core/co.h>

// #define CO_HTTP2_DEBUG

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_HTTP2_EXPORTS
#       define CO_HTTP2_API  __declspec(dllexport)
#   else
#       define CO_HTTP2_API  __declspec(dllimport)
#   endif
#else
#   define CO_HTTP2_API
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef CO_LITTLE_ENDIAN

#define co_byte_order_16_flip(n) \
    (((n & 0x00FF) << 8) | ((n & 0xFF00) >> 8))
#define co_byte_order_32_flip(n) \
    (((n & 0x0000000FF) << 24) | ((n & 0x00000FF00) << 8) | \
        ((n & 0x00FF0000) >> 8) | ((n & 0xFF000000) >>24))

#define co_byte_order_16_network_to_host(n) co_byte_order_16_flip(n)
#define co_byte_order_16_host_to_network(n) co_byte_order_16_flip(n)
#define co_byte_order_32_network_to_host(n) co_byte_order_32_flip(n)
#define co_byte_order_32_host_to_network(n) co_byte_order_32_flip(n)

#else
#define co_byte_order_16_network_to_host(n) (n)
#define co_byte_order_16_host_to_network(n) (n)
#define co_byte_order_32_network_to_host(n) (n)
#define co_byte_order_32_host_to_network(n) (n)
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP2_VERSION_2_0         "HTTP/2"
#define CO_HTTP2_ALPN_NAME           "h2"

#define CO_HTTP2_CONNECTION_PREFACE  "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define CO_HTTP2_CONNECTION_PREFACE_LENGTH  24

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_H_INCLUDED
