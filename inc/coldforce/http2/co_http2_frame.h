#ifndef CO_HTTP2_FRAME_H_INCLUDED
#define CO_HTTP2_FRAME_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/http2/co_http2.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 frame
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP2_FRAME_TYPE_DATA            0
#define CO_HTTP2_FRAME_TYPE_HEADERS         1
#define CO_HTTP2_FRAME_TYPE_PRIORITY        2
#define CO_HTTP2_FRAME_TYPE_RST_STREAM      3
#define CO_HTTP2_FRAME_TYPE_SETTINGS        4
#define CO_HTTP2_FRAME_TYPE_PUSH_PROMISE    5
#define CO_HTTP2_FRAME_TYPE_PING            6
#define CO_HTTP2_FRAME_TYPE_GOAWAY          7
#define CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE   8
#define CO_HTTP2_FRAME_TYPE_CONTINUATION    9

#define CO_HTTP2_FRAME_FLAG_END_STREAM      0x01
#define CO_HTTP2_FRAME_FLAG_ACK             0x01
#define CO_HTTP2_FRAME_FLAG_END_HEADERS     0x04
#define CO_HTTP2_FRAME_FLAG_PADDED          0x08
#define CO_HTTP2_FRAME_FLAG_PRIORITY        0x20

#define CO_HTTP2_FRAME_HEADER_SIZE          9

struct co_http2_client_t;

typedef struct
{
    uint32_t length;
    uint8_t type;
    uint8_t flags;
    uint32_t stream_id;

    bool payload_destroy;

} co_http2_frame_header_t;

typedef struct
{
    uint8_t pad_length;
    uint32_t data_length;
    uint8_t* data;
    uint8_t* padding;

} co_http2_frame_payload_data_t;

typedef struct
{
    uint8_t pad_length;
    uint32_t stream_dependency;
    uint8_t weight;
    uint32_t header_block_fragment_length;
    uint8_t* header_block_fragment;
    uint8_t* padding;

} co_http2_frame_payload_headers_t;

typedef struct
{
    uint32_t stream_dependency;
    uint8_t weight;

} co_http2_frame_payload_priority_t;

typedef struct
{
    uint32_t error_code;

} co_http2_frame_payload_rst_stream_t;

typedef struct
{
    uint16_t param_count;
    co_http2_setting_param_st* params;

} co_http2_frame_payload_settings_t;

typedef struct
{
    uint8_t pad_length;
    uint32_t promised_stream_id;
    uint32_t header_block_fragment_length;
    uint8_t* header_block_fragment;
    uint8_t* padding;

} co_http2_frame_payload_push_promise_t;

typedef struct
{
    uint64_t opaque_data;

} co_http2_frame_payload_ping_t;

typedef struct
{
    uint32_t last_stream_id;
    uint32_t error_code;

    uint32_t additional_debug_data_length;
    uint8_t* additional_debug_data;

} co_http2_frame_payload_goaway_t;

typedef struct
{
    uint32_t window_size_increment;

} co_http2_frame_payload_window_update_t;

typedef struct
{
    uint32_t header_block_fragment_length;
    uint8_t* header_block_fragment;

} co_http2_frame_payload_continuation_t;

typedef struct
{
    co_http2_frame_header_t header;
    
    union co_http2_frame_payload_t
    {
        co_http2_frame_payload_headers_t headers;
        co_http2_frame_payload_data_t data;
        co_http2_frame_payload_priority_t priority;
        co_http2_frame_payload_rst_stream_t rst_stream;
        co_http2_frame_payload_settings_t settings;
        co_http2_frame_payload_push_promise_t push_promise;
        co_http2_frame_payload_ping_t ping;
        co_http2_frame_payload_goaway_t goaway;
        co_http2_frame_payload_window_update_t window_update;
        co_http2_frame_payload_continuation_t continuation;

    } payload;

} co_http2_frame_t;

void co_http2_frame_serialize(
    const co_http2_frame_t* frame, co_byte_array_t* buffer);
int co_http2_frame_deserialize(
    const co_byte_array_t* data, size_t* index,
    uint32_t max_frame_size, co_http2_frame_t* frame);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_http2_frame_t* co_http2_frame_create(void);
void co_http2_frame_destroy(co_http2_frame_t* frame);

co_http2_frame_t* co_http2_create_data_frame(
    bool payload_copy,
    bool end_stream,
    const uint8_t* data, uint32_t data_length,
    const uint8_t* padding, uint8_t pad_length);

co_http2_frame_t* co_http2_create_headers_frame(
    bool payload_copy,
    bool end_stream,
    bool end_headers,
    const uint8_t* header_block_fragment,
    uint32_t header_block_fragment_length,
    uint32_t stream_dependency, uint8_t weight,
    const uint8_t* padding, uint8_t pad_length);

co_http2_frame_t* co_http2_create_priority_frame(
    uint32_t stream_dependency, uint8_t weight);

co_http2_frame_t* co_http2_create_rst_stream_frame(
    uint32_t error_code);

co_http2_frame_t* co_http2_create_settings_frame(
    bool payload_copy, bool ack,
    const co_http2_setting_param_st* params, uint16_t param_count);

co_http2_frame_t* co_http2_create_push_promise_frame(
    bool payload_copy,
    bool end_headers,
    uint32_t promised_stream_id,
    const uint8_t* header_block_fragment,
    uint32_t header_block_fragment_length,
    const uint8_t* padding, uint8_t pad_length);

co_http2_frame_t* co_http2_create_ping_frame(
    bool ack, uint64_t opaque_data);

co_http2_frame_t* co_http2_create_goaway_frame(
    bool payload_copy,
    uint32_t last_stream_id, uint32_t error_code,
    const uint8_t* additional_debug_data,
    uint32_t additional_debug_data_length);

co_http2_frame_t* co_http2_create_window_update_frame(
    uint32_t window_size_increment);

co_http2_frame_t* co_http2_create_continuation_frame(
    bool payload_copy, bool end_headers,
    const uint8_t* header_block_fragment,
    uint32_t header_block_fragment_length);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_FRAME_H_INCLUDED
