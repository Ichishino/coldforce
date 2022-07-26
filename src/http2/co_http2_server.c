#include <coldforce/core/co_std.h>

#include <coldforce/http/co_base64.h>
#include <coldforce/http/co_http_log.h>

#include <coldforce/http2/co_http2_server.h>
#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_http_extension.h>
#include <coldforce/http2/co_http2_log.h>

//---------------------------------------------------------------------------//
// http2 server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static bool
co_http2_server_on_upgrade_request(
    co_http2_client_t* client
)
{
    size_t data_size =
        co_byte_array_get_count(client->conn.receive_data.ptr);

    if ((data_size - client->conn.receive_data.index) >=
        CO_HTTP2_CONNECTION_PREFACE_LENGTH)
    {
        const uint8_t* data =
            co_byte_array_get_const_ptr(
                client->conn.receive_data.ptr,
                client->conn.receive_data.index);

        if (memcmp(data,
            CO_HTTP2_CONNECTION_PREFACE,
            CO_HTTP2_CONNECTION_PREFACE_LENGTH) == 0)
        {
            co_http2_log_debug(
                &client->conn.tcp_client->sock.local_net_addr,
                "<--",
                &client->conn.tcp_client->remote_net_addr,
                "http2 receive connection preface");

            client->conn.receive_data.index +=
                CO_HTTP2_CONNECTION_PREFACE_LENGTH;

            co_http2_send_initial_settings(client);

            return true;
        }
    }

    co_http_request_t* request = co_http_request_create();

    if (co_http_request_deserialize(
        request, client->conn.receive_data.ptr,
        &client->conn.receive_data.index) !=
        CO_HTTP_PARSE_COMPLETE)
    {
        co_http_request_destroy(request);

        return false;
    }

    co_http_log_debug_request_header(
        &client->conn.tcp_client->sock.local_net_addr,
        "<--",
        &client->conn.tcp_client->remote_net_addr,
        request,
        "http receive request");

    const co_http_header_t* request_header =
        co_http_request_get_header(request);
    const char* upgrade =
        co_http_header_get_field(
            request_header, CO_HTTP_HEADER_UPGRADE);
    const char* http2_settings =
        co_http_header_get_field(
            request_header, CO_HTTP2_HEADER_SETTINGS);

    if ((upgrade == NULL) || (http2_settings == NULL))
    {
        co_http_request_destroy(request);

        return false;
    }

    co_http_request_destroy(request);

    co_http2_set_upgrade_settings(
        http2_settings, strlen(http2_settings),
        &client->remote_settings);

    co_http_response_t* response =
        co_http_response_create_http2_upgrade(101, "Switching Protocols");

    co_http_connection_send_response(
        (co_http_connection_t*)client, response);

    co_http_response_destroy(response);

    co_http2_send_initial_settings(client);

    return true;
}

void
co_http2_server_on_tcp_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    (void)thread;

    co_http2_client_t* client =
        (co_http2_client_t*)tcp_client->sock.sub_class;

    client->conn.module.receive_all(
        client->conn.tcp_client,
        client->conn.receive_data.ptr);

    size_t data_size =
        co_byte_array_get_count(client->conn.receive_data.ptr);

    if (data_size == 0)
    {
        return;
    }

    while (data_size > client->conn.receive_data.index)
    {
        co_http2_frame_t* frame = co_http2_frame_create();

        int result = co_http2_frame_deserialize(
            client->conn.receive_data.ptr,
            &client->conn.receive_data.index,
            client->local_settings.max_frame_size,
            frame);

        co_assert(data_size >= client->conn.receive_data.index);

        if (result == CO_HTTP_PARSE_COMPLETE)
        {
            co_http2_stream_t* stream =
                co_http2_get_stream(client, frame->header.stream_id);

            if (stream == NULL)
            {
                if (co_map_get_count(client->stream_map) >=
                    client->local_settings.max_concurrent_streams)
                {
                    co_http2_close(
                        client, CO_HTTP2_STREAM_ERROR_REFUSED_STREAM);
                    co_http2_client_on_close(
                        client, CO_HTTP2_ERROR_MAX_STREAMS);

                    return;
                }

                stream = co_http2_stream_create(
                    frame->header.stream_id, client,
                    client->callbacks.on_receive_start,
                    client->callbacks.on_receive_finish,
                    client->callbacks.on_receive_data);
                co_map_set(client->stream_map,
                    (void*)(uintptr_t)frame->header.stream_id, stream);

                if (client->last_stream_id < frame->header.stream_id)
                {
                    client->last_stream_id = frame->header.stream_id;
                }
            }

            co_http2_log_debug_frame(
                &client->conn.tcp_client->sock.local_net_addr, "<--",
                &client->conn.tcp_client->remote_net_addr,
                frame,
                "http2 receive frame");

            if (frame->header.type == CO_HTTP2_FRAME_TYPE_PUSH_PROMISE)
            {
                co_http2_close(
                    client, CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR);
                co_http2_client_on_close(
                    client, CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR);

                return;
            }

            if (frame->header.stream_id == 0)
            {
                co_http2_client_on_receive_system_frame(client, frame);
            }
            else
            {
                if (co_http2_stream_on_receive_frame(stream, frame))
                {
                    if (frame->header.type == CO_HTTP2_FRAME_TYPE_DATA)
                    {
                        co_http2_stream_update_local_window_size(
                            client->system_stream, frame->header.length);
                    }
                }

                if (stream->state == CO_HTTP2_STREAM_STATE_CLOSED)
                {
                    co_http2_destroy_stream(client, stream);
                }
            }

            co_http2_frame_destroy(frame);

            if (client->conn.tcp_client == NULL)
            {
                return;
            }
        }
        else if (result == CO_HTTP_PARSE_MORE_DATA)
        {
            co_http2_frame_destroy(frame);

            return;
        }
        else
        {
            co_http2_frame_destroy(frame);

            if (co_http2_server_on_upgrade_request(client))
            {
                continue;
            }

            co_http2_close(
                client, CO_HTTP2_STREAM_ERROR_FRAME_SIZE_ERROR);
            co_http2_client_on_close(
                client, CO_HTTP2_STREAM_ERROR_FRAME_SIZE_ERROR);

            return;
        }
    }

    client->conn.receive_data.index = 0;
    co_byte_array_clear(client->conn.receive_data.ptr);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http2_client_t*
co_http2_client_create_with(
    co_tcp_client_t* tcp_client
)
{
    co_http2_client_t* client =
        (co_http2_client_t*)co_mem_alloc(sizeof(co_http2_client_t));

    if (client == NULL)
    {
        return NULL;
    }

    client->conn.base_url = NULL;
    client->conn.tcp_client = tcp_client;

    co_http2_client_setup(client);

    client->conn.tcp_client->callbacks.on_receive =
        (co_tcp_receive_fn)co_http2_server_on_tcp_receive_ready;
    client->conn.tcp_client->callbacks.on_close =
        (co_tcp_close_fn)co_http2_client_on_tcp_close;

    return client;
}

bool
co_http2_send_ping(
    co_http2_client_t* client,
    uint64_t user_data
)
{
    co_http2_frame_t* ping_frame =
        co_http2_create_ping_frame(false, user_data);

    return co_http2_stream_send_frame(
        client->system_stream, ping_frame);
}
