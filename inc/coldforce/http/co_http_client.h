#ifndef CO_HTTP_CLIENT_H_INCLUDED
#define CO_HTTP_CLIENT_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_url.h>

#include <coldforce/tls/co_tls.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_request.h>
#include <coldforce/http/co_http_response.h>
#include <coldforce/http/co_http_content_receiver.h>
#include <coldforce/http/co_http_connection.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http_client_t;

typedef void(*co_http_connect_fn)(
    co_thread_t* self, struct co_http_client_t* client,
    int error_code);

typedef bool(*co_http_receive_start_fn)(
    co_thread_t* self, struct co_http_client_t* client,
    const co_http_request_t* request,
    const co_http_response_t* response);

typedef void(*co_http_receive_finish_fn)(
    co_thread_t* self, struct co_http_client_t* client,
    const co_http_request_t* request,
    const co_http_response_t* response,
    int error_code);

typedef bool(*co_http_receive_data_fn)(
    co_thread_t* self, struct co_http_client_t* client,
    const co_http_request_t* request,
    const co_http_response_t* response,
    const uint8_t* data_block, size_t data_block_size);

typedef void(*co_http_close_fn)(
    co_thread_t* self, struct co_http_client_t* client);

typedef struct
{
    co_http_connect_fn on_connect;
    co_http_receive_start_fn on_receive_start;
    co_http_receive_finish_fn on_receive_finish;
    co_http_receive_data_fn on_receive_data;
    co_http_close_fn on_close;

} co_http_callbacks_st;

typedef struct co_http_client_t
{
    co_http_connection_t conn;
    co_http_callbacks_st callbacks;

    co_list_t* request_queue;
    co_http_content_receiver_t content_receiver;

    co_http_request_t* request;
    co_http_response_t* response;

} co_http_client_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http_client_setup(
    co_http_client_t* client
);

void
co_http_client_on_tcp_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

void
co_http_client_on_http_connection_close(
    co_thread_t* thread,
    co_http_connection_t* conn
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
co_http_client_t*
co_http_client_create(
    const char* url_origin,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_HTTP_API
void
co_http_client_destroy(
    co_http_client_t* client
);

CO_HTTP_API
co_http_callbacks_st*
co_http_get_callbacks(
    co_http_client_t* client
);

CO_HTTP_API
bool
co_http_connect_start(
    co_http_client_t* client
);

CO_HTTP_API
void
co_http_close(
    co_http_client_t* client
);

CO_HTTP_API
bool
co_http_send_request(
    co_http_client_t* client,
    co_http_request_t* request
);

CO_HTTP_API
bool
co_http_send_data(
    co_http_client_t* client,
    const void* data,
    size_t data_size
);

CO_HTTP_API
bool
co_http_is_running(
    const co_http_client_t* client
);

CO_HTTP_API
const co_net_addr_t*
co_http_get_remote_net_addr(
    const co_http_client_t* client
);

CO_HTTP_API
co_socket_t*
co_http_client_get_socket(
    co_http_client_t* client
);

CO_HTTP_API
const char*
co_http_get_url_origin(
    const co_http_client_t* client
);

CO_HTTP_API
bool
co_http_is_open(
    const co_http_client_t* client
);

CO_HTTP_API
void
co_http_set_user_data(
    co_http_client_t* client,
    void* user_data
);

CO_HTTP_API
void*
co_http_get_user_data(
    const co_http_client_t* client
);

CO_HTTP_API
co_http_response_t*
co_http_sync_request(
    const char* url,
    co_http_request_t* request,
    const char* save_file_name,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_CLIENT_H_INCLUDED
