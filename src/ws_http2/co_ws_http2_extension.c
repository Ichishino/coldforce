#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_byte_order.h>

#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_server.h>

#include <coldforce/ws/co_ws_http_extension.h>
#include <coldforce/ws/co_ws_log.h>

#include <coldforce/ws_http2/co_ws_http2_extension.h>

//---------------------------------------------------------------------------//
// http2 extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http2_header_t*
co_http2_header_create_ws_connect_request(
    const char* path,
    const char* protocols,
    const char* extensions
)
{
    co_http2_header_t* header =
        co_http2_header_create_request("CONNECT", path);
    co_http2_header_set_protocol(header, "websocket");

    co_http2_header_set_field(
        header, CO_HTTP2_HEADER_SEC_WS_VERSION, "13");

    if (protocols != NULL)
    {
        co_http2_header_set_field(
            header, CO_HTTP_HEADER_SEC_WS_PROTOCOL, protocols);
    }

    if (extensions != NULL)
    {
        co_http2_header_set_field(
            header, CO_HTTP_HEADER_SEC_WS_EXTENSIONS, extensions);
    }

    return header;
}

co_http2_header_t*
co_http2_header_create_ws_connect_response(
    const char* protocol,
    const char* extension
)
{
    co_http2_header_t* header =
        co_http2_header_create_response(200);

    if (protocol != NULL)
    {
        co_http2_header_set_field(
            header, CO_HTTP_HEADER_SEC_WS_PROTOCOL, protocol);
    }

    if (extension != NULL)
    {
        co_http2_header_set_field(
            header, CO_HTTP_HEADER_SEC_WS_EXTENSIONS, extension);
    }

    return header;
}

bool
co_http2_header_validate_ws_connect_request(
    const co_http2_stream_t* stream,
    const co_http2_header_t* header
)
{
    if (!stream->client->local_settings.enable_conncet_protocol)
    {
        return false;
    }

    const char* method =
        co_http2_header_get_method(header);

    if (method != NULL &&
        co_string_case_compare(method, "CONNECT") != 0)
    {
        return false;
    }

    const char* protocol =
        co_http2_header_get_protocol(header);

    if (protocol != NULL &&
        co_string_case_compare(protocol, "websocket") != 0)
    {
        return false;
    }

    const char* version =
        co_http2_header_get_field(
            header, CO_HTTP2_HEADER_SEC_WS_VERSION);

    if (version != NULL &&
        strcmp(version, "13") != 0)
    {
        return false;
    }

    return true;
}

bool
co_http2_header_validate_ws_connect_response(
    const co_http2_header_t* header
)
{
    if (co_http2_header_get_status_code(header) != 200)
    {
        return false;
    }

    return true;
}

co_ws_frame_t*
co_http2_stream_receive_ws_frame(
    const co_http2_stream_t* stream,
    const co_http2_data_st* data
)
{
    co_ws_frame_t* frame = co_ws_frame_create();

    size_t unused = 0;

    if (co_ws_frame_deserialize(
        frame, data->ptr, data->size, &unused) !=
        CO_WS_PARSE_COMPLETE)
    {
        co_ws_frame_destroy(frame);

        return NULL;
    }

    co_ws_log_debug_frame(
        &stream->client->conn.tcp_client->sock.local_net_addr,
        "<--",
        &stream->client->conn.tcp_client->remote_net_addr,
        frame->header.fin,
        frame->header.opcode,
        frame->payload_data,
        (size_t)frame->header.payload_size,
        "ws receive frame (over http2)");

    return frame;
}

bool
co_http2_stream_send_ws_frame(
    co_http2_stream_t* stream,
    bool fin,
    uint8_t opcode,
    const void* data,
    size_t data_size
)
{
    co_ws_log_debug_frame(
        &stream->client->conn.tcp_client->sock.local_net_addr,
        "-->",
        &stream->client->conn.tcp_client->remote_net_addr,
        fin, opcode, data, data_size,
        "ws send frame (over http2)");

    co_byte_array_t* buffer = co_byte_array_create();

    co_ws_frame_serialize(
        fin, opcode,
        !co_http_connection_is_server(&stream->client->conn),
        data, data_size,
        buffer);

    bool result =
        co_http2_stream_send_data(
            stream,
            true, co_byte_array_get_ptr(buffer, 0),
            (uint32_t)co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    return result;
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
        data, data_size);
}

bool
co_http2_stream_send_ws_close(
    co_http2_stream_t* stream,
    uint16_t reason_code,
    const char* utf8_reason_str
)
{
    if (co_http2_stream_get_protocol_data(stream) == 1)
    {
        return true;
    }

    co_http2_stream_set_protocol_data(stream, 1);

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
        data, data_size);
}

void
co_http2_stream_ws_default_handler(
    co_http2_stream_t* stream,
    const co_ws_frame_t* frame
)
{
    switch (frame->header.opcode)
    {
    case CO_WS_OPCODE_CLOSE:
    {
        uint16_t reason_code = CO_WS_CLOSE_REASON_NORMAL;

        if (frame->header.payload_size >= sizeof(uint16_t))
        {
            memcpy(&reason_code,
                frame->payload_data, sizeof(uint16_t));
            reason_code =
                co_byte_order_16_network_to_host(reason_code);
        }

        co_http2_stream_send_ws_close(stream, reason_code, NULL);

        co_http2_stream_set_protocol_mode(stream, NULL);

        break;
    }
    case CO_WS_OPCODE_PING:
    {
        co_http2_stream_send_ws_pong(stream,
            frame->payload_data, (size_t)frame->header.payload_size);

        break;
    }
    case CO_WS_OPCODE_PONG:
    {
        break;
    }
    default:
        break;
    }
}
