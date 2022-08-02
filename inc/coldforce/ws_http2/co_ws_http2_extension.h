#ifndef CO_WS_HTTP2_EXTENSION_H_INCLUDED
#define CO_WS_HTTP2_EXTENSION_H_INCLUDED

#include <coldforce/http2/co_http2_stream.h>

#include <coldforce/ws/co_ws_frame.h>

#include <coldforce/ws_http2/co_ws_http2.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_WS_HTTP2_API
co_http2_header_t*
co_http2_header_create_ws_connect_request(
    const char* path,
    const char* protocols,
    const char* extensions
);

CO_WS_HTTP2_API
co_http2_header_t*
co_http2_header_create_ws_connect_response(
    const char* protocol,
    const char* extension
);

CO_WS_HTTP2_API
bool
co_http2_header_validate_ws_connect_request(
    const co_http2_stream_t* stream,
    const co_http2_header_t* header
);

CO_WS_HTTP2_API
bool
co_http2_header_validate_ws_connect_response(
    const co_http2_header_t* header
);

CO_WS_HTTP2_API
co_ws_frame_t*
co_http2_stream_receive_ws_frame(
    const co_http2_stream_t* stream,
    const co_http2_data_st* data
);

CO_WS_HTTP2_API
bool
co_http2_stream_send_ws_frame(
    co_http2_stream_t* stream,
    bool fin,
    uint8_t opcode,
    const void* data,
    size_t data_size
);

CO_WS_HTTP2_API
bool
co_http2_stream_send_ws_binary(
    co_http2_stream_t* stream,
    bool fin,
    const void* data,
    size_t data_size
);

CO_WS_HTTP2_API
bool
co_http2_stream_send_ws_text(
    co_http2_stream_t* stream,
    bool fin,
    const char* text
);

CO_WS_HTTP2_API
bool
co_http2_stream_send_ws_continuation(
    co_http2_stream_t* stream,
    bool fin,
    const void* data,
    size_t data_size
);

CO_WS_HTTP2_API
bool
co_http2_stream_send_ws_close(
    co_http2_stream_t* stream,
    uint16_t reason_code,
    const char* utf8_reason_str
);

CO_WS_HTTP2_API
bool
co_http2_stream_send_ws_ping(
    co_http2_stream_t* stream,
    const void* data,
    size_t data_size
);

CO_WS_HTTP2_API
bool
co_http2_stream_send_ws_pong(
    co_http2_stream_t* stream,
    const void* data,
    size_t data_size
);

CO_WS_HTTP2_API
void
co_http2_stream_ws_default_handler(
    co_http2_stream_t* stream,
    const co_ws_frame_t* frame
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_HTTP2_EXTENSION_H_INCLUDED
