#ifndef CO_HTTP2_STREAM_H_INCLUDED
#define CO_HTTP2_STREAM_H_INCLUDED

#include <coldforce/core/co_list.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/http2/co_http2.h>
#include <coldforce/http2/co_http2_frame.h>
#include <coldforce/http2/co_http2_header.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 stream
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP2_STREAM_STATE_IDLE                  0
#define CO_HTTP2_STREAM_STATE_OPEN                  1
#define CO_HTTP2_STREAM_STATE_CLOSED                2
#define CO_HTTP2_STREAM_STATE_REMOTE_CLOSED         3
#define CO_HTTP2_STREAM_STATE_LOCAL_CLOSED          4
#define CO_HTTP2_STREAM_STATE_RESERVED_LOCAL        5
#define CO_HTTP2_STREAM_STATE_RESERVED_REMOTE       6

#define CO_HTTP2_STREAM_ERROR_NO_ERROR              0x0
#define CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR        0x1
#define CO_HTTP2_STREAM_ERROR_INTERNAL_ERROR        0x2
#define CO_HTTP2_STREAM_ERROR_FLOW_CONTROL_ERROR    0x3
#define CO_HTTP2_STREAM_ERROR_SETTINGS_TIMEOUT      0x4
#define CO_HTTP2_STREAM_ERROR_STREAM_CLOSED         0x5
#define CO_HTTP2_STREAM_ERROR_FRAME_SIZE_ERROR      0x6
#define CO_HTTP2_STREAM_ERROR_REFUSED_STREAM        0x7
#define CO_HTTP2_STREAM_ERROR_CANCEL                0x8
#define CO_HTTP2_STREAM_ERROR_COMPRESSION_ERROR     0x9
#define CO_HTTP2_STREAM_ERROR_CONNECT_ERROR         0xa
#define CO_HTTP2_STREAM_ERROR_ENHANCE_YOUR_CALM     0xb
#define CO_HTTP2_STREAM_ERROR_INADEQUATE_SECURITY   0xc
#define CO_HTTP2_STREAM_ERROR_HTTP_1_1_REQUIRED     0xd

struct co_http2_client_t;
struct co_http2_stream_t;

typedef void(*co_http2_message_fn)(
    void* self, struct co_http2_client_t* client, struct co_http2_stream_t* stream,
    const co_http2_header_t* receive_header, const co_http2_data_st* receive_data,
    int error_code);

typedef struct co_http2_stream_t
{
    uint32_t id;
    uint8_t state;

    struct co_http2_client_t* client;

    co_http2_header_t* send_header;
    co_http2_header_t* receive_header;
    co_http2_data_st receive_data;

    co_http2_message_fn on_message;

    struct co_http2_header_block_pool_t
    {
        uint8_t type;
        co_byte_array_t* data;

    } header_block_pool;

    co_byte_array_t* receive_data_pool;

    uint32_t max_local_window_size;
    uint32_t remote_window_size;
    uint32_t local_window_size;

    uint32_t promised_stream_id;

    FILE* save_data_fp;

} co_http2_stream_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

co_http2_stream_t*
co_http2_stream_create(
    uint32_t id,
    struct co_http2_client_t* client,
    co_http2_message_fn message_handler
);

void
co_http2_stream_destroy(
    co_http2_stream_t* stream
);

bool
co_http2_stream_on_receive_frame(
    co_http2_stream_t* stream,
    co_http2_frame_t* frame
);

bool
co_http2_stream_send_frame(
    co_http2_stream_t* stream,
    co_http2_frame_t* frame
);

void
co_http2_stream_update_local_window_size(
    co_http2_stream_t* stream,
    uint32_t consumed_size
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP2_API
ssize_t
co_http2_stream_send_data(
    co_http2_stream_t* stream,
    bool end_stream,
    const void* data,
    uint32_t data_size
);

CO_HTTP2_API
bool
co_http2_stream_send_header(
    co_http2_stream_t* stream,
    bool end_stream,
    co_http2_header_t* header
);

CO_HTTP2_API
co_http2_stream_t*
co_http2_stream_send_server_push_request(
    co_http2_stream_t* stream,
    co_http2_header_t* header
);

CO_HTTP2_API
bool
co_http2_stream_send_window_update(
    co_http2_stream_t* stream,
    uint32_t window_size_increment
);

CO_HTTP2_API
bool
co_http2_stream_send_rst_stream(
    co_http2_stream_t* stream,
    uint32_t error_code
);

CO_HTTP2_API
bool
co_http2_stream_send_priority(
    co_http2_stream_t* stream,
    uint32_t stream_dependency,
    uint8_t weight
);

CO_HTTP2_API
const co_http2_header_t*
co_http2_stream_get_send_header(
    const co_http2_stream_t* stream
);

CO_HTTP2_API
uint32_t
co_http2_stream_get_id(
    const co_http2_stream_t* stream
);

CO_HTTP2_API
uint32_t
co_http2_stream_get_state(
    const co_http2_stream_t* stream
);

CO_HTTP2_API
uint32_t
co_http2_stream_get_local_window_size(
    const co_http2_stream_t* stream
);

CO_HTTP2_API
uint32_t
co_http2_stream_get_remote_window_size(
    const co_http2_stream_t* stream
);

CO_HTTP2_API
uint32_t
co_http2_stream_get_sendable_data_size(
    const co_http2_stream_t* stream
);

CO_HTTP2_API
void
co_http2_stream_set_save_file_path(
    co_http2_stream_t* stream,
    const char* file_path
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_STREAM_H_INCLUDED
