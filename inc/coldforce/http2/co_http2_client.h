#ifndef CO_HTTP2_CLIENT_H_INCLUDED
#define CO_HTTP2_CLIENT_H_INCLUDED

#include <coldforce/core/co_map.h>

#include <coldforce/http/co_http_client.h>

#include <coldforce/http2/co_http2.h>
#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_hpack.h>
#include <coldforce/http2/co_http2_message.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP2_ERROR_STREAM_CLOSED        -6000
#define CO_HTTP2_ERROR_FILE_IO              -6101
#define CO_HTTP2_ERROR_PARSE_ERROR          -6102

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http2_client_t;

typedef void(*co_http2_close_fn)(
    void* self, struct co_http2_client_t* client, int error_code);

typedef void(*co_http2_priority_fn)(
    void* self, struct co_http2_client_t* client, co_http2_stream_t* stream,
    uint32_t stream_dependency, uint8_t weight);

typedef bool(*co_http2_push_request_fn)(
    void* self, struct co_http2_client_t* client, const co_http2_stream_t* stream,
    co_http2_message_t* push_message);

typedef struct
{
    uint32_t header_table_size;
    uint32_t enable_push;
    uint32_t max_concurrent_streams;
    uint32_t initial_window_size;
    uint32_t max_frame_size;
    uint32_t max_header_list_size;

} co_http2_settings_t;

typedef struct co_http2_client_t
{
    co_tcp_client_t* tcp_client;
    co_tcp_client_module_t module;

    bool connecting;
    co_http_url_st* base_url;
    co_list_t* request_queue;
    co_byte_array_t* receive_data;

    co_http2_stream_t* system_stream;
    co_map_t* stream_map;
    uint32_t last_stream_id;

    co_http2_close_fn on_close;

    co_http2_message_fn on_message;
    co_http2_priority_fn on_priority;

    co_http2_push_request_fn on_push_request;
    co_http2_message_fn on_push_response;

    co_http2_settings_t local_settings;
    co_http2_settings_t remote_settings;

    co_http2_hpack_dynamic_table_t local_dynamic_table;
    co_http2_hpack_dynamic_table_t remote_dynamic_table;

} co_http2_client_t;

bool co_http2_send_raw_data(
    co_http2_client_t* client, const void* data, size_t data_size);
void co_http2_client_send_all_requests(
    co_http2_client_t* client);
void co_http2_client_on_close(
    co_http2_client_t* client, int error_code);
void co_http2_client_on_push_promise_message(
    co_http2_client_t* client, co_http2_stream_t* stream,
    uint32_t promised_id, co_http2_message_t* message);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP2_API co_http2_client_t* co_http2_client_create(
    const char* base_url, const co_net_addr_t* local_net_addr, co_tls_ctx_st* tls_ctx);

CO_HTTP2_API void co_http2_client_destroy(co_http2_client_t* client);

CO_HTTP2_API bool co_http2_send_request(co_http2_client_t* client, co_http2_message_t* message);

CO_HTTP2_API void co_http2_set_message_handler(
    co_http2_client_t* client, co_http2_message_fn handler);
CO_HTTP2_API void co_http2_set_close_handler(
    co_http2_client_t* client, co_http2_close_fn handler);

CO_HTTP2_API void co_http2_set_server_push_request_handler(
    co_http2_client_t* client, co_http2_push_request_fn handler);
CO_HTTP2_API void co_http2_set_server_push_response_handler(
    co_http2_client_t* client, co_http2_message_fn handler);

CO_HTTP2_API bool co_http2_client_is_running(const co_http2_client_t* client);

CO_HTTP2_API const co_net_addr_t* co_http2_get_remote_net_addr(const co_http2_client_t* client);

CO_HTTP2_API co_socket_t* co_http2_client_get_socket(co_http2_client_t* client);
CO_HTTP2_API const char* co_http2_get_base_url(const co_http2_client_t* client);
CO_HTTP2_API bool co_http2_is_open(const co_http2_client_t* client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_CLIENT_H_INCLUDED
