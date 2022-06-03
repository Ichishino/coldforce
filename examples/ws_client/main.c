#include <coldforce/coldforce_ws.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_ws_client_t* client;

} my_app;

void on_my_ws_receive(my_app* self, co_ws_client_t* client, const co_ws_frame_t* frame, int error_code)
{
    if (error_code == 0)
    {
        bool fin = co_ws_frame_is_fin(frame);
        uint8_t opcode = co_ws_frame_get_opcode(frame);
        size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
        const uint8_t* data = co_ws_frame_get_payload_data(frame);

        switch (opcode)
        {
        case CO_WS_OPCODE_TEXT:
        {
            printf("receive text(%d): %*.*s\n", fin, (int)data_size, (int)data_size, (char*)data);

            break;
        }
        case CO_WS_OPCODE_BINARY:
        {
            printf("receive binary(%d): %zu bytes\n", fin, data_size);

            break;
        }
        case CO_WS_OPCODE_CONTINUATION:
        {
            printf("receive continuation(%d): %zu bytes\n", fin, data_size);

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
        printf("receive error: %d\n", error_code);

        // close
        co_ws_client_destroy(client);
        self->client = NULL;
    }
}

void on_my_ws_close(my_app* self, co_ws_client_t* client)
{
    printf("close\n");

    // close
    co_ws_client_destroy(client);
    self->client = NULL;

    // quit app
    co_net_app_stop();
}

void on_my_connect(my_app* self, co_ws_client_t* client, const co_http_response_t* response, int error_code)
{
    printf("connect: %d\n", error_code);

    if (error_code == 0)
    {
        co_http_response_print_header(response);

        bool upgrade_result = false;

        if (co_http_response_validate_ws_upgrade(response, client, &upgrade_result))
        {
            if (upgrade_result)
            {
                printf("upgrade success\n");

                // send
                co_ws_send_text(client, "hello");

                return;
            }
            else
            {
                printf("upgrade refused\n");
            }
        }
        else
        {
            printf("invalid upgrade response\n");
        }
    }
    else
    {
        printf("connect error\n");
    }

    // close
    co_ws_client_destroy(client);
    self->client = NULL;

    // quit app
    co_net_app_stop();
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    const char* base_url = "ws://127.0.0.1:9080";
    const char* file_path = "/";

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    self->client = co_ws_client_create(base_url, &local_net_addr, NULL);

    if (self->client == NULL)
    {
        return false;
    }

    co_ws_set_receive_handler(self->client, (co_ws_receive_fn)on_my_ws_receive);
    co_ws_set_close_handler(self->client, (co_ws_close_fn)on_my_ws_close);

    co_http_request_t* request = co_http_request_create_ws_upgrade(file_path, NULL, NULL);

    // connect
    co_ws_connect(self->client, request, (co_ws_connect_fn)on_my_connect);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_ws_client_destroy(self->client);
}

int main(int argc, char* argv[])
{
    co_tls_setup();

    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}
