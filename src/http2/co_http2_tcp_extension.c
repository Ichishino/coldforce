#include <coldforce/core/co_std.h>

#include <coldforce/http/co_http_tcp_extension.h>

#include <coldforce/http2/co_http2_tcp_extension.h>
#include <coldforce/http2/co_http2_server.h>
#include <coldforce/http2/co_http2_client.h>

//---------------------------------------------------------------------------//
// tcp extension for http2
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http2_client_t*
co_tcp_upgrade_to_http2(
    co_tcp_client_t* tcp_client,
    const char* url_origin
)
{
    co_http2_client_t* client =
        (co_http2_client_t*)co_mem_alloc(sizeof(co_http2_client_t));

    if (client == NULL)
    {
        return NULL;
    }

    if (!co_tcp_upgrade_to_http_connection(
        tcp_client, &client->conn, url_origin))
    {
        co_mem_free(client);

        return NULL;
    }

    co_http2_client_setup(client);

    if (co_http_connection_is_server(&client->conn))
    {
        client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_http2_server_on_tcp_receive_ready;
    }
    else
    {
        client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_http2_client_on_tcp_receive_ready;
    }

    client->conn.tcp_client->callbacks.on_close =
        (co_tcp_close_fn)co_http2_client_on_tcp_close;

    return client;
}
