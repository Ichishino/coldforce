#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_net_addr_resolve.h>

#include <coldforce/tls/co_tls_client.h>

#include <coldforce/http/co_http_client.h>
#include <coldforce/http/co_http_config.h>
#include <coldforce/http/co_http_log.h>

//---------------------------------------------------------------------------//
// http client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_client_setup(
    co_http_client_t* client
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

    client->receive_queue = co_list_create(NULL);
    client->receive_data_index = 0;
    client->receive_data = co_byte_array_create();

    client->request = NULL;
    client->response = NULL;

    co_http_content_receiver_setup(&client->content_receiver);

    client->on_receive = NULL;
    client->on_progress = NULL;
    client->on_close = NULL;
}

void
co_http_client_cleanup(
    co_http_client_t* client
)
{
    if (client != NULL)
    {
        co_http_content_receiver_cleanup(&client->content_receiver);

        co_list_destroy(client->receive_queue);
        client->receive_queue = NULL;

        co_byte_array_destroy(client->receive_data);
        client->receive_data = NULL;

        co_http_request_destroy(client->request);
        client->request = NULL;

        co_http_response_destroy(client->response);
        client->response = NULL;
    }
}

bool
co_http_send_raw_data(
    co_http_client_t* client,
    const void* data,
    size_t data_size
)
{
    return client->module.send(
        client->tcp_client, data, data_size);
}

static void
co_http_client_clear_request_queue(
    co_http_client_t* client
)
{
    co_list_iterator_t* receive_it =
        co_list_get_head_iterator(client->receive_queue);

    while (receive_it != NULL)
    {
        co_http_request_destroy(
            (co_http_request_t*)co_list_get_next(
                client->receive_queue, &receive_it)->value);
    }

    co_list_clear(client->receive_queue);
}

static void
co_http_client_on_resopnse(
    co_thread_t* thread,
    co_http_client_t* client,
    int error_code
)
{
    co_http_request_t* request = NULL;

    co_list_data_st* data =
        co_list_get_head(client->receive_queue);

    if (data != NULL)
    {
        request = (co_http_request_t*)data->value;
        co_list_remove_head(client->receive_queue);
    }
    else
    {
        return;
    }

    if (client->request != NULL)
    {
        co_http_request_destroy(client->request);
    }

    client->request = request;

    if (error_code == 0)
    {
        if (client->on_receive != NULL)
        {
            client->on_receive(thread, client,
                (const co_http_message_t*)client->response, error_code);
        }

        co_http_response_destroy(client->response);
        client->response = NULL;
    }
    else
    {
        co_http_content_receiver_clear(&client->content_receiver);

        co_http_response_destroy(client->response);
        client->response = NULL;

        co_http_client_clear_request_queue(client);

        if (client->on_receive != NULL)
        {
            client->on_receive(thread, client, NULL, error_code);
        }
    }
}

