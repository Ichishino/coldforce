#ifndef CO_HTTP_CLIENT_H_INCLUDED
#define CO_HTTP_CLIENT_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/tls/co_tls.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_url.h>
#include <coldforce/http/co_http_request.h>
#include <coldforce/http/co_http_response.h>
#include <coldforce/http/co_http_content_receiver.h>

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
    void (*destroy)(co_tcp_client_t*);
    void (*close)(co_tcp_client_t*);
    bool (*connect)(co_tcp_client_t*, const co_net_addr_t*);
    bool (*send)(co_tcp_client_t*, const void*, size_t);
    ssize_t (*receive_all)(co_tcp_client_t*, co_byte_array_t*);

} co_tcp_client_module_t;

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
    co_tcp_client_t* tcp_client;
    co_tcp_client_module_t module;

    co_http_callbacks_st callbacks;

    co_http_url_st* base_url;
    co_list_t* receive_queue;

    size_t receive_data_index;
    co_byte_array_t* receive_data;

    co_http_request_t* request;
    co_http_response_t* response;
    co_http_content_receiver_t content_receiver;

    co_timer_t* receive_timer;

} co_http_client_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http_client_setup(
    co_http_client_t* client
);

bool
co_http_send_raw_data(
    co_http_client_t* client,
    const void* data,
    size_t data_size
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
co_http_client_t*
co_http_client_create(
    const char* base_url,
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
co_http_connect(
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
co_http_get_base_url(
    const co_http_client_t* client
);

CO_HTTP_API
bool
co_http_is_open(
    const co_http_client_t* client
);

CO_HTTP_API
bool
co_http_set_user_data(
    co_http_client_t* client,
    uintptr_t user_data
);

CO_HTTP_API
bool
co_http_get_user_data(
    const co_http_client_t* client,
    uintptr_t* user_data
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_CLIENT_H_INCLUDED
