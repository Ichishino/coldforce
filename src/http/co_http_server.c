#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/tls/co_tls_client.h>
#include <coldforce/tls/co_tls_server.h>

#include <coldforce/http/co_http_string_list.h>
#include <coldforce/http/co_http_server.h>
#include <coldforce/http/co_http_client.h>
#include <coldforce/http/co_http_config.h>
#include <coldforce/http/co_http_log.h>

//---------------------------------------------------------------------------//
// http server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static void
co_http_server_on_request(
    co_thread_t* thread,
    co_http_client_t* client,
    int error_code
)
{
    co_http_request_t* request = client->request;

    if (client->callbacks.on_receive_finish != NULL)
    {
        client->callbacks.on_receive_finish(
            thread, client, request, NULL, error_code);
    }

    co_http_request_destroy(client->request);
    client->request = NULL;
}

static void
co_http_server_on_receive_ready(
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
        if (client->request == NULL)
        {
            client->request = co_http_request_create();

            if (client->request == NULL)
            {
                co_http_server_on_request(
                    thread, client, CO_HTTP_ERROR_OUT_OF_MEMORY);

                return;
            }

            int result = co_http_request_deserialize(
                client->request, client->receive_data,
                &client->receive_data_index);

            if (result == CO_HTTP_PARSE_COMPLETE)
            {
                co_http_log_debug_request_header(
                    &client->tcp_client->sock.local_net_addr, "<--",
                    &client->tcp_client->remote_net_addr,
                    client->request, "http receive request");

                co_http_content_receiver_clear(&client->content_receiver);

                if (!co_http_start_receive_content(
                    &client->content_receiver, client,
                    &client->request->message,
                    client->receive_data_index))
                {
                    return;
                }

                if ((!client->content_receiver.chunked) &&
                    (client->content_receiver.size >
                        co_http_config_get_max_receive_content_size()))
                {
                    co_http_server_on_request(
                        thread, client, CO_HTTP_ERROR_CONTENT_TOO_BIG);

                    return;
                }
            }
            else if (result == CO_HTTP_PARSE_MORE_DATA)
            {
                co_http_request_destroy(client->request);
                client->request = NULL;

                return;
            }
            else
            {
                co_http_server_on_request(thread, client, result);

                return;
            }
        }

        int result = co_http_receive_content_data(
            &client->content_receiver, client, client->receive_data);

        if (result == CO_HTTP_PARSE_COMPLETE)
        {
            co_http_complete_receive_content(
                &client->content_receiver,
                &client->receive_data_index,
                &client->request->message.data);

            co_http_server_on_request(thread, client, 0);

            if (client->tcp_client == NULL)
            {
                return;
            }
        }
        else if (result == CO_HTTP_PARSE_MORE_DATA)
        {
            co_http_content_more_data(
                &client->content_receiver, client->receive_data);

            return;
        }
        else if (result == CO_HTTP_PARSE_CANCEL)
        {
            co_http_server_on_request(thread, client, CO_HTTP_ERROR_CANCEL);

            return;
        }
        else
        {
            co_http_server_on_request(thread, client, result);

            return;
        }
    }

    client->receive_data_index = 0;
    co_byte_array_clear(client->receive_data);
}

static void
co_http_server_on_close(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* client =
        (co_http_client_t*)tcp_client->sock.sub_class;

    if (client->request != NULL)
    {
        co_http_server_on_request(
            thread, client, CO_HTTP_ERROR_CONNECTION_CLOSED);
    }

    if (client->callbacks.on_close != NULL)
    {
        client->callbacks.on_close(thread, client);
    }
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http_client_t*
co_http_client_create_with(
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* client =
        (co_http_client_t*)co_mem_alloc(sizeof(co_http_client_t));

    if (client == NULL)
    {
        return NULL;
    }

    client->base_url = NULL;
    client->tcp_client = tcp_client;

    co_http_client_setup(client);

    co_tcp_set_receive_handler(
        client->tcp_client,
        (co_tcp_receive_fn)co_http_server_on_receive_ready);
    co_tcp_set_close_handler(
        client->tcp_client,
        (co_tcp_close_fn)co_http_server_on_close);

    return client;
}

bool
co_http_send_response(
    co_http_client_t* client,
    co_http_response_t* response
)
{
    size_t content_size =
        co_http_response_get_data_size(response);

    if (content_size > 0)
    {
        if (!co_http_header_contains(
            &response->message.header, CO_HTTP_HEADER_CONTENT_LENGTH))
        {
            co_http_header_set_content_length(
                &response->message.header, content_size);
        }
    }

    co_http_response_set_version(response, CO_HTTP_VERSION_1_1);

    co_http_log_debug_response_header(
        &client->tcp_client->sock.local_net_addr, "-->",
        &client->tcp_client->remote_net_addr,
        response, "http send response");

    co_byte_array_t* buffer = co_byte_array_create();

    co_http_response_serialize(response, buffer);

    bool result = 
        co_http_send_raw_data(client,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    co_http_response_destroy(response);

    return result;
}

bool
co_http_begin_chunked_response(
    co_http_client_t* client,
    co_http_response_t* response
)
{
    const char* transfer_encoding =
        co_http_header_get_field(&response->message.header,
            CO_HTTP_HEADER_TRANSFER_ENCODING);

    if (transfer_encoding == NULL)
    {
        co_http_header_add_field(
            &response->message.header,
            CO_HTTP_HEADER_TRANSFER_ENCODING,
            CO_HTTP_TRANSFER_ENCODING_CHUNKED);
    }
    else
    {
        co_http_string_item_st items[32];

        size_t count = co_http_string_list_parse(
            transfer_encoding, items, 32);

        if (!co_http_string_list_contains(
            items, count,
            CO_HTTP_TRANSFER_ENCODING_CHUNKED))
        {
            char new_item[1024];

            strcpy(new_item, transfer_encoding);
            strcat(new_item, ", ");
            strcat(new_item, CO_HTTP_TRANSFER_ENCODING_CHUNKED);

            co_http_header_set_field(
                &response->message.header,
                CO_HTTP_HEADER_TRANSFER_ENCODING,
                new_item);
        }

        co_http_string_list_cleanup(items, count);
    }

    co_http_response_set_version(response, CO_HTTP_VERSION_1_1);

    co_http_log_debug_response_header(
        &client->tcp_client->sock.local_net_addr, "-->",
        &client->tcp_client->remote_net_addr,
        response, "http send response");

    co_byte_array_t* buffer = co_byte_array_create();

    co_http_response_serialize(response, buffer);

    bool result =
        co_http_send_raw_data(client,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    co_http_response_destroy(response);

    return result;
}

bool
co_http_send_chunked_response(
    co_http_client_t* client,
    const void* data,
    size_t data_length
)
{
    co_byte_array_t* buffer = co_byte_array_create();

    char chunk_size[64];
    sprintf(chunk_size, "%x"CO_HTTP_CRLF, (unsigned int)data_length);

    co_byte_array_add_string(buffer, chunk_size);

    if (data_length > 0)
    {
        co_byte_array_add(buffer, data, data_length);
    }

    co_byte_array_add_string(buffer, CO_HTTP_CRLF);

    co_http_log_debug(NULL, NULL, NULL,
        "http send chunked data %zd", data_length);

    bool result =
        co_http_send_raw_data(client,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    return result;
}

bool
co_http_end_chunked_response(
    co_http_client_t* client
)
{
    return co_http_send_chunked_response(client, NULL, 0);
}
