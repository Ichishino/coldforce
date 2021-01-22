#include <coldforce/core/co_std.h>

#include <coldforce/http/co_base64.h>

#include <coldforce/http2/co_http2_server.h>
#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_stream.h>

//---------------------------------------------------------------------------//
// http2 server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static bool
co_http2_server_on_upgrade_request(
    co_http2_client_t* client
)
{
    size_t data_size =
        co_byte_array_get_count(client->receive_data);

    if ((data_size - client->receive_data_index) >=
        CO_HTTP2_CONNECTION_PREFACE_LENGTH)
    {
        const uint8_t* data =
            co_byte_array_get_const_ptr(
                client->receive_data,
                client->receive_data_index);

        if (memcmp(data,
            CO_HTTP2_CONNECTION_PREFACE,
            CO_HTTP2_CONNECTION_PREFACE_LENGTH) == 0)
        {
            client->receive_data_index +=
                CO_HTTP2_CONNECTION_PREFACE_LENGTH;

            co_http2_send_initial_settings(client);

            return true;
        }
    }

    if (client->tcp_client->sock.tls != NULL)
    {
        return false;
    }

    co_http_request_t* request = co_http_request_create();

    if (co_http_request_deserialize(
        request, client->receive_data,
        &client->receive_data_index) !=
        CO_HTTP_PARSE_COMPLETE)
    {
        co_http_request_destroy(request);

        return false;
    }

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

    bool result = co_http2_set_upgrade_settings(
        http2_settings, strlen(http2_settings),
        &client->remote_settings);

    co_http2_send_upgrade_response(client, result);

    co_http_request_destroy(request);

    if (result)
    {
        co_http2_send_initial_settings(client);
    }

    return result;
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

    client->module.receive_all(
        client->tcp_client, client->receive_data);

    size_t data_size =
        co_byte_array_get_count(client->receive_data);

    if (data_size == 0)
    {
        return;
    }

    while (data_size > client->receive_data_index)
    {
        co_http2_frame_t* frame = co_http2_frame_create();

        int result = co_http2_frame_deserialize(
            client->receive_data, &client->receive_data_index,
            client->local_settings.max_frame_size, frame);

        co_assert(data_size >= client->receive_data_index);

        if (result == CO_HTTP_PARSE_COMPLETE)
        {
            co_http2_stream_t* stream =
                co_http2_get_stream(client, frame->header.stream_id);

            if (stream == NULL)
            {
                if (co_map_get_count(client->stream_map) >=
                    client->local_settings.max_concurrent_streams)
                {
                    co_http2_client_close(
                        client, CO_HTTP2_STREAM_ERROR_REFUSED_STREAM);
                    co_http2_client_on_close(
                        client, CO_HTTP2_ERROR_MAX_STREAMS);

                    return;
                }

                stream = co_http2_stream_create(
                    frame->header.stream_id, client, client->on_message);
                co_map_set(client->stream_map,
                    frame->header.stream_id, (uintptr_t)stream);

                if (client->last_stream_id < frame->header.stream_id)
                {
                    client->last_stream_id = frame->header.stream_id;
                }
            }

#ifdef CO_HTTP2_DEBUG
            co_http2_stream_frame_trace(stream, false, frame);
#endif
            if (frame->header.type == CO_HTTP2_FRAME_TYPE_PUSH_PROMISE)
            {
                co_http2_client_close(
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
            }

            co_http2_frame_destroy(frame);

            if (client->tcp_client == NULL)
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

            co_http2_client_close(
                client, CO_HTTP2_STREAM_ERROR_FRAME_SIZE_ERROR);
            co_http2_client_on_close(
                client, CO_HTTP2_STREAM_ERROR_FRAME_SIZE_ERROR);

            return;
        }
    }

    client->receive_data_index = 0;
    co_byte_array_clear(client->receive_data);
}

//---------------------------------------------------------------------------//
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

    client->base_url = NULL;
    client->tcp_client = tcp_client;

    co_http2_client_setup(client);

    co_tcp_set_receive_handler(
        client->tcp_client,
        (co_tcp_receive_fn)co_http2_server_on_tcp_receive_ready);
    co_tcp_set_close_handler(
        client->tcp_client,
        (co_tcp_close_fn)co_http2_client_on_tcp_close);

    return client;
}

void
co_http2_set_priority_handler(
    co_http2_client_t* client,
    co_http2_priority_fn handler
)
{
    client->on_priority = handler;
}

bool
co_http2_send_ping(
    co_http2_client_t* client,
    uint64_t user_data,
    co_http2_ping_fn handler
)
{
    client->on_ping = handler;

    co_http2_frame_t* ping_frame =
        co_http2_create_ping_frame(false, user_data);

    return co_http2_stream_send_frame(
        client->system_stream, ping_frame);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_set_http2_upgrade_request_handler(
    co_http_client_t* client,
    co_http_upgrade_request_fn handler
)
{
    co_http_set_upgrade_handler(
        client, CO_HTTP_UPGRADE_CONNECTION_PREFACE, (void*)handler);
    co_http_set_upgrade_handler(
        client, CO_HTTP2_UPGRADE, (void*)handler);
}

