#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_net_addr_resolve.h>
#include <coldforce/net/co_byte_order.h>

#include <coldforce/tls/co_tls_client.h>

#include <coldforce/http/co_base64.h>
#include <coldforce/http/co_http_server.h>

#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_frame.h>
#include <coldforce/http2/co_http2_log.h>

//---------------------------------------------------------------------------//
// http2 client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void co_http2_server_on_tcp_receive_ready(
    co_thread_t* thread, co_tcp_client_t* tcp_client);

void
co_http2_client_setup(
    co_http2_client_t* client
)
{
#ifdef CO_CAN_USE_TLS
    if (client->tcp_client->sock.tls != NULL)
    {
        client->module.destroy = co_tls_client_destroy;
        client->module.close = co_tls_client_close;
        client->module.connect = co_tls_connect;
        client->module.send = co_tls_send;
        client->module.receive_all = co_tls_receive_all;
    }
    else
    {
#endif
        client->module.destroy = co_tcp_client_destroy;
        client->module.close = co_tcp_client_close;
        client->module.connect = co_tcp_connect;
        client->module.send = co_tcp_send;
        client->module.receive_all = co_tcp_receive_all;

#ifdef CO_CAN_USE_TLS
    }
#endif

    client->tcp_client->sock.sub_class = client;

    client->receive_data_index = 0;
    client->receive_data = co_byte_array_create();

    client->upgrade_request_data = NULL;

    client->callbacks.on_connect = NULL;
    client->callbacks.on_upgrade = NULL;
    client->callbacks.on_close = NULL;
    client->callbacks.on_receive_start = NULL;
    client->callbacks.on_receive_finish = NULL;
    client->callbacks.on_receive_data = NULL;
    client->callbacks.on_push_request = NULL;
    client->callbacks.on_push_start = NULL;
    client->callbacks.on_push_finish = NULL;
    client->callbacks.on_push_data = NULL;
    client->callbacks.on_priority = NULL;
    client->callbacks.on_window_update = NULL;
    client->callbacks.on_close_stream = NULL;
    client->callbacks.on_ping = NULL;

    co_map_ctx_st map_ctx = { 0 };
    map_ctx.free_value = (co_item_free_fn)co_http2_stream_destroy;
    client->stream_map = co_map_create(&map_ctx);

    client->last_stream_id = 0;
    client->new_stream_id = 0;

    client->local_settings.header_table_size =
        CO_HTTP2_SETTING_DEFAULT_HEADER_TABLE_SIZE;
    client->local_settings.enable_push =
        CO_HTTP2_SETTING_DEFAULT_ENABLE_PUSH;
    client->local_settings.max_concurrent_streams =
        CO_HTTP2_SETTING_DEFAULT_MAX_CONCURRENT_STREAMS;
    client->local_settings.initial_window_size =
        CO_HTTP2_SETTING_DEFAULT_INITIAL_WINDOW_SIZE;
    client->local_settings.max_frame_size =
        CO_HTTP2_SETTING_DEFAULT_MAX_FRAME_SIZE;
    client->local_settings.max_header_list_size =
        CO_HTTP2_SETTING_DEFAULT_MAX_HEADER_LIST_SIZE;

    client->remote_settings.header_table_size =
        CO_HTTP2_SETTING_DEFAULT_HEADER_TABLE_SIZE;
    client->remote_settings.enable_push =
        CO_HTTP2_SETTING_DEFAULT_ENABLE_PUSH;
    client->remote_settings.max_concurrent_streams =
        CO_HTTP2_SETTING_DEFAULT_MAX_CONCURRENT_STREAMS;
    client->remote_settings.initial_window_size =
        CO_HTTP2_SETTING_DEFAULT_INITIAL_WINDOW_SIZE;
    client->remote_settings.max_frame_size =
        CO_HTTP2_SETTING_DEFAULT_MAX_FRAME_SIZE;
    client->remote_settings.max_header_list_size =
        CO_HTTP2_SETTING_DEFAULT_MAX_HEADER_LIST_SIZE;

    co_http2_hpack_dynamic_table_setup(
        &client->local_dynamic_table,
        client->local_settings.max_header_list_size);
    co_http2_hpack_dynamic_table_setup(
        &client->remote_dynamic_table,
        client->remote_settings.max_header_list_size);

    client->system_stream = co_http2_stream_create(0, client, NULL, NULL, NULL);
}

