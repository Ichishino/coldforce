#ifndef CO_HTTP2_STREAM_H_INCLUDED
#define CO_HTTP2_STREAM_H_INCLUDED

#include <coldforce/core/co_list.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/http2/co_http2.h>
#include <coldforce/http2/co_http2_frame.h>
#include <coldforce/http2/co_http2_message.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 stream
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http2_client_t;

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

typedef struct co_http2_stream_t
{
    uint32_t id;
    uint8_t state;

    struct co_http2_client_t* client;

    co_http2_message_t* send_message;
    co_http2_message_t* receive_message;

    co_http2_message_fn on_message;

    struct co_http2_header_block_pool_t
    {
        uint8_t type;
        co_byte_array_t* data;

    } header_block_pool;

    co_byte_array_t* send_data_pool;
    co_byte_array_t* receive_data_pool;

    uint32_t local_window_size;
    uint32_t remote_window_size;
    uint32_t current_window_size;

    uint32_t promised_stream_id;

    FILE* data_save_fp;

} co_http2_stream_t;

void co_http2_stream_frame_trace(
    co_http2_stream_t* stream, bool send, const co_http2_frame_t* frame);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_http2_stream_t* co_http2_stream_create(
    uint32_t id, struct co_http2_client_t* client, co_http2_message_fn message_handler);
void co_http2_stream_destroy(co_http2_stream_t* stream);


bool co_http2_stream_on_receive_frame(co_http2_stream_t* stream, co_http2_frame_t* frame);
bool co_http2_stream_send_frame(co_http2_stream_t* stream, co_http2_frame_t* frame);
void co_http2_stream_update_local_window_size(co_http2_stream_t* stream, uint32_t consumed_size);

bool co_http2_stream_send_request_message(
    co_http2_stream_t* stream,
    co_http2_message_t* message);

bool co_http2_stream_send_data_frame(
    co_http2_stream_t* stream,
    const co_http2_data_t* data);

bool co_http2_stream_send_headers_frame(
    co_http2_stream_t* stream,
    bool end_stream, const co_http2_header_t* header,
    uint32_t stream_dependency, uint8_t weight);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP2_API const co_http2_message_t* co_http2_stream_get_send_message(
    const co_http2_stream_t* stream);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_STREAM_H_INCLUDED
