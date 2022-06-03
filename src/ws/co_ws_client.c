#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_byte_order.h>
#include <coldforce/net/co_net_addr_resolve.h>

#include <coldforce/tls/co_tls_client.h>

#include <coldforce/ws/co_random.h>
#include <coldforce/ws/co_ws_client.h>
#include <coldforce/ws/co_ws_http_extension.h>

//---------------------------------------------------------------------------//
// websocket client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_ws_client_setup(
    co_ws_client_t* client,
    co_tcp_client_t* tcp_client,
    co_http_url_st* base_url
)
{
    client->tcp_client = tcp_client;

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

    client->base_url = base_url;

    client->receive_data_index = 0;
    client->receive_data = co_byte_array_create();

    client->on_connect = NULL;
    client->on_receive = NULL;
    client->on_close = NULL;

    client->upgrade_request = NULL;

    client->mask = false;
    client->closed = false;
}

void
co_ws_client_cleanup(
    co_ws_client_t* client
)
{
    if (client != NULL)
    {
        co_http_url_destroy(client->base_url);
        client->base_url = NULL;

        co_byte_array_destroy(client->receive_data);
        client->receive_data = NULL;

        co_http_request_destroy(client->upgrade_request);
        client->upgrade_request = NULL;
    }
}

void
co_ws_client_on_frame(
    co_thread_t* thread,
    co_ws_client_t* client,
    co_ws_frame_t* frame,
    int error_code
)
{
    if (error_code == CO_WS_ERROR_DATA_TOO_BIG)
    {
        co_ws_send_close(client,
            CO_WS_CLOSE_REASON_DATA_TOO_BIG, NULL);
    }
    else if (error_code != 0)
    {
        co_ws_send_close(client,
            CO_WS_CLOSE_REASON_PROTOCOL_ERROR, NULL);
    }

    if (client->on_receive != NULL)
    {
        client->on_receive(
            thread, client, frame, error_code);
    }

    co_ws_frame_destroy(frame);
}

static void
co_ws_client_on_connect(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    co_ws_client_t* client =
        (co_ws_client_t*)tcp_client->sock.sub_class;

    if (error_code == 0)
    {
        co_http_request_set_version(
            client->upgrade_request, CO_HTTP_VERSION_1_1);

        co_http_header_t* header =
            co_http_request_get_header(client->upgrade_request);

        if (!co_http_header_contains(header, CO_HTTP_HEADER_HOST))
        {
            char* host =
                co_http_url_create_host_and_port(client->base_url);

            co_http_header_add_field(
                header, CO_HTTP_HEADER_HOST, host);

            co_http_url_destroy_string(host);
        }

        co_byte_array_t* buffer = co_byte_array_create();

        co_http_request_serialize(
            client->upgrade_request, buffer);

        co_ws_send_raw_data(client,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

        co_byte_array_destroy(buffer);
    }
    else
    {
        co_http_request_destroy(client->upgrade_request);
        client->upgrade_request = NULL;

        if (client->on_connect != NULL)
        {
            co_ws_connect_fn handler = client->on_connect;
            client->on_connect = NULL;

            handler(thread, client, NULL, error_code);
        }
    }
}

void
co_ws_client_on_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_ws_client_t* client =
        (co_ws_client_t*)tcp_client->sock.sub_class;

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
        if ((data_size - client->receive_data_index) <
            CO_WS_FRAME_HEADER_MIN_SIZE)
        {
            return;
        }

        co_ws_frame_t* frame = co_ws_frame_create();

        int result = co_ws_frame_deserialize(frame,
            client->receive_data, &client->receive_data_index);

        if (result == CO_WS_PARSE_COMPLETE)
        {
            co_ws_client_on_frame(
                thread, client, frame, 0);

            if (client->tcp_client == NULL)
            {
                return;
            }

            continue;
        }
        else if (result == CO_WS_PARSE_MORE_DATA)
        {
            co_ws_frame_destroy(frame);

            return;
        }
        else
        {
            co_ws_frame_destroy(frame);

            if (client->on_connect != NULL)
            {
                co_ws_connect_fn handler = client->on_connect;
                client->on_connect = NULL;

                co_http_response_t* response = co_http_response_create();

                if (co_http_response_deserialize(response,
                    client->receive_data, &client->receive_data_index) ==
                    CO_HTTP_PARSE_COMPLETE)
                {
                    handler(thread, client, response, 0);

                    co_http_response_destroy(response);
                    co_http_request_destroy(client->upgrade_request);
                    client->upgrade_request = NULL;

                    if (client->tcp_client == NULL)
                    {
                        return;
                    }

                    continue;
                }
                else
                {
                    co_http_response_destroy(response);

                    handler(thread, client, NULL,
                        CO_WS_ERROR_INVALID_RESPONSE);

                    return;
                }
            }

            co_ws_client_on_frame(
                thread, client, NULL, result);

            return;
        }
    }

    client->receive_data_index = 0;
    co_byte_array_clear(client->receive_data);
}

