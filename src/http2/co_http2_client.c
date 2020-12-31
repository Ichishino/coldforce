#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_net_addr_resolve.h>

#include <coldforce/tls/co_tls_tcp_client.h>

#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_frame.h>

//---------------------------------------------------------------------------//
// http2 client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http2_client_setup(
    co_http2_client_t* client,
    bool secure
)
{
    if (secure)
    {
        client->module.destroy = co_tls_tcp_client_destroy;
        client->module.close = co_tls_tcp_client_close;
        client->module.connect_async = co_tls_tcp_connect_async;
        client->module.send = co_tls_tcp_send;
        client->module.receive_all = co_tls_tcp_receive_all;
    }
    else
    {
        client->module.destroy = co_tcp_client_destroy;
        client->module.close = co_tcp_client_close;
        client->module.connect_async = co_tcp_connect_async;
        client->module.send = co_tcp_send;
        client->module.receive_all = co_tcp_receive_all;
    }

    client->tcp_client->sock.sub_class = client;

    client->connecting = false;
    client->request_queue = co_list_create(NULL);
    client->receive_data = co_byte_array_create();

    client->on_close = NULL;
    client->on_message = NULL;
    client->on_push_request = NULL;
    client->on_push_response = NULL;

    co_map_ctx_st map_ctx = { 0 };
    map_ctx.free_value = (co_free_fn)co_http2_stream_destroy;
    client->stream_map = co_map_create(&map_ctx);

    client->last_stream_id = 0;

    client->local_settings.header_table_size = 4096;
    client->local_settings.enable_push = 1;
    client->local_settings.max_concurrent_streams = INT32_MAX;
    client->local_settings.initial_window_size = 65535;
    client->local_settings.max_frame_size = 16384;
    client->local_settings.max_header_list_size = INT32_MAX;

    client->remote_settings.header_table_size = 4096;
    client->remote_settings.enable_push = 1;
    client->remote_settings.max_concurrent_streams = INT32_MAX;
    client->remote_settings.initial_window_size = 65535;
    client->remote_settings.max_frame_size = 16384;
    client->remote_settings.max_header_list_size = INT32_MAX;

    co_http2_hpack_dynamic_table_setup(
        &client->local_dynamic_table,
        client->local_settings.max_header_list_size);
    co_http2_hpack_dynamic_table_setup(
        &client->remote_dynamic_table,
        client->remote_settings.max_header_list_size);

    client->system_stream = co_http2_stream_create(0, client, NULL);
}

void
co_http2_client_cleanup(
    co_http2_client_t* client
)
{
    if (client != NULL)
    {
        co_list_iterator_t* req_it =
            co_list_get_head_iterator(client->request_queue);

        while (req_it != NULL)
        {
            co_http2_message_destroy(
                (co_http2_message_t*)co_list_get_next(
                    client->request_queue, &req_it)->value);
        }

        co_list_destroy(client->request_queue);
        client->request_queue = NULL;

        co_byte_array_destroy(client->receive_data);
        client->receive_data = NULL;

        co_http2_stream_destroy(client->system_stream);
        client->system_stream = NULL;

        co_map_destroy(client->stream_map);
        client->stream_map = NULL;

        co_http2_hpack_dynamic_table_cleanup(&client->local_dynamic_table);
        co_http2_hpack_dynamic_table_cleanup(&client->remote_dynamic_table);
    }
}

void
co_http2_client_clear_closed_stream(
    co_http2_client_t* client
)
{
    co_map_iterator_t it;
    co_map_iterator_init(client->stream_map, &it);

    while (co_map_iterator_has_next(&it))
    {
        const co_map_data_st* data = &it.item->data;
        co_http2_stream_t* stream = (co_http2_stream_t*)data->value;
        co_map_iterator_get_next(&it);

        if (stream->state == CO_HTTP2_STREAM_STATE_CLOSED)
        {
            co_map_remove(client->stream_map, (uintptr_t)stream->id);
        }
    }
}