static bool
co_http_client_on_progress(
    co_thread_t* thread,
    co_http_client_t* client
)
{
    if (client->on_progress != NULL)
    {
        if (!client->on_progress(thread, client,
            (const co_http_message_t*)client->response,
            client->content_receiver.receive_size))
        {
            co_http_client_on_resopnse(
                thread, client, CO_HTTP_ERROR_CANCEL);

            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static void
co_http_client_on_tcp_connect(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    if (client->on_connect != NULL)
    {
        co_http_connect_fn handler = client->on_connect;
        client->on_connect = NULL;

        handler(thread, client, error_code);
    }
}

static void
co_http_client_on_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

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
        co_list_data_st* data =
            co_list_get_head(client->receive_queue);

        if (data == NULL)
        {
            break;
        }

        co_http_request_t* request =
            (co_http_request_t*)data->value;

        if (client->response == NULL)
        {
            client->response = co_http_response_create();

            if (client->response == NULL)
            {
                co_http_client_on_resopnse(
                    thread, client, CO_HTTP_ERROR_OUT_OF_MEMORY);

                return;
            }

            int result = co_http_response_deserialize(
                client->response, client->receive_data,
                &client->receive_data_index);

            if (result == CO_HTTP_PARSE_COMPLETE)
            {
                co_http_log_trace_response_header(
                    client, "<--", client->response, "http receive response");

                co_http_content_receiver_clear(&client->content_receiver);

                if (!co_http_start_receive_content(
                    &client->content_receiver,
                    &client->response->message,
                    client->receive_data_index,
                    request->save_file_path))
                {
                    return;
                }

                if ((!client->content_receiver.chunked) &&
                    (client->content_receiver.size >
                        co_http_config_get_max_receive_content_size()))
                {
                    co_http_client_on_resopnse(
                        thread, client, CO_HTTP_ERROR_CONTENT_TOO_BIG);

                    return;
                }

                if (!co_http_client_on_progress(thread, client))
                {
                    return;
                }
            }
            else if (result == CO_HTTP_PARSE_MORE_DATA)
            {
                co_http_response_destroy(client->response);
                client->response = NULL;

                return;
            }
            else
            {
                co_http_client_on_resopnse(thread, client, result);

                return;
            }
        }

        int result = co_http_receive_content_data(
            &client->content_receiver, client->receive_data);

        if (result == CO_HTTP_PARSE_COMPLETE)
        {
            if (!co_http_client_on_progress(thread, client))
            {
                return;
            }

            co_http_complete_receive_content(
                &client->content_receiver,
                &client->receive_data_index,
                &client->response->message.content);

            co_http_client_on_resopnse(thread, client, 0);

            if (client->tcp_client == NULL)
            {
                return;
            }
        }
        else if (result == CO_HTTP_PARSE_MORE_DATA)
        {
            co_http_content_more_data(
                &client->content_receiver, client->receive_data);

            co_http_client_on_progress(thread, client);

            return;
        }
        else
        {
            co_http_client_on_resopnse(
                thread, client, CO_HTTP_ERROR_PARSE_CONTENT);

            return;
        }
    }

    if (co_list_get_count(client->receive_queue) == 0)
    {
        client->receive_data_index = 0;
        co_byte_array_clear(client->receive_data);
    }
}

static void
co_http_client_on_tcp_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    client->module.close(client->tcp_client);

    co_http_client_on_resopnse(
        thread, client, CO_HTTP_ERROR_CONNECTION_CLOSED);

    if (client->on_close != NULL)
    {
        client->on_close(thread, client);
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_http_client_t*
co_http_client_create(
    const char* base_url,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_http_client_t* client =
        (co_http_client_t*)co_mem_alloc(sizeof(co_http_client_t));

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

            const char* protocol = CO_HTTP_PROTOCOL;

            co_tls_set_available_protocols(
                client->tcp_client, &protocol, 1);
        }
#else
        (void)tls_ctx;

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

    co_http_client_setup(client);

    co_tcp_set_receive_handler(
        client->tcp_client,
        (co_tcp_receive_fn)co_http_client_on_receive_ready);
    co_tcp_set_close_handler(
        client->tcp_client,
        (co_tcp_close_fn)co_http_client_on_tcp_close);

    return client;
}

void
co_http_client_destroy(
    co_http_client_t* client
)
{
    if (client != NULL)
    {
        co_http_client_clear_request_queue(client);

        co_http_client_cleanup(client);

        co_http_url_destroy(client->base_url);
        client->base_url = NULL;

        if (client->tcp_client != NULL)
        {
            client->module.destroy(client->tcp_client);
            client->tcp_client = NULL;
        }

        co_mem_free_later(client);
    }
}

bool
co_http_connect(
    co_http_client_t* client,
    co_http_connect_fn handler
)
{
    client->on_connect = handler;

    return client->module.connect(
        client->tcp_client,
        &client->tcp_client->remote_net_addr,
        (co_tcp_connect_fn)co_http_client_on_tcp_connect);
}

bool
co_http_send_request(
    co_http_client_t* client,
    co_http_request_t* request
)
{    
    co_http_request_set_version(request, CO_HTTP_VERSION_1_1);

    if (!co_http_header_contains(
        &request->message.header, CO_HTTP_HEADER_HOST))
    {
        char* host = co_http_url_create_host_and_port(client->base_url);

        co_http_header_add_field(
            &request->message.header,
            CO_HTTP_HEADER_HOST, host);

        co_http_url_destroy_string(host);
    }

    if ((request->message.content.size > 0) &&
        !co_http_header_contains(
            &request->message.header, CO_HTTP_HEADER_CONTENT_LENGTH))
    {
        co_http_header_set_content_length(
            &request->message.header,
            request->message.content.size);
    }

    co_http_log_trace_request_header(
        client, "-->", request, "http send request");

    co_byte_array_t* buffer = co_byte_array_create();

    co_http_request_serialize(request, buffer);

    bool result =
        co_http_send_raw_data(client,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    if (result)
    {
        co_list_add_tail(client->receive_queue, (uintptr_t)request);
    }

    return result;
}

bool
co_http_send_data(
    co_http_client_t* client,
    const void* data,
    size_t data_size
)
{
    co_net_log_hex_dump(
        CO_HTTP_LOG_CATEGORY,
        &client->tcp_client->sock.local_net_addr,
        "-->",
        &client->tcp_client->remote_net_addr,
        data, data_size,
        "http send data %zd bytes", data_size);

    return client->module.send(
        client->tcp_client, data, data_size);
}

bool
co_http_is_running(
    const co_http_client_t* client
)
{
    return (client != NULL && client->tcp_client != NULL &&
        co_list_get_count(client->receive_queue) > 0);
}

const co_http_request_t*
co_http_get_request(
    const co_http_client_t* client
)
{
    return client->request;
}

const co_http_response_t*
co_http_get_response(
    const co_http_client_t* client
)
{
    return client->response;
}

void
co_http_set_receive_handler(
    co_http_client_t* client,
    co_http_receive_fn handler
)
{
    client->on_receive = handler;
}

void
co_http_set_progress_handler(
    co_http_client_t* client,
    co_http_progress_fn handler
)
{
    client->on_progress = handler;
}

void
co_http_set_close_handler(
    co_http_client_t* client,
    co_http_close_fn handler
)
{
    client->on_close = handler;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

const co_net_addr_t*
co_http_get_remote_net_addr(
    const co_http_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        &client->tcp_client->remote_net_addr : NULL);
}

co_socket_t*
co_http_client_get_socket(
    co_http_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        &client->tcp_client->sock : NULL);
}

const char*
co_http_get_base_url(
    const co_http_client_t* client
)
{
    return ((client->base_url != NULL) ?
        client->base_url->src : NULL);
}

bool
co_http_is_open(
    const co_http_client_t* client
)
{
    return ((client->tcp_client != NULL) ?
        co_tcp_is_open(client->tcp_client) : false);
}

bool
co_http_set_user_data(
    co_http_client_t* client,
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
co_http_get_user_data(
    const co_http_client_t* client,
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