void
co_http2_client_cleanup(
    co_http2_client_t* client
)
{
    if (client != NULL)
    {
        co_byte_array_destroy(client->receive_data);
        client->receive_data = NULL;

        co_byte_array_destroy(client->upgrade_request_data);
        client->upgrade_request_data = NULL;

        co_http2_stream_destroy(client->system_stream);
        client->system_stream = NULL;

        co_map_destroy(client->stream_map);
        client->stream_map = NULL;

        co_http2_hpack_dynamic_table_cleanup(&client->local_dynamic_table);
        co_http2_hpack_dynamic_table_cleanup(&client->remote_dynamic_table);
    }
}

static void
co_http2_set_setting_param(
    co_http2_settings_st* settings,
    uint16_t identifier,
    uint32_t value
)
{
    switch (identifier)
    {
    case CO_HTTP2_SETTING_ID_HEADER_TABLE_SIZE:
    {
        settings->header_table_size = value;

        break;
    }
    case CO_HTTP2_SETTING_ID_ENABLE_PUSH:
    {
        settings->enable_push = value;

        break;
    }
    case CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS:
    {
        settings->max_concurrent_streams = value;

        break;
    }
    case CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE:
    {
        settings->initial_window_size = value;

        break;
    }
    case CO_HTTP2_SETTING_ID_MAX_FRAME_SIZE:
    {
        settings->max_frame_size = value;

        break;
    }
    case CO_HTTP2_SETTING_ID_MAX_HEADER_LIST_SIZE:
    {
        settings->max_header_list_size = value;

        break;
    }
    default:
        break;
    }
}

co_http2_stream_t*
co_http2_create_stream(
    co_http2_client_t* client
)
{
    client->new_stream_id += 2;

    co_http2_stream_t* stream = co_http2_stream_create(
        client->new_stream_id, client,
        client->callbacks.on_receive_start,
        client->callbacks.on_receive_finish,
        client->callbacks.on_receive_data);

    co_map_set(client->stream_map,
        client->new_stream_id, (uintptr_t)stream);

    return stream;
}

void
co_http2_destroy_stream(
    co_http2_client_t* client,
    co_http2_stream_t* stream
)
{
    co_map_remove(client->stream_map, stream->id);
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
co_http2_client_on_close(
    co_http2_client_t* client,
    int error_code
)
{
    client->module.close(client->tcp_client);

    if (client->callbacks.on_close != NULL)
    {
        client->callbacks.on_close(
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
            if (client->callbacks.on_connect != NULL)
            {
                co_http2_connect_fn handler = client->callbacks.on_connect;
                client->callbacks.on_connect = NULL;

                handler(
                    client->tcp_client->sock.owner_thread,
                    client, 0);
            }

            return;
        }

        for (size_t index = 0;
            index < frame->payload.settings.param_count;
            ++index)
        {
            co_http2_set_setting_param(
                &client->remote_settings,
                frame->payload.settings.params[index].id,
                frame->payload.settings.params[index].value);
        }

        co_http2_frame_t* ack_frame =
            co_http2_create_settings_frame(false, true, NULL, 0);

        co_http2_stream_send_frame(
            client->system_stream, ack_frame);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PING:
    {
        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_ACK)
        {
            if (client->callbacks.on_ping != NULL)
            {
                client->callbacks.on_ping(
                    client->tcp_client->sock.owner_thread,
                    client, frame->payload.ping.opaque_data);
            }
        }
        else
        {
            co_http2_frame_t* ack_frame =
                co_http2_create_ping_frame(true,
                    frame->payload.ping.opaque_data);

            co_http2_stream_send_frame(
                client->system_stream, ack_frame);
        }

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
        if ((client->system_stream->remote_window_size +
            frame->payload.window_update.window_size_increment) >
            CO_HTTP2_SETTING_MAX_WINDOW_SIZE)
        {
            co_http2_client_close(
                client, CO_HTTP2_STREAM_ERROR_FLOW_CONTROL_ERROR);
            co_http2_client_on_close(
                client, CO_HTTP2_STREAM_ERROR_FLOW_CONTROL_ERROR);

            break;
        }

        client->system_stream->remote_window_size +=
            frame->payload.window_update.window_size_increment;

        if (client->callbacks.on_window_update != NULL)
        {
            client->callbacks.on_window_update(
                client->tcp_client->sock.owner_thread,
                client, client->system_stream);
        }

        break;
    }
    default:
    {
        break;
    }
    }
}

