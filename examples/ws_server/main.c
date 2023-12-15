#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

//---------------------------------------------------------------------------//
// app object
//---------------------------------------------------------------------------//

typedef struct
{
    co_app_t base_app;

    // app data
    co_tcp_server_t* tcp_server;
    co_list_t* ws_clients;

} app_st;

#define app_get_remote_address(protocol, net_unit, buffer) \
    co_net_addr_to_string( \
        co_socket_get_remote_net_addr( \
            co_##protocol##_get_socket(net_unit)), \
        buffer, sizeof(buffer));

//---------------------------------------------------------------------------//
// websocket callback
//---------------------------------------------------------------------------//

void
app_on_ws_receive_frame(
    app_st* self,
    co_ws_client_t* ws_client,
    const co_ws_frame_t* frame,
    int error_code
)
{
    char remote_str[64];
    app_get_remote_address(ws, ws_client, remote_str);

    if (error_code == 0)
    {
        printf("receive frame: %s\n", remote_str);

        bool fin = co_ws_frame_get_fin(frame);
        uint8_t opcode = co_ws_frame_get_opcode(frame);
        size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
        const uint8_t* data = co_ws_frame_get_payload_data(frame);

        printf("frame: fin(%d) opcode(%d) data_size(%zu)\n", fin, opcode, data_size);

        switch (opcode)
        {
        case CO_WS_OPCODE_TEXT:
        case CO_WS_OPCODE_BINARY:
        case CO_WS_OPCODE_CONTINUATION:
        {
            // echo
            co_ws_send(ws_client, fin, opcode, data, (size_t)data_size);

            break;
        }
        default:
        {
            co_ws_default_handler(ws_client, frame);

            break;
        }
        }
    }
    else
    {
        printf("received invalid data: %s\n", remote_str);

        // close
        co_list_remove(self->ws_clients, ws_client);
    }
}

void
app_on_ws_close(
    app_st* self,
    co_ws_client_t* ws_client)
{
    char remote_str[64];
    app_get_remote_address(ws, ws_client, remote_str);
    printf("closed: %s\n", remote_str);

    co_list_remove(self->ws_clients, ws_client);
}

void
app_on_ws_upgrade(
    app_st* self,
    co_ws_client_t* ws_client,
    const co_http_request_t* request,
    int error_code
)
{
    char remote_str[64];
    app_get_remote_address(ws, ws_client, remote_str);

    if (error_code == 0)
    {
        printf("receive upgrade request: %s\n", remote_str);

        co_http_response_t* response =
            co_http_response_create_ws_upgrade(
                request, NULL, NULL);

        // send upgrade response
        co_http_connection_send_response(
            (co_http_connection_t*)ws_client, response);

        co_http_response_destroy(response);
    }
    else
    {
        printf("receive invalid upgrade request: %s\n", remote_str);

        co_list_remove(self->ws_clients, ws_client);
    }
}

//---------------------------------------------------------------------------//
// tcp callback
//---------------------------------------------------------------------------//

void
app_on_tcp_accept(
    app_st* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
)
{
    (void)tcp_server;

    char remote_str[64];
    app_get_remote_address(tcp, tcp_client, remote_str);
    printf("tcp accept: %s\n", remote_str);

    // tcp accept
    co_tcp_accept((co_thread_t*)self, tcp_client);

    // upgrade to websocket
    co_ws_client_t* ws_client = co_tcp_upgrade_to_ws(tcp_client, NULL);

    // callbacks
    co_ws_callbacks_st* callbacks = co_ws_get_callbacks(ws_client);
    callbacks->on_upgrade = (co_ws_upgrade_fn)app_on_ws_upgrade;
    callbacks->on_receive_frame = (co_ws_receive_frame_fn)app_on_ws_receive_frame;
    callbacks->on_close = (co_ws_close_fn)app_on_ws_close;

    co_list_add_tail(self->ws_clients, ws_client);
}

//---------------------------------------------------------------------------//
// app callback
//---------------------------------------------------------------------------//

bool
app_on_create(
    app_st* self
)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count <= 1)
    {
        printf("<Usage>\n");
        printf("ws_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_ws_client_destroy;
    self->ws_clients = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    // create tcp server
    self->tcp_server = co_tcp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->tcp_server), true);

    // callbacks
    co_tcp_server_callbacks_st* callbacks =
        co_tcp_server_get_callbacks(self->tcp_server);
    callbacks->on_accept = (co_tcp_accept_fn)app_on_tcp_accept;

    // start listen
    co_tcp_server_start(self->tcp_server, SOMAXCONN);

    printf("start server ws://127.0.0.1:%d\n", port);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_tcp_server_destroy(self->tcp_server);
    co_list_destroy(self->ws_clients);
}

void
app_on_signal(
    int sig
)
{
    (void)sig;

    // quit app
    co_app_stop();
}

//---------------------------------------------------------------------------//
// main
//---------------------------------------------------------------------------//

int
main(
    int argc,
    char* argv[]
)
{
    co_win_debug_crt_set_flags();

    signal(SIGINT, app_on_signal);

//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_ws_log_set_level(CO_LOG_LEVEL_MAX);

    // app instance
    app_st self = { 0 };

    // start app
    return co_net_app_start(
        (co_app_t*)&self, "ws-server-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
