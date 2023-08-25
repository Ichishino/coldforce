#ifndef CO_HTTP_TCP_EXTENSION_H_INCLUDED
#define CO_HTTP_TCP_EXTENSION_H_INCLUDED

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_client.h>
#include <coldforce/http/co_http_connection.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tcp extension for http
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_HTTP_API
bool
co_tcp_upgrade_to_http_connection(
    co_tcp_client_t* tcp_client,
    co_http_connection_t* conn,
    const char* url_origin
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
co_http_client_t*
co_tcp_upgrade_to_http(
    co_tcp_client_t* tcp_client,
    const char* url_origin
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_TCP_EXTENSION_H_INCLUDED