void
co_http2_client_on_push_promise(
    co_http2_client_t* client,
    co_http2_stream_t* stream,
    uint32_t promised_id,
    co_http2_header_t* header
)
{
    co_http2_stream_t* promised_stream =
        co_http2_stream_create(
            promised_id, client,
            client->callbacks.on_push_start,
            client->callbacks.on_push_finish,
            client->callbacks.on_push_data);

    co_map_set(client->stream_map,
        promised_id, (uintptr_t)promised_stream);

    if (client->last_stream_id < promised_id)
    {
        client->last_stream_id = promised_id;
    }

    promised_stream->send_header = header;
    promised_stream->state = CO_HTTP2_STREAM_STATE_RESERVED_REMOTE;

    if (client->callbacks.on_push_request != NULL)
    {
        if (!client->callbacks.on_push_request(
            client->tcp_client->sock.owner_thread,
            client, stream, promised_stream, header))
        {
            co_http2_stream_send_rst_stream(
                promised_stream, CO_HTTP2_STREAM_ERROR_CANCEL);
        }
    }
}

static bool
co_http2_client_on_upgrade_response(
    co_thread_t* thread,
    co_http2_client_t* client
)
{
    co_http_response_t* response = co_http_response_create();

    if (co_http_response_deserialize(
        response, client->receive_data,
        &client->receive_data_index) !=
        CO_HTTP_PARSE_COMPLETE)
    {
        co_http_response_destroy(response);

        return false;
    }

    const co_http_header_t* response_header =
        co_http_response_get_header(response);
    const char* upgrade =
        co_http_header_get_field(
            response_header, CO_HTTP_HEADER_UPGRADE);

    if ((upgrade == NULL) ||
        (strcmp(upgrade, CO_HTTP2_UPGRADE) != 0))
    {
        co_http_response_destroy(response);

        return false;
    }

    uint16_t status_code =
        co_http_response_get_status_code(response);

    if (client->callbacks.on_upgrade != NULL)
    {
        if (status_code == 101)
        {
            client->callbacks.on_upgrade(
                thread, client, response, 0);
        }
        else
        {
            client->callbacks.on_upgrade(
                thread, client, response,
                CO_HTTP2_ERROR_UPGRADE_FAILED);
        }
    }

    co_http_response_destroy(response);

    return true;
}

static void
co_http2_client_on_tcp_connect(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    co_http2_client_t* client =
        (co_http2_client_t*)tcp_client->sock.sub_class;

    if (error_code == 0)
    {
        co_http2_log_info(
            &client->tcp_client->sock.local_net_addr,
            "<--",
            &client->tcp_client->remote_net_addr,
            "http2 connect success");

        co_http2_log_info(
            &client->tcp_client->sock.local_net_addr,
            "-->",
            &client->tcp_client->remote_net_addr,
            "http2 send connection preface");

        co_http2_send_raw_data(client,
            CO_HTTP2_CONNECTION_PREFACE,
            CO_HTTP2_CONNECTION_PREFACE_LENGTH);

        co_http2_send_initial_settings(client);
    }
    else
    {
        co_http2_log_error(
            &client->tcp_client->sock.local_net_addr,
            "<--",
            &client->tcp_client->remote_net_addr,
            "http2 connect error (%d)",
            error_code);

        if (client->callbacks.on_connect != NULL)
        {
            client->callbacks.on_connect(
                thread, client, error_code);
        }
    }
}

