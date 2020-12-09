#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_net_addr_resolve.h>

#include <coldforce/tls/co_tls_tcp_client.h>

#include <coldforce/http/co_http_client.h>

//---------------------------------------------------------------------------//
// http client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_client_clear_request_queue(
    co_http_client_t* client
)
{
    co_list_iterator_t* send_it =
        co_list_get_head_iterator(client->send_queue);

    while (send_it != NULL)
    {
        co_http_request_destroy(
            (co_http_request_t*)co_list_get_next(
                client->send_queue, &send_it)->value);
    }

    co_list_clear(client->send_queue);

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

void
co_http_client_send_request(
    co_http_client_t* client
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(client->send_queue);

    while (it != NULL)
    {
        co_list_data_st* data = co_list_get(client->send_queue, it);
        co_http_request_t* request = (co_http_request_t*)data->value;

        co_http_request_set_version(request, "1.1");

        if (!co_http_header_contains(
            &request->message.header, CO_HTTP_HEADER_HOST))
        {
            co_http_header_add_item(
                &request->message.header,
                CO_HTTP_HEADER_HOST, client->base_url->host);
        }

        if ((request->message.content.size > 0) &&
            !co_http_header_contains(
                &request->message.header, CO_HTTP_HEADER_CONTENT_LENGTH))
        {
            co_http_header_set_content_length(
                &request->message.header,
                request->message.content.size);
        }

        co_byte_array_t* buffer = co_byte_array_create();

        co_http_request_serialize(request, buffer);

        bool result = client->module.send(
            client->tcp_client,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

        co_byte_array_destroy(buffer);

        if (result)
        {
            co_list_add_tail(client->receive_queue, (uintptr_t)request);

            co_list_iterator_t* temp = it;
            it = co_list_get_next_iterator(client->send_queue, it);
            co_list_remove_at(client->send_queue, temp);
        }
        else
        {
            break;
        }
    }
}

void
co_http_client_resopnse(
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
        data = co_list_get_head(client->send_queue);

        if (data != NULL)
        {
            request = (co_http_request_t*)data->value;
            co_list_remove_head(client->send_queue);
        }
    }

    if (request == NULL)
    {
        co_http_client_clear_request_queue(client);

        return;
    }

    co_http_response_fn response_handler = client->on_response;

    if (error_code == 0)
    {
        co_http_response_t* response = client->response;
        client->response = NULL;
 
        if (response_handler != NULL)
        {
            response_handler(
                client->tcp_client->sock.owner_thread,
                client, request, response, error_code);
        }

        co_http_request_destroy(request);
        co_http_response_destroy(response);
    }
    else
    {
        co_http_content_receiver_clear(&client->content_receiver);

        co_http_response_destroy(client->response);
        client->response = NULL;

        co_http_client_clear_request_queue(client);

        if (response_handler != NULL)
        {
            response_handler(
                client->tcp_client->sock.owner_thread,
                client, request, NULL, error_code);
        }

        co_http_request_destroy(request);
    }
}

void
co_http_client_receive(
    co_http_client_t* client
)
{
    ssize_t size = client->module.receive_all(
        client->tcp_client, client->receive_data);

    if (size <= 0)
    {
        return;
    }

    size_t index = 0;

    while (((size_t)size) > index)
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
                co_http_client_resopnse(
                    client, CO_HTTP_ERROR_OUT_OF_MEMORY);

                return;
            }

            int result = co_http_response_deserialize(
                client->response, client->receive_data, &index);

            if (result == CO_HTTP_PARSE_COMPLETE)
            {
                co_http_content_receiver_clear(&client->content_receiver);

                if (!co_http_start_receive_content(
                    &client->content_receiver, &client->response->message,
                    index, request->save_file_path))
                {
                    return;
                }

                if (client->on_progress != NULL)
                {
                    if (!client->on_progress(
                        client->tcp_client->sock.owner_thread,
                        client, request, client->response,
                        client->content_receiver.receive_size))
                    {
                        co_http_client_resopnse(
                            client, CO_HTTP_ERROR_CANCEL);

                        return;
                    }
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
                co_http_client_resopnse(
                    client, CO_HTTP_ERROR_PARSE_HEADER);

                return;
            }
        }

        int result = co_http_receive_content_data(
            &client->content_receiver, client->receive_data);

        if (result == CO_HTTP_PARSE_COMPLETE)
        {            
            if (client->on_progress != NULL)
            {
                if (!client->on_progress(
                    client->tcp_client->sock.owner_thread,
                    client, request, client->response,
                    client->content_receiver.receive_size))
                {
                    co_http_client_resopnse(
                        client, CO_HTTP_ERROR_CANCEL);

                    return;
                }
            }

            co_http_complete_receive_content(
                &client->content_receiver,
                &index, &client->response->message.content);

            co_http_client_resopnse(client, 0);

            if (client->tcp_client == NULL)
            {
                return;
            }
        }
        else if (result == CO_HTTP_PARSE_MORE_DATA)
        {
            co_http_content_more_data(
                &client->content_receiver, client->receive_data);

            if (client->on_progress != NULL)
            {
                if (!client->on_progress(
                    client->tcp_client->sock.owner_thread,
                    client, request, client->response,
                    client->content_receiver.receive_size))
                {
                    co_http_client_resopnse(
                        client, CO_HTTP_ERROR_CANCEL);
                }
            }

            return;
        }
        else
        {
            co_http_client_resopnse(
                client, CO_HTTP_ERROR_PARSE_CONTENT);

            return;
        }
    }

    if (co_list_get_count(client->receive_queue) == 0)
    {
        co_byte_array_set_count(client->receive_data, 0);
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_client_on_connect(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    (void)thread;

    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    if (error_code == 0)
    {
        if (tcp_client->sock.tls != NULL)
        {
            co_tls_tcp_start_handshake_async(tcp_client,
                (co_tls_tcp_handshake_fn)co_http_client_on_tls_handshake);

            return;
        }
        else
        {
            co_http_client_send_request(client);
        }
    }
    else
    {
        co_http_client_resopnse(
            client, CO_HTTP_ERROR_CONNECT_FAILED);
    }

    client->connecting = false;
}

void
co_http_client_on_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    (void)thread;

    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    co_http_client_receive(client);
}

