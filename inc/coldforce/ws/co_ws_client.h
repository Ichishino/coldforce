#ifndef CO_WS_CLIENT_H_INCLUDED
#define CO_WS_CLIENT_H_INCLUDED

#include <coldforce/http/co_http_client.h>

#include <coldforce/ws/co_ws.h>
#include <coldforce/ws/co_ws_frame.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// websocket client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_ws_client_t;

typedef void(*co_ws_connect_fn)(
    co_thread_t* self, struct co_ws_client_t*,
    int error_code);

typedef void(*co_ws_upgrade_fn)(
    co_thread_t* self, struct co_ws_client_t*,
    const co_http_message_t* request_or_response,
    int error_code);

typedef void(*co_ws_receive_frame_fn)(
    co_thread_t* self, struct co_ws_client_t*, const co_ws_frame_t*, int error_code);

typedef void(*co_ws_close_fn)(
    co_thread_t* self, struct co_ws_client_t*);

typedef struct
{
    co_ws_connect_fn on_connect;
    co_ws_upgrade_fn on_upgrade;
    co_ws_receive_frame_fn on_receive_frame;
    co_ws_close_fn on_close;

} co_ws_callbacks_st;

typedef struct co_ws_client_t
{
    co_http_connection_t conn;
    co_ws_callbacks_st callbacks;
    co_http_request_t* upgrade_request;

    bool mask;
    bool closed;

} co_ws_client_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_ws_client_setup(
    co_ws_client_t* client
);

void
co_ws_client_cleanup(
    co_ws_client_t* client
);

void
co_ws_client_on_receive_frame(
    co_thread_t* thread,
    co_ws_client_t* client,
    co_ws_frame_t* frame,
    int error_code
);

void
co_ws_client_on_tcp_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

void
co_ws_client_on_http_connection_close(
    co_thread_t* thread,
    co_http_connection_t* conn
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_WS_API
co_ws_client_t*
co_ws_client_create(
    const char* url,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_WS_API
void
co_ws_client_destroy(
    co_ws_client_t* client
);

CO_WS_API
co_ws_callbacks_st*
co_ws_get_callbacks(
    co_ws_client_t* client
);

CO_WS_API
bool
co_ws_connect_start(
    co_ws_client_t* client
);

CO_WS_API
bool
co_ws_send_upgrade_request(
    co_ws_client_t* client,
    co_http_request_t* upgrade_request
);

CO_WS_API
void
co_ws_close(
    co_ws_client_t* client
);

CO_WS_API
bool
co_ws_send(
    co_ws_client_t* client,
    bool fin,
    uint8_t opcode,
    const void* data,
    size_t data_size
);

CO_WS_API
bool
co_ws_send_binary(
    co_ws_client_t* client,
    const void* data,
    size_t data_size
);

CO_WS_API
bool
co_ws_send_text(
    co_ws_client_t* client,
    const char* utf8_str
);

CO_WS_API
bool
co_ws_send_continuation(
    co_ws_client_t* client,
    bool fin,
    const void* data,
    size_t data_size
);

CO_WS_API
bool
co_ws_send_close(
    co_ws_client_t* client,
    uint16_t reason_code,
    const char* utf8_reason_str
);

CO_WS_API
bool
co_ws_send_ping(
    co_ws_client_t* client,
    const void* data,
    size_t data_size
);

CO_WS_API
bool
co_ws_send_pong(
    co_ws_client_t* client,
    const void* data,
    size_t data_size
);

CO_WS_API
void
co_ws_default_handler(
    co_ws_client_t* client,
    const co_ws_frame_t* frame
);

CO_WS_API
co_socket_t*
co_ws_get_socket(
    co_ws_client_t* client
);

CO_WS_API
const char*
co_ws_get_url_origin(
    const co_ws_client_t* client
);

CO_WS_API
bool
co_ws_is_open(
    const co_ws_client_t* client
);

CO_WS_API
void
co_ws_set_user_data(
    co_ws_client_t* client,
    void* user_data
);

CO_WS_API
void*
co_ws_get_user_data(
    const co_ws_client_t* client
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_CLIENT_H_INCLUDED
