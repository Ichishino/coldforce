#ifndef CO_HTTP2_CLIENT_H_INCLUDED
#define CO_HTTP2_CLIENT_H_INCLUDED

#include <coldforce/core/co_map.h>

#include <coldforce/http/co_http_client.h>

#include <coldforce/http2/co_http2.h>
#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_hpack.h>
#include <coldforce/http2/co_http2_header.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http2_client_t;

typedef void(*co_http2_connect_fn)(
    co_thread_t* self, struct co_http2_client_t* client, int error_code);

typedef void(*co_http2_upgrade_fn)(
    co_thread_t* self, struct co_http2_client_t* client,
    const co_http_response_t* response, int error_code);

typedef void(*co_http2_close_fn)(
    co_thread_t* self, struct co_http2_client_t* client, int error_code);

typedef void(*co_http2_priority_fn)(
    co_thread_t* self, struct co_http2_client_t* client, co_http2_stream_t* stream,
    uint32_t stream_dependency, uint8_t weight);

typedef void(*co_http2_window_update_fn)(
    co_thread_t* self, struct co_http2_client_t* client, co_http2_stream_t* stream);

typedef void(*co_http2_close_stream_fn)(
    co_thread_t* self, struct co_http2_client_t* client, co_http2_stream_t* stream, int error_code);

typedef bool(*co_http2_push_request_fn)(
    co_thread_t* self, struct co_http2_client_t* client,
    const co_http2_stream_t* request_stream, co_http2_stream_t* response_stream,
    const co_http2_header_t* request_header);

typedef void(*co_http2_ping_fn)(
    co_thread_t* self, struct co_http2_client_t* client, uint64_t user_data);

typedef struct
{
    uint32_t header_table_size;
    uint32_t enable_push;
    uint32_t max_concurrent_streams;
    uint32_t initial_window_size;
    uint32_t max_frame_size;
    uint32_t max_header_list_size;

} co_http2_settings_st;

typedef struct
{
    co_http2_connect_fn on_connect;
    co_http2_upgrade_fn on_upgrade;
    co_http2_close_fn on_close;
    co_http2_receive_start_fn on_receive_start;
    co_http2_receive_finish_fn on_receive_finish;
    co_http2_receive_data_fn on_receive_data;
    co_http2_push_request_fn on_push_request;
    co_http2_receive_start_fn on_push_start;
    co_http2_receive_finish_fn on_push_finish;
    co_http2_receive_data_fn on_push_data;
    co_http2_priority_fn on_priority;
    co_http2_window_update_fn on_window_update;
    co_http2_close_stream_fn on_close_stream;
    co_http2_ping_fn on_ping;

} co_http2_callbacks_st;

typedef struct co_http2_client_t
{
    co_tcp_client_t* tcp_client;
    co_tcp_client_module_t module;

    co_http2_callbacks_st callbacks;

    co_http_url_st* base_url;

    size_t receive_data_index;
    co_byte_array_t* receive_data;

    co_http2_stream_t* system_stream;
    co_map_t* stream_map;

    uint32_t last_stream_id;
    uint32_t new_stream_id;

    co_byte_array_t* upgrade_request_data;

    co_http2_settings_st local_settings;
    co_http2_settings_st remote_settings;

    co_http2_hpack_dynamic_table_t local_dynamic_table;
    co_http2_hpack_dynamic_table_t remote_dynamic_table;

} co_http2_client_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http2_client_setup(
    co_http2_client_t* client
);

void
co_http2_client_on_tcp_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

void
co_http2_client_on_tcp_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

bool
co_http2_send_raw_data(
    co_http2_client_t* client,
    const void* data,
    size_t data_size
);

void
co_http2_client_on_close(
    co_http2_client_t* client,
    int error_code
);

void
co_http2_client_on_push_promise(
    co_http2_client_t* client,
    co_http2_stream_t* stream,
    uint32_t promised_id,
    co_http2_header_t* header
);

void
co_http2_client_on_receive_system_frame(
    co_http2_client_t* client,
    const co_http2_frame_t* frame
);

bool
co_http2_set_upgrade_settings(
    const char* b64_settings,
    size_t b64_settings_length,
    co_http2_settings_st* settings
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP2_API
co_http2_client_t*
co_http2_client_create(
    const char* base_url,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_HTTP2_API
void
co_http2_client_destroy(
    co_http2_client_t* client
);

CO_HTTP2_API
co_http2_callbacks_st*
co_http2_get_callbacks(
    co_http2_client_t* client
);

CO_HTTP2_API
bool
co_http2_is_running(
    const co_http2_client_t* client
);

CO_HTTP2_API
bool
co_http2_connect(
    co_http2_client_t* client
);

CO_HTTP2_API
void
co_http2_close(
    co_http2_client_t* client,
    int error_code
);

CO_HTTP2_API
bool
co_http2_connect_and_request_upgrade(
    co_http2_client_t* client,
    co_http_request_t* upgrade_request
);

CO_HTTP2_API
co_http2_stream_t*
co_http2_get_stream(
    co_http2_client_t* client,
    uint32_t stream_id
);

CO_HTTP2_API
co_http2_stream_t*
co_http2_create_stream(
    co_http2_client_t* client
);

CO_HTTP2_API
void
co_http2_destroy_stream(
    co_http2_client_t* client,
    co_http2_stream_t* stream
);

CO_HTTP2_API
bool
co_http2_send_initial_settings(
    co_http2_client_t* client
);

CO_HTTP2_API
void
co_http2_init_settings(
    co_http2_client_t* client,
    const co_http2_setting_param_st* params,
    uint16_t param_count
);

CO_HTTP2_API
void
co_http2_update_settings(
    co_http2_client_t* client,
    const co_http2_setting_param_st* params,
    uint16_t param_count
);

CO_HTTP2_API
const co_http2_settings_st*
co_http2_get_local_settings(
    const co_http2_client_t* client
);

CO_HTTP2_API
const co_http2_settings_st*
co_http2_get_remote_settings(
    const co_http2_client_t* client
);

CO_HTTP2_API
const co_net_addr_t*
co_http2_get_remote_net_addr(
    const co_http2_client_t* client
);

CO_HTTP2_API
co_socket_t*
co_http2_client_get_socket(
    co_http2_client_t* client
);

CO_HTTP2_API
const char*
co_http2_get_base_url(
    const co_http2_client_t* client
);

CO_HTTP2_API
bool
co_http2_is_open(
    const co_http2_client_t* client
);

CO_HTTP2_API
void
co_http2_set_user_data(
    co_http2_client_t* client,
    void* user_data
);

CO_HTTP2_API
void*
co_http2_get_user_data(
    const co_http2_client_t* client
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_CLIENT_H_INCLUDED