void
co_http_client_on_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    client->module.close(client->tcp_client);

    co_http_close_fn close_handler = client->on_close;

    co_http_client_resopnse(
        client, CO_HTTP_ERROR_CONNECTION_CLOSED);

    if (close_handler != NULL)
    {
        close_handler(thread, client);
    }
}

void
co_http_client_on_tls_handshake(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    (void)thread;

    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    client->connecting = false;

    if (error_code == 0)
    {
        co_http_client_send_request(client);
    }
    else
    {
        co_http_client_resopnse(
            client, CO_HTTP_ERROR_TLS_HANDSHAKE_FAILED);
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

    if (!co_net_addr_set_address(
        &client->remote_net_addr, client->base_url->host))
    {
        co_resolve_hint_st hint = { 0 };
        hint.family = address_family;

        if (co_net_addr_resolve_service(
            client->base_url->host, client->base_url->scheme,
            &hint, &client->remote_net_addr, 1) == 0)
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
            &client->remote_net_addr, port);
    }

    if (secure)
    {
        client->module.destroy = co_tls_tcp_client_destroy;
        client->module.close = co_tls_tcp_client_close;
        client->module.connect_async = co_tls_tcp_connect_async;
        client->module.send = co_tls_tcp_send;
        client->module.receive_all = co_tls_tcp_receive_all;

        client->tcp_client =
            co_tls_tcp_client_create(local_net_addr, tls_ctx);
    }
    else
    {
        client->module.destroy = co_tcp_client_destroy;
        client->module.close = co_tcp_client_close;
        client->module.connect_async = co_tcp_connect_async;
        client->module.send = co_tcp_send;
        client->module.receive_all = co_tcp_receive_all;

        client->tcp_client =
            co_tcp_client_create(local_net_addr);
    }

    if (client->tcp_client == NULL)
    {
        co_http_url_destroy(client->base_url);
        co_mem_free(client);

        return NULL;
    }

    client->tcp_client->sock.sub_class = client;

    client->connecting = false;
    client->send_queue = co_list_create(NULL);
    client->receive_queue = co_list_create(NULL);
    client->response = NULL;
    client->receive_data = co_byte_array_create();

    co_http_content_receiver_setup(&client->content_receiver);

    client->on_response = NULL;
    client->on_progress = NULL;
    client->on_close = NULL;

    co_tcp_set_receive_handler(
        client->tcp_client,
        (co_tcp_receive_fn)co_http_client_on_receive_ready);
    co_tcp_set_close_handler(
        client->tcp_client,
        (co_tcp_close_fn)co_http_client_on_close);

    return client;
}

void
co_http_client_destroy(
    co_http_client_t* client
)
{
    if (client != NULL)
    {
        co_http_content_receiver_cleanup(&client->content_receiver);

        co_http_client_clear_request_queue(client);

        co_list_destroy(client->send_queue);
        client->send_queue = NULL;

        co_list_destroy(client->receive_queue);
        client->receive_queue = NULL;

        co_http_response_destroy(client->response);
        client->response = NULL;

        co_http_url_destroy(client->base_url);
        client->base_url = NULL;

        co_byte_array_destroy(client->receive_data);
        client->receive_data = NULL;

        client->module.destroy(client->tcp_client);
        client->tcp_client = NULL;

        co_mem_free_later(client);
    }
}

bool
co_http_request_async(
    co_http_client_t* client,
    co_http_request_t* request
)
{    
    if (co_tcp_is_open(client->tcp_client))
    {
        co_list_add_tail(client->send_queue, (uintptr_t)request);

        co_http_client_send_request(client);
    }
    else if(client->connecting)
    { 
        co_list_add_tail(client->send_queue, (uintptr_t)request);
    }
    else
    {
        if (!client->module.connect_async(
            client->tcp_client,
            &client->remote_net_addr,
            (co_tcp_connect_fn)co_http_client_on_connect))
        {
            return false;
        }

        client->connecting = true;

        co_list_add_tail(client->send_queue, (uintptr_t)request);
    }

    return true;
}

void
co_http_set_response_handler(
    co_http_client_t* client,
    co_http_response_fn handler
)
{
    client->on_response = handler;
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

const char*
co_http_client_get_base_url(
    const co_http_client_t* client
)
{
    return client->base_url->src;
}

bool
co_http_client_is_open(
    const co_http_client_t* client
)
{
    return co_tcp_is_open(client->tcp_client);
}
