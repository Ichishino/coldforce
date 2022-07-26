#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_tcp_server_t* server;
    co_list_t* clients;

} my_app;

void on_my_ws_receive_frame(my_app* self, co_ws_client_t* client, const co_ws_frame_t* frame, int error_code)
{
    if (error_code == 0)
    {
        printf("receive frame\n");

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
            co_ws_send(client, fin, opcode, data, (size_t)data_size);

            break;
        }
        default:
        {
            co_ws_default_handler(client, frame);

            break;
        }
        }
    }
    else
    {
        printf("received invalid data\n");

        // close
        co_list_remove(self->clients, client);
    }
}

void on_my_ws_close(my_app* self, co_ws_client_t* client)
{
    printf("closed\n");

    co_list_remove(self->clients, client);
}

void on_my_ws_upgrade(my_app* self, co_ws_client_t* client, const co_http_request_t* request, int error_code)
{
    if (error_code == 0)
    {
        printf("receive upgrade request\n");

        co_http_response_t* response =
            co_http_response_create_ws_upgrade(
                request, NULL, NULL);

        co_http_connection_send_response(
            (co_http_connection_t*)client, response);

        co_http_response_destroy(response);
    }
    else
    {
        printf("receive invalid upgrade request\n");

        co_list_remove(self->clients, client);
    }
}

void on_my_tcp_accept(my_app* self, co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    (void)tcp_server;

    printf("accept\n");

    co_tcp_accept((co_thread_t*)self, tcp_client);

    // create websocket client
    co_ws_client_t* ws_client = co_ws_client_create_with(tcp_client);

    // callback
    co_ws_callbacks_st* callbacks = co_ws_get_callbacks(ws_client);
    callbacks->on_upgrade = (co_ws_upgrade_fn)on_my_ws_upgrade;
    callbacks->on_receive_frame = (co_ws_receive_frame_fn)on_my_ws_receive_frame;
    callbacks->on_close = (co_ws_close_fn)on_my_ws_close;

    co_list_add_tail(self->clients, ws_client);
}

bool on_my_app_create(my_app* self)
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
    self->clients = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_tcp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    // callback
    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_my_tcp_accept;

    // listen start
    co_tcp_server_start(self->server, SOMAXCONN);

    printf("ws://127.0.0.1:%d\n", port);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->clients);
    co_tcp_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_ws_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);

    // run
    int exit_code = co_app_run((co_app_t*)&app);

    co_net_app_cleanup((co_app_t*)&app);

    return exit_code;
}
