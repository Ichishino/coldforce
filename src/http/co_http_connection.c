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
    const co_http_request_t* request
)
{
    if (!co_http_header_contains(
        &request->message.header, CO_HTTP_HEADER_HOST))
    {
        char* host =
            co_http_url_create_host_and_port(conn->base_url);

        co_http_header_add_field(
            (co_http_header_t*)&request->message.header,
            CO_HTTP_HEADER_HOST, host);

        co_string_destroy(host);
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
    const co_http_response_t* response
)
{
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
