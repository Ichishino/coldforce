#ifndef CO_HTTP2_TCP_EXTENSION_H_INCLUDED
#define CO_HTTP2_TCP_EXTENSION_H_INCLUDED

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/http2/co_http2.h>

CO_EXTERN_C_BEGIN

struct co_http2_client_t;

//---------------------------------------------------------------------------//
// tcp extension for http2
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP2_API
struct co_http2_client_t*
co_tcp_upgrade_to_http2(
    co_tcp_client_t* tcp_client,
    const char* url_origin
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_TCP_EXTENSION_H_INCLUDED
