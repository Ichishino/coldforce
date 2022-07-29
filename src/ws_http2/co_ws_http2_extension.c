#include <coldforce/core/co_std.h>

#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_server.h>

#include <coldforce/ws/co_ws_http_extension.h>

#include <coldforce/ws_http2/co_ws_http2_extension.h>

//---------------------------------------------------------------------------//
// http2 extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

bool
co_http2_stream_send_ws_frame(
    co_http2_stream_t* stream,
    bool fin,
    uint8_t opcode,
    bool mask,
    const void* data,
    size_t data_size
)
{
    return co_http_connection_send_ws_frame(
        &stream->client->conn, fin, opcode, mask, data, data_size);
}

bool
co_http2_stream_send_ws_binary(
    co_http2_stream_t* stream,
    bool fin,
    const void* data,
    size_t data_size
)
{
    return co_http2_stream_send_ws_frame(
        stream, fin,
        CO_WS_OPCODE_BINARY,
        !co_http2_is_server(stream->client),
        data, data_size);
}

bool
co_http2_stream_send_ws_text(
    co_http2_stream_t* stream,
    bool fin,
    const char* text
)
{
    return co_http2_stream_send_ws_frame(
        stream, fin,
        CO_WS_OPCODE_TEXT,
        !co_http2_is_server(stream->client),
        text, strlen(text));
}

bool
co_http2_stream_send_ws_continuation(
    co_http2_stream_t* stream,
    bool fin,
    const void* data,
    size_t data_size
)
{
    return co_http2_stream_send_ws_frame(
        stream, fin,
        CO_WS_OPCODE_CONTINUATION,
        !co_http2_is_server(stream->client),
        data, data_size);
}

bool
co_http2_stream_send_ws_close(
    co_http2_stream_t* stream,
    uint16_t reason_code,
    const char* utf8_reason_str
)
{
    size_t data_size = sizeof(reason_code);

    if (utf8_reason_str != NULL)
    {
        data_size += strlen(utf8_reason_str);
    }

    uint8_t* data =
        (uint8_t*)co_mem_alloc(data_size + 1);

    if (data == NULL)
    {
        return false;
    }

    data[0] = (uint8_t)((reason_code & 0xff00) >> 8);
    data[1] = (uint8_t)(reason_code & 0x00ff);

    if (utf8_reason_str != NULL)
    {
        strcpy((char*)&data[2], utf8_reason_str);
    }

    bool result =
        co_http2_stream_send_ws_frame(
            stream, true,
            CO_WS_OPCODE_CLOSE,
            !co_http2_is_server(stream->client),
            data, data_size);

    co_mem_free(data);

    return result;
}

bool
co_http2_stream_send_ws_ping(
    co_http2_stream_t* stream,
    const void* data,
    size_t data_size
)
{
    return co_http2_stream_send_ws_frame(
        stream, true,
        CO_WS_OPCODE_PING,
        !co_http2_is_server(stream->client),
        data, data_size);
}

bool
co_http2_stream_send_ws_pong(
    co_http2_stream_t* stream,
    const void* data,
    size_t data_size
)
{
    return co_http2_stream_send_ws_frame(
        stream, true,
        CO_WS_OPCODE_PONG,
        !co_http2_is_server(stream->client),
        data, data_size);
}
