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
// private
//---------------------------------------------------------------------------//

void
co_http_server_on_tcp_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

void
co_http_server_on_tcp_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
bool
co_http_send_response(
    co_http_client_t* client,
    co_http_response_t* response
);

CO_HTTP_API
bool
co_http_start_chunked_response(
    co_http_client_t* client,
    co_http_response_t* response
);

CO_HTTP_API
bool
co_http_send_chunked_data(
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
