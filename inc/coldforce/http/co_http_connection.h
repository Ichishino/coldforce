#ifndef CO_HTTP_CONNECTION_H_INCLUDED
#define CO_HTTP_CONNECTION_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_url.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_request.h>
#include <coldforce/http/co_http_response.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http connection
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http_connection_t;

typedef void(*co_http_connection_connect_fn)(
    co_thread_t* self, struct co_http_connection_t* client,
    int error_code);
typedef void(*co_http_connection_close_fn)(
    co_thread_t* self, struct co_http_connection_t* client);

typedef struct
{
    co_http_connection_connect_fn on_connect;
    co_http_connection_close_fn on_close;

} co_http_connection_callbacks_st;

typedef struct co_http_connection_t
{
    co_tcp_client_t* tcp_client;
    co_tcp_client_module_t module;
    co_http_connection_callbacks_st callbacks;

    co_url_st* url_origin;

    struct co_http_connection_receive_data_t
    {
        size_t index;
        co_byte_array_t* ptr;

    } receive_data;

} co_http_connection_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http_connection_on_tcp_connect(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
);

void
co_http_connection_on_tcp_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

CO_HTTP_API
bool
co_http_connection_setup(
    co_http_connection_t* conn,
    co_url_st* url_origin,
    const co_net_addr_t* local_net_addr,
    const char** protocols,
    size_t protocol_count,
    co_tls_ctx_st* tls_ctx
);

CO_HTTP_API
void
co_http_connection_cleanup(
    co_http_connection_t* conn
);

CO_HTTP_API
void
co_http_connection_move(
    co_http_connection_t* from_conn,
    co_http_connection_t* to_conn
);

CO_HTTP_API
bool
co_http_connection_is_server(
    const co_http_connection_t* conn
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
bool
co_http_connection_send_request(
    co_http_connection_t* conn,
    const co_http_request_t* request
);

CO_HTTP_API
bool
co_http_connection_send_response(
    co_http_connection_t* conn,
    const co_http_response_t* response
);

CO_HTTP_API
bool
co_http_connection_send_data(
    co_http_connection_t* conn,
    const void* data,
    size_t data_size
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_CONNECTION_H_INCLUDED
