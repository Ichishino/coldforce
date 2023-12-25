#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_net_addr_resolve.h>

#include <coldforce/tls/co_tls_tcp_client.h>

#include <coldforce/http/co_http_client.h>
#include <coldforce/http/co_http_config.h>
#include <coldforce/http/co_http_log.h>

//---------------------------------------------------------------------------//
// http client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static void
co_http_client_on_tcp_receive_timer(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
);

static void
co_http_client_clear_request_queue(
    co_http_client_t* client
)
{
    if (client != NULL &&
        client->request_queue != NULL)
    {
        co_list_iterator_t* receive_it =
            co_list_get_head_iterator(client->request_queue);

        while (receive_it != NULL)
        {
            co_http_request_t* request =
                (co_http_request_t*)co_list_get_next(
                    client->request_queue, &receive_it)->value;

            if (client->request == request)
            {
                client->request = NULL;
            }

            co_http_request_destroy(request);
        }

        co_list_clear(client->request_queue);
    }
}

void
co_http_client_setup(
    co_http_client_t* client
)
{
    client->conn.tcp_client->callbacks.on_timer =
        co_http_client_on_tcp_receive_timer;

    co_tcp_create_timer(
        client->conn.tcp_client,
        co_http_config_get_max_receive_wait_time());

    client->callbacks.on_close = NULL;
    client->callbacks.on_connect = NULL;
    client->callbacks.on_receive_start = NULL;
    client->callbacks.on_receive_finish = NULL;
    client->callbacks.on_receive_data = NULL;

    client->request_queue = co_list_create(NULL);

    co_http_content_receiver_setup(&client->content_receiver);

    client->request = NULL;
    client->response = NULL;
}

void
co_http_client_cleanup(
    co_http_client_t* client
)
{
    if (client != NULL)
    {
        co_http_client_clear_request_queue(client);

        co_list_destroy(client->request_queue);
        client->request_queue = NULL;

        co_http_content_receiver_cleanup(&client->content_receiver);

        co_http_request_destroy(client->request);
        client->request = NULL;

        co_http_response_destroy(client->response);
        client->response = NULL;
    }
}