void
co_ws_client_on_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_ws_client_t* client =
        (co_ws_client_t*)tcp_client->sock.sub_class;

    if (client->on_close != NULL)
    {
        client->on_close(thread, client);
    }
}

bool
co_ws_send_raw_data(
    co_ws_client_t* client,
    const void* data,
    size_t data_size
)
{
    return client->module.send(
        client->tcp_client, data, data_size);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_ws_client_t*
co_ws_client_create(
    const char* base_url,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_ws_client_t* client =
        (co_ws_client_t*)co_mem_alloc(sizeof(co_ws_client_t));

    if (client == NULL)
    {
        return NULL;
    }

    co_http_url_st* url = co_http_url_create(base_url);

    if (url->host == NULL)
    {
        co_http_url_destroy(url);
        co_mem_free(client);

        return NULL;
    }

    if (url->scheme == NULL)
    {
        client->base_url->scheme = co_string_duplicate("http");
    }
    else if (co_string_case_compare(url->scheme, "wss") == 0)
    {
        co_string_destroy(url->scheme);
        url->scheme = co_string_duplicate("https");
    }
    else if (co_string_case_compare(url->scheme, "ws") == 0)
    {
        co_string_destroy(url->scheme);
        url->scheme = co_string_duplicate("http");
    }

    bool secure = false;

    if (co_string_case_compare(url->scheme, "https") == 0)
    {
        secure = true;
    }

    int address_family =
        co_net_addr_get_family(local_net_addr);

    co_net_addr_t remote_net_addr = { 0 };

    if (!co_net_addr_set_address(
        &remote_net_addr, url->host))
    {
        co_resolve_hint_st hint = { 0 };
        hint.family = address_family;

        if (co_net_addr_resolve_service(
            url->host, url->scheme,
            &hint, &remote_net_addr, 1) == 0)
        {
            co_http_url_destroy(url);
            co_mem_free(client);

            return NULL;
        }
    }
    else
    {
        uint16_t port = url->port;

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

    co_tcp_client_t* tcp_client = NULL;

    if (secure)
    {
#ifdef CO_CAN_USE_TLS
        tcp_client =
            co_tls_client_create(local_net_addr, tls_ctx);

        if (tcp_client != NULL)
        {
            co_tls_set_host_name(tcp_client, url->host);

            const char* protocol = CO_HTTP_PROTOCOL;

            co_tls_set_available_protocols(tcp_client, &protocol, 1);
        }
#else
        (void)tls_ctx;

        co_http_url_destroy(url);
        co_mem_free(client);

        return NULL;
#endif
    }
    else
    {
        tcp_client = co_tcp_client_create(local_net_addr);
    }

    if (tcp_client == NULL)
    {
        co_http_url_destroy(url);
        co_mem_free(client);

        return NULL;
    }

    memcpy(&tcp_client->remote_net_addr,
        &remote_net_addr, sizeof(co_net_addr_t));

    co_ws_client_setup(client, tcp_client, url);

    client->mask = true;

    co_tcp_set_receive_handler(
        client->tcp_client,
        (co_tcp_receive_fn)co_ws_client_on_receive_ready);
    co_tcp_set_close_handler(
        client->tcp_client,
        (co_tcp_close_fn)co_ws_client_on_close);

    return client;
}

void
co_ws_client_destroy(
    co_ws_client_t* client
)
{
    if (client != NULL)
    {
        if (client->tcp_client != NULL)
        {
            co_ws_send_close(
                client, CO_WS_CLOSE_REASON_NORMAL, NULL);

            client->module.destroy(client->tcp_client);
            client->tcp_client = NULL;
        }

        co_ws_client_cleanup(client);
        co_mem_free_later(client);
    }
}

void
co_ws_client_close(
    co_ws_client_t* client
)
{
    if (client != NULL)
    {
        if (client->tcp_client != NULL)
        {
            client->module.close(client->tcp_client);
        }
    }
}

bool
co_ws_connect(
    co_ws_client_t* client,
    co_http_request_t* upgrade_request,
    co_ws_connect_fn handler
)
{
    client->on_connect = handler;

    co_http_request_destroy(client->upgrade_request);
    client->upgrade_request = upgrade_request;

    return client->module.connect(
        client->tcp_client,
        &client->tcp_client->remote_net_addr,
        (co_tcp_connect_fn)co_ws_client_on_connect);
}

bool
co_ws_send(
    co_ws_client_t* client,
    bool fin,
    uint8_t opcode,
    const void* data,
    size_t data_size
)
{
    uint8_t header[32];
    uint32_t header_size = CO_WS_FRAME_HEADER_MIN_SIZE;

    header[0] = opcode;

    if (fin)
    {
        header[0] |= 0x80;
    }

    if (data_size <= 125)
    {
        header[1] = (uint8_t)data_size;
    }
    else if (data_size <= UINT16_MAX)
    {
        header[1] = 126;

        header[2] = (uint8_t)((data_size & 0xff00) >> 8);
        header[3] = (uint8_t)(data_size & 0x00ff);

        header_size += 2;
    }
    else
    {
        header[1] = 127;

        header[2] = (uint8_t)((data_size & 0xff00000000000000) >> 56);
        header[3] = (uint8_t)((data_size & 0x00ff000000000000) >> 48);
        header[4] = (uint8_t)((data_size & 0x0000ff0000000000) >> 40);
        header[5] = (uint8_t)((data_size & 0x000000ff00000000) >> 32);
        header[6] = (uint8_t)((data_size & 0x00000000ff000000) >> 24);
        header[7] = (uint8_t)((data_size & 0x0000000000ff0000) >> 16);
        header[8] = (uint8_t)((data_size & 0x000000000000ff00) >> 8);
        header[9] = (uint8_t) (data_size & 0x00000000000000ff);

        header_size += 8;
    }

    bool result = false;

    if (client->mask)
    {
        header[1] |= 0x80;

        uint8_t* masked_data =
            (uint8_t*)co_mem_alloc(data_size);

        if (masked_data == NULL)
        {
            return false;
        }

        uint8_t mask_key[CO_WS_FRAME_MASK_SIZE];

        co_random(mask_key, sizeof(mask_key));

        for (uint16_t index = 0;
            index < CO_WS_FRAME_MASK_SIZE; ++index)
        {
            header[header_size] = mask_key[index];
            ++header_size;
        }

        for (uint64_t index = 0; index < data_size; ++index)
        {
            masked_data[index] = ((uint8_t*)data)[index] ^
                mask_key[index % CO_WS_FRAME_MASK_SIZE];
        }

        result = co_ws_send_raw_data(
            client, header, header_size);

        if (result && (data_size > 0))
        {
            result = co_ws_send_raw_data(
                client, masked_data, data_size);
        }

        co_mem_free(masked_data);
    }
    else
    {
        result = co_ws_send_raw_data(
            client, header, header_size);

        if (result && (data_size > 0))
        {
            result = co_ws_send_raw_data(
                client, data, data_size);
        }
    }

    return result;
}

bool
co_ws_send_binary(
    co_ws_client_t* client,
    const void* data,
    size_t data_size
)
{
    return co_ws_send(client, true,
        CO_WS_OPCODE_BINARY, data, data_size);
}

bool
co_ws_send_text(
    co_ws_client_t* client,
    const char* utf8_str
)
{
    return co_ws_send(client, true,
        CO_WS_OPCODE_TEXT, utf8_str, strlen(utf8_str));
}

bool
co_ws_send_continuation(
    co_ws_client_t* client,
    bool fin,
    const void* data,
    size_t data_size
)
{
    return co_ws_send(client, fin,
        CO_WS_OPCODE_CONTINUATION, data, data_size);
}

bool
co_ws_send_close(
    co_ws_client_t* client,
    uint16_t reason_code,
    const char* utf8_reason_str
)
{
    if (client->closed)
    {
        return true;
    }

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
    
    bool result = co_ws_send(client,
        true, CO_WS_OPCODE_CLOSE, data, data_size);

    co_mem_free(data);

    client->closed = true;

    return result;
}

bool
co_ws_send_ping(
    co_ws_client_t* client,
    const void* data,
    size_t data_size
)
{
    return co_ws_send(client,
        true, CO_WS_OPCODE_PING, data, data_size);
}

bool
co_ws_send_pong(
    co_ws_client_t* client,
    const void* data,
    size_t data_size
)
{
    return co_ws_send(client,
        true, CO_WS_OPCODE_PONG, data, data_size);
}

void
co_ws_set_receive_handler(
    co_ws_client_t* client,
    co_ws_receive_fn handler
)
{
    client->on_receive = handler;
}

void
co_ws_set_close_handler(
    co_ws_client_t* client,
    co_ws_close_fn handler
)
{
    client->on_close = handler;
}

void
co_ws_default_handler(
    co_ws_client_t* client,
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

        co_ws_send_close(client, reason_code, NULL);

        break;
    }
    case CO_WS_OPCODE_PING:
    {
        co_ws_send_pong(client,
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

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

const co_net_addr_t*
co_ws_get_remote_net_addr(
    const co_ws_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        &client->tcp_client->remote_net_addr : NULL);
}

co_socket_t*
co_ws_client_get_socket(
    co_ws_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        &client->tcp_client->sock : NULL);
}

const char*
co_ws_get_base_url(
    const co_ws_client_t* client
)
{
    return ((client->base_url != NULL) ?
        client->base_url->src : NULL);
}

bool
co_ws_is_open(
    const co_ws_client_t* client
)
{
    return (!client->closed) && ((client->tcp_client != NULL) ?
        co_tcp_is_open(client->tcp_client) : false);
}

bool
co_ws_set_user_data(
    co_ws_client_t* client,
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
co_ws_get_user_data(
    const co_ws_client_t* client,
    uintptr_t* user_data
)
{
    if (client != NULL)
    {
        return co_tcp_get_user_data(
            client->tcp_client, user_data);
    }

    return false;
}
