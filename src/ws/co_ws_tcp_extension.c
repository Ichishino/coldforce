#include <coldforce/core/co_std.h>

#include <coldforce/http/co_http_tcp_extension.h>

#include <coldforce/ws/co_ws_tcp_extension.h>
#include <coldforce/ws/co_ws_server.h>
#include <coldforce/ws/co_ws_client.h>

//---------------------------------------------------------------------------//
// tcp extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_ws_client_t*
co_tcp_upgrade_to_ws(
    co_tcp_client_t* tcp_client,
    const char* url_origin
)
{
    co_ws_client_t* client =
        (co_ws_client_t*)co_mem_alloc(sizeof(co_ws_client_t));

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

    bool is_server =
        co_http_connection_is_server(&client->conn);

    if (is_server)
    {
        client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_ws_server_on_tcp_receive_ready;
    }
    else
    {
        client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_ws_client_on_tcp_receive_ready;
    }

    client->conn.callbacks.on_close =
        (co_http_connection_close_fn)
            co_ws_client_on_http_connection_close;

    co_ws_client_setup(client);

    client->mask = !is_server;

    return client;
}