co_http2_stream_t*
co_http2_client_create_request_stream(
    co_http2_client_t* client
)
{
    co_http2_client_clear_closed_stream(client);

    if (client->last_stream_id == 0)
    {
        client->last_stream_id = 1;
    }
    else
    {
        client->last_stream_id += 2;
    }

    co_http2_stream_t* stream =
        co_http2_stream_create(
            client->last_stream_id, client, client->on_message);

    co_map_set(client->stream_map,
        client->last_stream_id, (uintptr_t)stream);

    return stream;
}

co_http2_stream_t*
co_http2_client_get_stream(
    co_http2_client_t* client,
    uint32_t stream_id
)
{
    co_http2_stream_t* stream = NULL;

    if (stream_id != 0)
    {
        co_map_data_st* data =
            co_map_get(client->stream_map, (uintptr_t)stream_id);

        if (data != NULL)
        {
            stream = (co_http2_stream_t*)data->value;
        }
    }
    else
    {
        stream = client->system_stream;
    }

    return stream;
}

bool
co_http2_send_raw_data(
    co_http2_client_t* client,
    const void* data,
    size_t data_size
)
{
    return client->module.send(
        client->tcp_client, data, data_size);
}

void
co_http2_client_send_initial_settings(
    co_http2_client_t* client
)
{
    co_http2_send_raw_data(client,
        CO_HTTP2_CONNECTION_PREFACE,
        CO_HTTP2_CONNECTION_PREFACE_LENGTH);

    co_http2_setting_param_t params[6];

    params[0].identifier = CO_HTTP2_SETTINGS_HEADER_TABLE_SIZE;
    params[0].value = client->local_settings.header_table_size;

    params[1].identifier = CO_HTTP2_SETTINGS_ENABLE_PUSH;
    params[1].value = client->local_settings.enable_push;

    params[2].identifier = CO_HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS;
    params[2].value = client->local_settings.max_concurrent_streams;
    
    params[3].identifier = CO_HTTP2_SETTINGS_INITIAL_WINDOW_SIZE;
    params[3].value = client->local_settings.initial_window_size;

    params[4].identifier = CO_HTTP2_SETTINGS_MAX_FRAME_SIZE;
    params[4].value = client->local_settings.max_frame_size;

    params[5].identifier = CO_HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE;
    params[5].value = client->local_settings.max_header_list_size;
    
    co_http2_frame_t* frame =
        co_http2_create_settings_frame(false, params, 6);

    co_http2_stream_send_frame(
        client->system_stream, frame);
}

void
co_http2_client_send_all_requests(
    co_http2_client_t* client
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(client->request_queue);

    while (it != NULL)
    {
        co_list_data_st* data = co_list_get(client->request_queue, it);
        co_http2_message_t* message = (co_http2_message_t*)data->value;

        co_http2_stream_t* stream =
            co_http2_client_create_request_stream(client);

        bool result =
            co_http2_stream_send_request_message(stream, message);

        if (result)
        {
            co_list_iterator_t* temp = it;
            it = co_list_get_next_iterator(client->request_queue, it);
            co_list_remove_at(client->request_queue, temp);
        }
        else
        {
            break;
        }
    }
}

void
co_http2_client_on_close(
    co_http2_client_t* client,
    int error_code
)
{
    client->module.close(client->tcp_client);

    co_http2_close_fn close_handler = client->on_close;

    if (close_handler != NULL)
    {
        close_handler(
            client->tcp_client->sock.owner_thread,
            client, error_code);
    }
}

