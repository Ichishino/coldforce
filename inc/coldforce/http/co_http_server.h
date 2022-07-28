#ifndef CO_HTTP_SERVER_H_INCLUDED
#define CO_HTTP_SERVER_H_INCLUDED

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_client.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
co_http_client_t*
co_tcp_upgrade_to_http(
    co_tcp_client_t* tcp_client
);

CO_HTTP_API
bool
co_http_send_response(
    co_http_client_t* client,
    co_http_response_t* response
);

CO_HTTP_API
bool
co_http_begin_chunked_response(
    co_http_client_t* client,
    co_http_response_t* response
);

CO_HTTP_API
bool
co_http_send_chunked_response(
    co_http_client_t* client,
    const void* data,
    size_t data_length
);

CO_HTTP_API
bool
co_http_end_chunked_response(
    co_http_client_t* client
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_SERVER_H_INCLUDED
