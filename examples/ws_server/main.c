#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <coldforce/coldforce_ws.h>

#include <string.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http_server_t* server;
    co_list_t* clients;

} my_app;

#define my_client_log(protocol, client, str) \
    do { \
        char remote_str[64]; \
        co_net_addr_to_string( \
            co_##protocol##_get_remote_net_addr(client), remote_str, sizeof(remote_str)); \
        printf("%s: %s\n", str, remote_str); \
    } while(0)

void on_my_ws_receive(my_app* self, co_ws_client_t* client, const co_ws_frame_t* frame, int error_code)
{
    my_client_log(ws, client, "receive");

    if (error_code == 0)
    {
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
        my_client_log(ws, client, "close (received invalid data)");

        // close
        co_list_remove(self->clients, (uintptr_t)client);
    }
}

void on_my_ws_close(my_app* self, co_ws_client_t* client)
{
    my_client_log(ws, client, "close");

    co_list_remove(self->clients, (uintptr_t)client);
}

void on_my_tcp_accept(my_app* self, co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    (void)tcp_server;

    my_client_log(tcp, tcp_client, "accept");

    co_tcp_accept((co_thread_t*)self, tcp_client);

    // create websocket client
    co_ws_client_t* ws_client = co_ws_client_create_with(tcp_client);

    co_ws_set_receive_handler(ws_client, (co_ws_receive_fn)on_my_ws_receive);
    co_ws_set_close_handler(ws_client, (co_ws_close_fn)on_my_ws_close);

    co_list_add_tail(self->clients, (uintptr_t)ws_client);
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    if (arg->argc <= 1)
    {
        printf("<Usage>\n");
        printf("ws_server port_number\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(arg->argv[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_item_free_fn)co_ws_client_destroy;
    self->clients = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_http_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_http_server_get_socket(self->server), true);

    // listen start
    co_http_server_start(self->server,
        (co_tcp_accept_fn)on_my_tcp_accept, SOMAXCONN);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("listen %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_http_server_destroy(self->server);
    co_list_destroy(self->clients);
}

int main(int argc, char* argv[])
{
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_ws_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    return exit_code;
}