void
co_http2_client_on_receive_system_frame(
    co_http2_client_t* client,
    const co_http2_frame_t* frame
)
{
    switch (frame->header.type)
    {
    case CO_HTTP2_FRAME_TYPE_SETTINGS:
    {
        if (frame->header.flags != 0)
        {
            co_http2_client_send_all_requests(client);

            return;
        }

        for (size_t index = 0;
            index < frame->payload.settings.param_count;
            ++index)
        {
            const co_http2_setting_param_t* param =
                &frame->payload.settings.params[index];

            switch (param->identifier)
            {
            case CO_HTTP2_SETTINGS_HEADER_TABLE_SIZE:
            {
                client->remote_settings.header_table_size =
                    param->value;

                break;
            }
            case CO_HTTP2_SETTINGS_ENABLE_PUSH:
            {
                client->remote_settings.enable_push =
                    param->value;

                break;
            }
            case CO_HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
            {
                client->remote_settings.max_concurrent_streams =
                    param->value;

                break;
            }
            case CO_HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
            {
                client->remote_settings.initial_window_size =
                    param->value;

                break;
            }
            case CO_HTTP2_SETTINGS_MAX_FRAME_SIZE:
            {
                client->remote_settings.max_frame_size =
                    param->value;

                break;
            }
            case CO_HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
            {
                client->remote_settings.max_header_list_size =
                    param->value;

                break;
            }
            default:
                break;
            }
        }

        co_http2_frame_t* ack_frame =
            co_http2_create_settings_frame(true, NULL, 0);

        co_http2_stream_send_frame(
            client->system_stream, ack_frame);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PING:
    {
        co_http2_frame_t* ack_frame =
            co_http2_create_ping_frame(true,
                frame->payload.ping.opaque_data, 8);

        co_http2_stream_send_frame(
            client->system_stream, ack_frame);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_GOAWAY:
    {
        co_http2_client_on_close(
            (co_http2_client_t*)client->tcp_client->sock.sub_class,
            CO_HTTP2_ERROR_STREAM_CLOSED - frame->payload.goaway.error_code);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE:
    {
        client->system_stream->remote_window_size =
            frame->payload.window_update.window_size_increment;

        break;
    }
    default:
    {
        co_http2_frame_t* goaway_frame =
            co_http2_create_goaway_frame(
                client->last_stream_id,
                CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR,
                NULL, 0);

        co_http2_stream_send_frame(
            client->system_stream, goaway_frame);

        break;
    }
    }
}

void
co_http2_client_on_push_promise_message(
    co_http2_client_t* client,
    co_http2_stream_t* stream,
    uint32_t promised_id,
    co_http2_message_t* message
)
{
    co_http2_stream_t* promised_stream =
        co_http2_client_get_stream(client, promised_id);

    if (stream == NULL)
    {
        co_http2_message_destroy(message);

        return;
    }

#ifdef CO_HTTP2_DEBUG
    printf("[CO_HTTP2] <INF> ==== PUSH-PROMISE REQUEST ===\n");
    co_http2_header_print(&message->header);
    printf("[CO_HTTP2] <INF> =============================\n");
#endif

    promised_stream->send_message = message;
    promised_stream->state = CO_HTTP2_STREAM_STATE_RESERVED_REMOTE;

    co_http2_push_request_fn handler = client->on_push_request;

    if (handler != NULL)
    {
        if (!handler(
            client->tcp_client->sock.owner_thread,
            client, stream, message))
        {
            co_http2_frame_t* frame =
                co_http2_create_rst_stream_frame(
                    CO_HTTP2_STREAM_ERROR_CANCEL);

            co_http2_stream_send_frame(
                client->system_stream, frame);
        }
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http2_client_on_tcp_connect(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    co_http2_client_t* client =
        (co_http2_client_t*)tcp_client->sock.sub_class;

    client->connecting = false;

    if (error_code == 0)
    {
        co_http2_client_send_initial_settings(client);
    }
    else
    {
        co_http2_message_fn handler = client->on_message;

        if (handler != NULL)
        {
            handler(thread, client,
                NULL, NULL, CO_HTTP_ERROR_CONNECT_FAILED);
        }
    }
}

void
co_http2_client_on_client_tcp_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    (void)thread;

    co_http2_client_t* client =
        (co_http2_client_t*)tcp_client->sock.sub_class;

    ssize_t receive_result = client->module.receive_all(
        client->tcp_client, client->receive_data);

    if (receive_result <= 0)
    {
        return;
    }

    size_t index = 0;
    size_t data_size = co_byte_array_get_count(client->receive_data);

    while (data_size > index)
    {
        co_http2_frame_t* frame = co_http2_frame_create();

        int result = co_http2_frame_deserialize(
            client->receive_data, &index,
            client->local_settings.max_frame_size, frame);

        co_assert(data_size >= index);

        if (result == CO_HTTP_PARSE_COMPLETE)
        {
            co_http2_stream_t* stream =
                co_http2_client_get_stream(client, frame->header.stream_id);

            if ((stream == NULL) ||
                (stream->state == CO_HTTP2_STREAM_STATE_CLOSED))
            {
                co_http2_frame_destroy(frame);

                continue;
            }

#ifdef CO_HTTP2_DEBUG
            co_http2_stream_frame_trace(stream, false, frame);
#endif
            if (frame->header.type == CO_HTTP2_FRAME_TYPE_PUSH_PROMISE)
            {
                if (co_http2_client_get_stream(
                    client, frame->payload.push_promise.promised_stream_id) != NULL)
                {
                    co_http2_frame_destroy(frame);

                    continue;
                }

                co_http2_client_clear_closed_stream(client);

                co_http2_stream_t* promised_stream =
                    co_http2_stream_create(
                        frame->payload.push_promise.promised_stream_id,
                        client, client->on_push_response);

                co_map_set(client->stream_map,
                    frame->payload.push_promise.promised_stream_id,
                    (uintptr_t)promised_stream);
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
        }
        else if (result == CO_HTTP_PARSE_MORE_DATA)
        {
            co_http2_frame_destroy(frame);

            return;
        }
        else
        {
            co_http2_frame_destroy(frame);

            co_http2_client_on_close(
                client, CO_HTTP2_ERROR_PARSE_ERROR);

            return;
        }
    }

    co_byte_array_clear(client->receive_data);
}

void
co_http2_client_on_client_tcp_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    (void)thread;

    co_http2_client_t* client =
        (co_http2_client_t*)tcp_client->sock.sub_class;

    co_http2_client_on_close(client, 0);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_http2_client_t*
co_http2_client_create(
    const char* base_url,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_http2_client_t* client =
        (co_http2_client_t*)co_mem_alloc(sizeof(co_http2_client_t));

    if (client == NULL)
    {
        return NULL;
    }

    client->base_url = co_http_url_create(base_url);

    if (client->base_url->host == NULL)
    {
        co_http_url_destroy(client->base_url);
        co_mem_free(client);

        return NULL;
    }

    if (client->base_url->scheme == NULL)
    {
        client->base_url->scheme = co_string_duplicate("http");
    }

    bool secure = false;

    if (co_string_case_compare(
        client->base_url->scheme, "https") == 0)
    {
        secure = true;
    }

    int address_family =
        co_net_addr_get_family(local_net_addr);

    co_net_addr_t remote_net_addr = CO_NET_ADDR_INIT;

    if (!co_net_addr_set_address(
        &remote_net_addr, client->base_url->host))
    {
        co_resolve_hint_st hint = { 0 };
        hint.family = address_family;

        if (co_net_addr_resolve_service(
            client->base_url->host, client->base_url->scheme,
            &hint, &remote_net_addr, 1) == 0)
        {
            co_http_url_destroy(client->base_url);
            co_mem_free(client);

            return NULL;
        }
    }
    else
    {
        uint16_t port = client->base_url->port;

        if (port == 0)
        {
            if (secure)
            {
                port = 443;
            }
            else
            {
                port = 80;
            }
        }

        co_net_addr_set_port(
            &remote_net_addr, port);
    }

    if (secure)
    {
        client->tcp_client =
            co_tls_tcp_client_create(local_net_addr, tls_ctx);

        if (client->tcp_client != NULL)
        {
            co_tls_tcp_client_set_host_name(
                client->tcp_client, client->base_url->host);

            const char* protocol = CO_HTTP2_ALPN_NAME;

            co_tls_tcp_client_set_alpn_protocols(
                client->tcp_client, &protocol, 1);
        }
    }
    else
    {
        client->tcp_client =
            co_tcp_client_create(local_net_addr);
    }

    if (client->tcp_client == NULL)
    {
        co_http_url_destroy(client->base_url);
        co_mem_free(client);

        return NULL;
    }

    memcpy(&client->tcp_client->remote_net_addr,
        &remote_net_addr, sizeof(co_net_addr_t));

    co_http2_client_setup(client, secure);

    co_tcp_set_receive_handler(
        client->tcp_client,
        (co_tcp_receive_fn)co_http2_client_on_client_tcp_receive_ready);
    co_tcp_set_close_handler(
        client->tcp_client,
        (co_tcp_close_fn)co_http2_client_on_client_tcp_close);

    return client;
}

void
co_http2_client_destroy(
    co_http2_client_t* client
)
{
    if (client != NULL)
    {
        co_http2_client_cleanup(client);

        co_http_url_destroy(client->base_url);
        client->base_url = NULL;

        if (client->tcp_client != NULL)
        {
            client->module.destroy(client->tcp_client);
            client->tcp_client = NULL;

            co_mem_free_later(client);
        }
    }
}

bool
co_http2_client_is_running(
    const co_http2_client_t* client
)
{
    co_map_iterator_t it;
    co_map_iterator_init(client->stream_map, &it);

    while (co_map_iterator_has_next(&it))
    {
        const co_map_data_st* data = &it.item->data;
        co_http2_stream_t* stream = (co_http2_stream_t*)data->value;
        co_map_iterator_get_next(&it);

        if ((stream->state != CO_HTTP2_STREAM_STATE_CLOSED) &&
            (stream->state != CO_HTTP2_STREAM_STATE_REMOTE_CLOSED))
        {
            return true;
        }
    }

    return false;
}

bool
co_http2_send_request(
    co_http2_client_t* client,
    co_http2_message_t* message
)
{
    if (co_tcp_is_open(client->tcp_client))
    {
        co_list_add_tail(client->request_queue, (uintptr_t)message);

        co_http2_client_send_all_requests(client);
    }
    else if (client->connecting)
    {
        co_list_add_tail(client->request_queue, (uintptr_t)message);
    }
    else
    {
        if (!client->module.connect_async(
            client->tcp_client,
            &client->tcp_client->remote_net_addr,
            (co_tcp_connect_fn)co_http2_client_on_tcp_connect))
        {
            return false;
        }

        client->connecting = true;

        co_list_add_tail(client->request_queue, (uintptr_t)message);
    }

    return true;
}

void
co_http2_set_message_handler(
    co_http2_client_t* client,
    co_http2_message_fn handler
)
{
    client->on_message = handler;
}

void
co_http2_set_close_handler(
    co_http2_client_t* client,
    co_http2_close_fn handler
)
{
    client->on_close = handler;
}

void
co_http2_set_server_push_request_handler(
    co_http2_client_t* client,
    co_http2_push_request_fn handler
)
{
    client->on_push_request = handler;
}

void
co_http2_set_server_push_response_handler(
    co_http2_client_t* client,
    co_http2_message_fn handler
)
{
    client->on_push_response = handler;
}

const co_net_addr_t*
co_http2_get_remote_net_addr(
    const co_http2_client_t* client
)
{
    return &client->tcp_client->remote_net_addr;
}

co_socket_t*
co_http2_client_get_socket(
    co_http2_client_t* client
)
{
    return &client->tcp_client->sock;
}

const char*
co_http2_get_base_url(
    const co_http2_client_t* client
)
{
    return client->base_url->src;
}

bool
co_http2_is_open(
    const co_http2_client_t* client
)
{
    return co_tcp_is_open(client->tcp_client);
}
