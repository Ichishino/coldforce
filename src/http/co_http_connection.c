#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_client.h>
#include <coldforce/http/co_http_log.h>

//---------------------------------------------------------------------------//
// http connection
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

bool
co_http_connection_send_request(
    co_http_connection_t* conn,
    co_http_request_t* request
)
{
    if (!co_http_header_contains(
        &request->message.header, CO_HTTP_HEADER_HOST))
    {
        char* host =
            co_http_url_create_host_and_port(conn->base_url);

        co_http_header_add_field(
            &request->message.header,
            CO_HTTP_HEADER_HOST, host);

        co_string_destroy(host);
    }

    if ((request->message.data.size > 0) &&
        !co_http_header_contains(
            &request->message.header, CO_HTTP_HEADER_CONTENT_LENGTH))
    {
        co_http_header_set_content_length(
            &request->message.header,
            request->message.data.size);
    }

    co_http_log_debug_request_header(
        &conn->tcp_client->sock.local_net_addr, "-->",
        &conn->tcp_client->remote_net_addr,
        request, "http send request");

    co_byte_array_t* buffer = co_byte_array_create();

    co_http_request_serialize(request, buffer);

    bool result =
        co_http_connection_send_data(conn,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    if (result)
    {
        if (conn->request_queue != NULL)
        {
            co_list_add_tail(conn->request_queue, request);
        }
        else
        {
            co_http_request_destroy(request);
        }

        if (conn->receive_timer != NULL)
        {
            if (!co_timer_is_running(conn->receive_timer))
            {
                co_timer_start(conn->receive_timer);
            }
        }
    }

    return result;
}

bool
co_http_connection_send_response(
    co_http_connection_t* conn,
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

    co_http_log_debug_response_header(
        &conn->tcp_client->sock.local_net_addr, "-->",
        &conn->tcp_client->remote_net_addr,
        response, "http send response");

    co_byte_array_t* buffer = co_byte_array_create();

    co_http_response_serialize(response, buffer);

    bool result =
        co_http_connection_send_data(conn,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    co_http_response_destroy(response);

    return result;
}

bool
co_http_connection_send_data(
    co_http_connection_t* conn,
    const void* data,
    size_t data_size
)
{
    return conn->module.send(
        conn->tcp_client, data, data_size);
}