static void
co_http2_client_on_tcp_connect_and_upgrade(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    co_http2_client_t* client =
        (co_http2_client_t*)tcp_client->sock.sub_class;

    if (error_code == 0)
    {
        co_http2_send_raw_data(client,
            co_byte_array_get_const_ptr(
                client->upgrade_request_data, 0),
            co_byte_array_get_count(
                client->upgrade_request_data));

        co_byte_array_destroy(client->upgrade_request_data);
        client->upgrade_request_data = NULL;
    }
    else
    {
        co_byte_array_destroy(client->upgrade_request_data);
        client->upgrade_request_data = NULL;

        if (client->callbacks.on_upgrade != NULL)
        {
            client->callbacks.on_upgrade(
                thread, client, NULL, error_code);
        }
    }
}

void
co_http2_client_on_tcp_receive_ready(
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

    size_t data_size =
        co_byte_array_get_count(client->receive_data);

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

            if ((stream == NULL) ||
                (stream->state == CO_HTTP2_STREAM_STATE_CLOSED))
            {
                co_http2_frame_destroy(frame);

                continue;
            }

            co_http2_log_debug_frame(
                &stream->client->tcp_client->sock.local_net_addr, "<--",
                &stream->client->tcp_client->remote_net_addr,
                frame,
                "http2 receive frame");

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
        }
        else if (result == CO_HTTP_PARSE_MORE_DATA)
        {
            co_http2_frame_destroy(frame);

            return;
        }
        else
        {
            co_http2_frame_destroy(frame);

            if (co_http2_client_on_upgrade_response(thread, client))
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

void
co_http2_client_on_tcp_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    (void)thread;

    co_http2_client_t* client =
        (co_http2_client_t*)tcp_client->sock.sub_class;

    co_http2_client_on_close(client, 0);
}

bool
co_http2_set_upgrade_settings(
    const char* b64_settings,
    size_t b64_settings_length,
    co_http2_settings_st* settings
)
{
    size_t settings_data_size = 0;
    uint8_t* settings_data = NULL;

    bool result = co_base64url_decode(
        b64_settings, b64_settings_length,
        &settings_data, &settings_data_size);

    if (result)
    {
        uint16_t param_count =
            (uint16_t)(settings_data_size /
                (sizeof(uint16_t) + sizeof(uint32_t)));
        const uint8_t* temp_data = settings_data;

        for (uint16_t count = 0; count < param_count; ++count)
        {
            uint16_t identifier;
            uint32_t value;

            memcpy(&identifier, temp_data, sizeof(uint16_t));
            identifier = co_byte_order_16_network_to_host(identifier);
            temp_data += sizeof(uint16_t);

            memcpy(&value, temp_data, sizeof(uint16_t));
            value = co_byte_order_32_network_to_host(value);
            temp_data += sizeof(uint32_t);

            co_http2_set_setting_param(settings, identifier, value);
        }
    }

    co_mem_free(settings_data);

    return result;
}