static void
co_http_client_on_resopnse(
    co_thread_t* thread,
    co_http_client_t* client,
    int error_code
)
{
    co_list_data_st* data =
        co_list_get_head(client->request_queue);

    if (data != NULL)
    {
        if (client->request != NULL &&
            client->request == (co_http_request_t*)data->value)
        {
            co_list_remove_head(client->request_queue);
        }
    }

    if ((error_code != 0) ||
        (co_list_get_count(client->request_queue) == 0))
    {
        co_tcp_stop_timer(client->conn.tcp_client);
    }

    if (error_code == 0)
    {
        if (client->callbacks.on_receive_finish != NULL)
        {
            client->callbacks.on_receive_finish(
                thread, client,
                client->request, client->response, error_code);
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

        if (client->callbacks.on_receive_finish != NULL)
        {
            client->callbacks.on_receive_finish(
                thread, client,
                client->request, NULL, error_code);
        }
    }

    co_http_request_destroy(client->request);
    client->request = NULL;
}

static void
co_http_client_on_http_connection_connect(
    co_thread_t* thread,
    co_http_connection_t* conn,
    int error_code
)
{
    co_http_client_t* client = (co_http_client_t*)conn;

    if (client->callbacks.on_connect != NULL)
    {
        client->callbacks.on_connect(thread, client, error_code);
    }
}

void
co_http_client_on_http_connection_close(
    co_thread_t* thread,
    co_http_connection_t* conn
)
{
    co_http_client_t* client = (co_http_client_t*)conn;

    co_tcp_stop_timer(client->conn.tcp_client);

    client->conn.module.close(client->conn.tcp_client);

    if (client->callbacks.on_close != NULL)
    {
        client->callbacks.on_close(thread, client);
    }
}

void
co_http_client_on_tcp_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    ssize_t receive_result =
        client->conn.module.receive_all(
            client->conn.tcp_client,
            client->conn.receive_data.ptr);

    co_tcp_restart_timer(client->conn.tcp_client);

    if (receive_result <= 0)
    {
        return;
    }

    size_t data_size =
        co_byte_array_get_count(client->conn.receive_data.ptr);

    while (data_size > client->conn.receive_data.index)
    {
        if (co_list_get_count(client->request_queue) == 0)
        {
            break;
        }

        if (client->response == NULL)
        {
            co_list_data_st* data =
                co_list_get_head(client->request_queue);
            client->request = (co_http_request_t*)data->value;
            client->response = co_http_response_create(0, NULL);

            if (client->response == NULL)
            {
                co_http_client_on_resopnse(
                    thread, client, CO_HTTP_ERROR_OUT_OF_MEMORY);

                return;
            }

            int result = co_http_response_deserialize(
                client->response, client->conn.receive_data.ptr,
                &client->conn.receive_data.index);

            if (result == CO_HTTP_PARSE_COMPLETE)
            {
                co_http_log_debug_response_header(
                    &client->conn.tcp_client->sock.local.net_addr, "<--",
                    &client->conn.tcp_client->sock.remote.net_addr,
                    client->response, "http receive response");

                co_http_content_receiver_clear(&client->content_receiver);

                if (!co_http_start_receive_content(
                    &client->content_receiver, client,
                    &client->response->message,
                    client->conn.receive_data.index))
                {
                    co_http_client_on_resopnse(
                        thread, client, CO_HTTP_ERROR_CANCEL);

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
            &client->content_receiver, client, client->conn.receive_data.ptr);

        if (result == CO_HTTP_PARSE_COMPLETE)
        {
            co_http_complete_receive_content(
                &client->content_receiver,
                &client->conn.receive_data.index,
                &client->response->message.data);

            co_http_client_on_resopnse(thread, client, 0);

            if (client->conn.tcp_client == NULL)
            {
                return;
            }
        }
        else if (result == CO_HTTP_PARSE_MORE_DATA)
        {
            co_http_content_more_data(
                &client->content_receiver,
                &client->conn.receive_data.index,
                client->conn.receive_data.ptr);

            return;
        }
        else if (result == CO_HTTP_PARSE_CANCEL)
        {
            co_http_client_on_resopnse(
                thread, client, CO_HTTP_ERROR_CANCEL);

            return;
        }
        else
        {
            co_http_client_on_resopnse(
                thread, client, CO_HTTP_ERROR_PARSE_CONTENT);

            return;
        }
    }

    if (co_list_get_count(client->request_queue) == 0)
    {
        client->conn.receive_data.index = 0;
        co_byte_array_clear(client->conn.receive_data.ptr);
    }
}

static void
co_http_client_on_tcp_receive_timer(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    co_http_log_error(
        &client->conn.tcp_client->sock.local.net_addr,
        "<--",
        &client->conn.tcp_client->sock.remote.net_addr,
        "receive timeout");

    co_http_client_on_resopnse(
        thread, client, CO_HTTP_ERROR_RECEIVE_TIMEOUT);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http_client_t*
co_http_client_create(
    const char* url_origin,
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

    co_url_st* url = co_url_create(url_origin);

    const char* protocols[1] = { CO_HTTP_PROTOCOL };

    if (!co_http_connection_setup(
        (co_http_connection_t*)client, url, local_net_addr,
        protocols, 1, tls_ctx))
    {
        co_url_destroy(url);
        co_mem_free(client);

        return NULL;
    }

    client->conn.tcp_client->callbacks.on_receive =
        (co_tcp_receive_fn)co_http_client_on_tcp_receive_ready;

    client->conn.callbacks.on_connect =
        (co_http_connection_connect_fn)
            co_http_client_on_http_connection_connect;
    client->conn.callbacks.on_close =
        (co_http_connection_close_fn)
            co_http_client_on_http_connection_close;

    co_http_client_setup(client);

    return client;
}

void
co_http_client_destroy(
    co_http_client_t* client
)
{
    if (client != NULL)
    {
        co_http_client_cleanup(client);
        co_http_connection_cleanup(&client->conn);

        co_mem_free_later(client);
    }
}

co_http_callbacks_st*
co_http_get_callbacks(
    co_http_client_t* client
)
{
    return &client->callbacks;
}

bool
co_http_start_connect(
    co_http_client_t* client
)
{
    return client->conn.module.connect(
        client->conn.tcp_client,
        &client->conn.tcp_client->sock.remote.net_addr);
}

void
co_http_close(
    co_http_client_t* client
)
{
    if ((client != NULL) &&
        (client->conn.tcp_client != NULL) &&
        (co_tcp_is_open(client->conn.tcp_client)))
    {
        client->conn.module.close(client->conn.tcp_client);
    }
}

bool
co_http_send_request(
    co_http_client_t* client,
    co_http_request_t* request
)
{
    if (!co_http_header_contains(
        &request->message.header, CO_HTTP_HEADER_HOST))
    {
        co_http_header_add_field(
            &request->message.header,
            CO_HTTP_HEADER_HOST,
            client->conn.url_origin->host_and_port);
    }

    bool result =
        co_http_connection_send_request(
            &client->conn, request);

    if (result)
    {
        co_list_add_tail(client->request_queue, request);
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
    co_tcp_log_debug(
        &client->conn.tcp_client->sock.local.net_addr,
        "-->",
        &client->conn.tcp_client->sock.remote.net_addr,
        "http send data %zd bytes", data_size);

    return co_http_connection_send_data(
        &client->conn, data, data_size);
}

bool
co_http_is_running(
    const co_http_client_t* client
)
{
    return (client != NULL && client->conn.tcp_client != NULL &&
        co_list_get_count(client->request_queue) > 0);
}

co_socket_t*
co_http_get_socket(
    co_http_client_t* client
)
{
    return ((client->conn.tcp_client != NULL) ?
        &client->conn.tcp_client->sock : NULL);
}

const char*
co_http_get_url_origin(
    const co_http_client_t* client
)
{
    return ((client->conn.url_origin != NULL) ?
        client->conn.url_origin->src : NULL);
}

bool
co_http_is_open(
    const co_http_client_t* client
)
{
    return ((client->conn.tcp_client != NULL) ?
        co_tcp_is_open(client->conn.tcp_client) : false);
}

void
co_http_set_user_data(
    co_http_client_t* client,
    void* user_data
)
{
    co_tcp_set_user_data(
        client->conn.tcp_client, user_data);
}

void*
co_http_get_user_data(
    const co_http_client_t* client
)
{
    return co_tcp_get_user_data(
        client->conn.tcp_client);
}
