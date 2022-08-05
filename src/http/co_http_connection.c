#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/tls/co_tls_client.h>

#include <coldforce/http/co_http_client.h>
#include <coldforce/http/co_http_log.h>

//---------------------------------------------------------------------------//
// http connection
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

bool
co_http_connection_setup(
    co_http_connection_t* conn,
    co_url_st* base_url,
    const co_net_addr_t* local_net_addr,
    const char** protocols,
    size_t protocol_count,
    co_tls_ctx_st* tls_ctx
)
{
    if (base_url->host == NULL)
    {
        return false;
    }

    bool tls_scheme = false;

    if (base_url->scheme == NULL)
    {
        base_url->scheme = co_string_duplicate("http");
    }
    else if (
        co_string_case_compare(
            base_url->scheme, "https") == 0)
    {
        tls_scheme = true;
    }

    if (tls_scheme)
    {
#ifdef CO_CAN_USE_TLS
        conn->module.destroy = co_tls_client_destroy;
        conn->module.close = co_tls_close;
        conn->module.connect = co_tls_connect;
        conn->module.send = co_tls_send;
        conn->module.receive_all = co_tls_receive_all;

        conn->tcp_client =
            co_tls_client_create(local_net_addr, tls_ctx);

        if (conn->tcp_client != NULL &&
            protocols != NULL && protocol_count > 0)
        {
            co_tls_set_host_name(
                conn->tcp_client, base_url->host);

            co_tls_set_available_protocols(
                conn->tcp_client, protocols, protocol_count);
        }
#else
        (void)protocols;
        (void)protocol_count;
        (void)tls_ctx;

        co_http_log_error(NULL, NULL, NULL,
            "OpenSSL is not installed");

        return false;
#endif
    }
    else
    {
        conn->module.destroy = co_tcp_client_destroy;
        conn->module.close = co_tcp_close;
        conn->module.connect = co_tcp_connect;
        conn->module.send = co_tcp_send;
        conn->module.receive_all = co_tcp_receive_all;

        conn->tcp_client =
            co_tcp_client_create(local_net_addr);
    }

    if (conn->tcp_client == NULL)
    {
        return false;
    }

    int address_family =
        co_net_addr_get_family(local_net_addr);

    co_net_addr_init(&conn->tcp_client->remote_net_addr);

    if (!co_url_to_net_addr(
        base_url, address_family,
        &conn->tcp_client->remote_net_addr))
    {
        co_http_log_error(NULL, NULL, NULL,
            "failed to resolve hostname (%s)", base_url->src);

        return false;
    }

    conn->base_url = base_url;
    conn->tcp_client->sock.sub_class = conn;
    conn->receive_data.index = 0;
    conn->receive_data.ptr = co_byte_array_create();
    conn->receive_timer = NULL;

    return true;
}

void
co_http_connection_cleanup(
    co_http_connection_t* conn
)
{
    if (conn != NULL)
    {
        co_byte_array_destroy(conn->receive_data.ptr);
        conn->receive_data.ptr = NULL;

        co_timer_destroy(conn->receive_timer);
        conn->receive_timer = NULL;

        co_url_destroy(conn->base_url);
        conn->base_url = NULL;

        if (conn->tcp_client != NULL)
        {
            conn->module.destroy(conn->tcp_client);
            conn->tcp_client = NULL;
        }
    }
}

void
co_tcp_upgrade_to_http_connection(
    co_tcp_client_t* tcp_client,
    co_http_connection_t* conn,
    co_url_st* base_url
)
{
#ifdef CO_CAN_USE_TLS
    if (tcp_client->sock.tls != NULL)
    {
        conn->module.destroy = co_tls_client_destroy;
        conn->module.close = co_tls_close;
        conn->module.connect = co_tls_connect;
        conn->module.send = co_tls_send;
        conn->module.receive_all = co_tls_receive_all;
    }
    else
    {
#endif
        conn->module.destroy = co_tcp_client_destroy;
        conn->module.close = co_tcp_close;
        conn->module.connect = co_tcp_connect;
        conn->module.send = co_tcp_send;
        conn->module.receive_all = co_tcp_receive_all;

#ifdef CO_CAN_USE_TLS
    }
#endif

    conn->tcp_client = tcp_client;
    conn->tcp_client->sock.sub_class = conn;
    conn->base_url = base_url;
    conn->receive_data.index = 0;
    conn->receive_data.ptr = co_byte_array_create();
    conn->receive_timer = NULL;
}

void
co_http_connection_move(
    co_http_connection_t* from_conn,
    co_http_connection_t* to_conn
)
{
    *to_conn = *from_conn;

    to_conn->tcp_client->sock.sub_class = to_conn;
    to_conn->receive_timer = NULL;

    from_conn->tcp_client = NULL;
    from_conn->base_url = NULL;
    from_conn->receive_data.ptr = NULL;
}

bool
co_http_connection_is_server(
    const co_http_connection_t* conn
)
{
    return (conn->base_url == NULL);
}

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
            co_url_create_host_and_port(conn->base_url);

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