//---------------------------------------------------------------------------//
// public
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

    co_net_addr_t remote_net_addr = { 0 };

    if (!co_net_addr_set_address(
        &remote_net_addr, client->base_url->host))
    {
        co_resolve_hint_st hint = { 0 };
        hint.family = address_family;

        if (co_net_addr_resolve_service(
            client->base_url->host, client->base_url->scheme,
            &hint, &remote_net_addr, 1) == 0)
        {
            co_http2_log_error(NULL, NULL, NULL,
                "failed to resolve hostname (%s)", base_url);

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
#ifdef CO_CAN_USE_TLS
        client->tcp_client =
            co_tls_client_create(local_net_addr, tls_ctx);

        if (client->tcp_client != NULL)
        {
            co_tls_set_host_name(
                client->tcp_client, client->base_url->host);

            const char* protocol = CO_HTTP2_PROTOCOL;

            co_tls_set_available_protocols(
                client->tcp_client, &protocol, 1);
        }
#else
        (void)tls_ctx;

        co_http2_log_error(NULL, NULL, NULL,
            "OpenSSL is not installed");

        co_http_url_destroy(client->base_url);
        co_mem_free(client);

        return NULL;
#endif
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

    co_http2_client_setup(client);

    client->new_stream_id = UINT32_MAX;

    client->tcp_client->callbacks.on_receive =
        (co_tcp_receive_fn)co_http2_client_on_tcp_receive_ready;
    client->tcp_client->callbacks.on_close =
        (co_tcp_close_fn)co_http2_client_on_tcp_close;

    return client;
}

void
co_http2_client_destroy(
    co_http2_client_t* client
)
{
    if (client != NULL)
    {
        co_http2_client_close(client, 0);

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

co_http2_callbacks_st*
co_http2_get_callbacks(
    co_http2_client_t* client
)
{
    return &client->callbacks;
}

void
co_http2_client_close(
    co_http2_client_t* client,
    int error_code
)
{
    if ((client != NULL) &&
        (client->tcp_client != NULL) &&
        (co_tcp_is_open(client->tcp_client)))
    {
        if ((client->system_stream != NULL) &&
            (client->system_stream->state !=
                CO_HTTP2_STREAM_STATE_CLOSED))
        {
            co_http2_frame_t* goaway_frame =
                co_http2_create_goaway_frame(
                    false, client->last_stream_id, error_code,
                    NULL, 0);

            co_http2_stream_send_frame(
                client->system_stream, goaway_frame);
        }

        client->module.close(client->tcp_client);
    }
}

bool
co_http2_is_running(
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
co_http2_connect(
    co_http2_client_t* client
)
{
    client->tcp_client->callbacks.on_connect =
        (co_tcp_connect_fn)co_http2_client_on_tcp_connect;

    bool result = client->module.connect(
        client->tcp_client,
        &client->tcp_client->remote_net_addr);

    if (result)
    {
        co_http2_log_info(
            &client->tcp_client->sock.local_net_addr,
            "-->",
            &client->tcp_client->remote_net_addr,
            "http2 connect start (%s)",
            client->base_url->src);
    }

    return result;
}

bool
co_http2_connect_and_request_upgrade(
    co_http2_client_t* client,
    co_http_request_t* upgrade_request
)
{
    client->upgrade_request_data = co_byte_array_create();

    co_http_request_serialize(
        upgrade_request, client->upgrade_request_data);

    co_http_request_destroy(upgrade_request);

    client->tcp_client->callbacks.on_connect =
        (co_tcp_connect_fn)co_http2_client_on_tcp_connect_and_upgrade;

    return client->module.connect(
        client->tcp_client,
        &client->tcp_client->remote_net_addr);
}

co_http2_stream_t*
co_http2_get_stream(
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
co_http2_send_initial_settings(
    co_http2_client_t* client
)
{
    uint16_t param_count = 0;
    co_http2_setting_param_st params[6];

    if (client->local_settings.header_table_size !=
        CO_HTTP2_SETTING_DEFAULT_HEADER_TABLE_SIZE)
    {
        params[param_count].id =
            CO_HTTP2_SETTING_ID_HEADER_TABLE_SIZE;
        params[param_count].value =
            client->local_settings.header_table_size;
        ++param_count;
    }

    if (client->local_settings.enable_push !=
        CO_HTTP2_SETTING_DEFAULT_ENABLE_PUSH)
    {
        params[param_count].id =
            CO_HTTP2_SETTING_ID_ENABLE_PUSH;
        params[param_count].value =
            client->local_settings.enable_push;
        ++param_count;
    }

    if (client->local_settings.max_concurrent_streams !=
        CO_HTTP2_SETTING_DEFAULT_MAX_CONCURRENT_STREAMS)
    {
        params[param_count].id =
            CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS;
        params[param_count].value =
            client->local_settings.max_concurrent_streams;
        ++param_count;
    }

    if (client->local_settings.initial_window_size !=
        CO_HTTP2_SETTING_DEFAULT_INITIAL_WINDOW_SIZE)
    {
        params[param_count].id =
            CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
        params[param_count].value =
            client->local_settings.initial_window_size;
        ++param_count;
    }

    if (client->local_settings.max_frame_size !=
        CO_HTTP2_SETTING_DEFAULT_MAX_FRAME_SIZE)
    {
        params[param_count].id =
            CO_HTTP2_SETTING_ID_MAX_FRAME_SIZE;
        params[param_count].value =
            client->local_settings.max_frame_size;
        ++param_count;
    }

    if (client->local_settings.max_header_list_size !=
        CO_HTTP2_SETTING_DEFAULT_MAX_HEADER_LIST_SIZE)
    {
        params[param_count].id =
            CO_HTTP2_SETTING_ID_MAX_HEADER_LIST_SIZE;
        params[param_count].value =
            client->local_settings.max_header_list_size;
        ++param_count;
    }

    co_http2_frame_t* frame =
        co_http2_create_settings_frame(
            false, false, params, param_count);

    bool result = co_http2_stream_send_frame(
        client->system_stream, frame);

    if (result &&
        (client->local_settings.initial_window_size !=
            CO_HTTP2_SETTING_DEFAULT_INITIAL_WINDOW_SIZE))
    {
        result = co_http2_stream_send_window_update(
            client->system_stream,
            client->local_settings.initial_window_size);
    }

    return result;
}

void
co_http2_init_settings(
    co_http2_client_t* client,
    const co_http2_setting_param_st* params,
    uint16_t param_count
)
{
    for (size_t index = 0; index < param_count; ++index)
    {
        co_http2_set_setting_param(
            &client->local_settings,
            params[index].id, params[index].value);
    }
}

void
co_http2_update_settings(
    co_http2_client_t* client,
    const co_http2_setting_param_st* params,
    uint16_t param_count
)
{
    co_http2_frame_t* settings_frame =
        co_http2_create_settings_frame(
            false, false, params, param_count);

    co_http2_stream_send_frame(
        client->system_stream, settings_frame);

    for (size_t index = 0; index < param_count; ++index)
    {
        co_http2_set_setting_param(
            &client->local_settings,
            params[index].id, params[index].value);
    }
}

const co_http2_settings_st*
co_http2_get_local_settings(
    const co_http2_client_t* client
)
{
    return &client->local_settings;
}

const co_http2_settings_st*
co_http2_get_remote_settings(
    const co_http2_client_t* client
)
{
    return &client->remote_settings;
}

const co_net_addr_t*
co_http2_get_remote_net_addr(
    const co_http2_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        &client->tcp_client->remote_net_addr : NULL);
}

co_socket_t*
co_http2_client_get_socket(
    co_http2_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        &client->tcp_client->sock : NULL);
}

const char*
co_http2_get_base_url(
    const co_http2_client_t* client
)
{
    return ((client->base_url != NULL) ?
        client->base_url->src : NULL);
}

bool
co_http2_is_open(
    const co_http2_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        co_tcp_is_open(client->tcp_client) : false);
}

bool
co_http2_set_user_data(
    co_http2_client_t* client,
    uintptr_t user_data
)
{
    if (client != NULL)
    {
        return co_tcp_set_user_data(
            client->tcp_client, user_data);
    }

    return false;
}

bool
co_http2_get_user_data(
    const co_http2_client_t* client,
    uintptr_t* user_data
)
{
    if (client != NULL)
    {
        return co_tcp_get_user_data(client->tcp_client, user_data);
    }

    return false;
}
