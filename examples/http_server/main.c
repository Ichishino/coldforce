#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

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
    co_list_t* http_clients;

} app_st;

#define app_get_remote_address(protocol, net_unit, buffer) \
    co_net_addr_to_string( \
        co_socket_get_remote_net_addr( \
            co_##protocol##_get_socket(net_unit)), \
        buffer, sizeof(buffer));

//---------------------------------------------------------------------------//
// http callback
//---------------------------------------------------------------------------//

void
app_on_http_request(
    app_st* self,
    co_http_client_t* http_client,
    const co_http_request_t* request,
    const co_http_response_t* unused,
    int error_code
)
{
    (void)unused;

    char remote_addr_str[64];
    app_get_remote_address(http, http_client, remote_addr_str);
    printf("http request: %s\n", remote_addr_str);

    if (error_code == 0)
    {
        const co_url_st* url = co_http_request_get_url(request);

        co_http_response_t* response = co_http_response_create(200, "OK");
        co_http_header_t* response_header = co_http_response_get_header(response);

        co_http_header_add_field(response_header, "Content-Type", "text/html");
        co_http_header_add_field(response_header, "Cache-Control", "no-store");

        const char* response_content;

        if (strcmp(url->path, "/stop") == 0)
        {
            // server stop request

            response_content =
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head><title>Coldforce Http Server</title></head>\n"
                "<body>Server Stopped</body>\n"
                "</html>\n";

            // quit app (later)
            co_app_stop();
        }
        else
        {
            response_content =
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head><title>Coldforce Http Server</title></head>\n"
                "<body>Hello</body>\n"
                "</html>\n";
        }

        size_t response_content_size = (uint32_t)strlen(response_content);
        co_http_header_set_content_length(response_header, response_content_size);

        // send response
        co_http_send_response(http_client, response);
        co_http_send_data(http_client, response_content, response_content_size);
    }
    else
    {
        co_http_response_t* response =
            co_http_response_create(400, "Bad Request");

        co_http_send_response(http_client, response);
    }
    
    // close client
    co_list_remove(self->http_clients, http_client);
}

void
app_on_http_close(
    app_st* self,
    co_http_client_t* http_client
)
{
    (void)self;

    char remote_addr_str[64];
    app_get_remote_address(http, http_client, remote_addr_str);
    printf("http closed: %s\n", remote_addr_str);

    co_list_remove(self->http_clients, http_client);
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

    char remote_addr_str[64];
    app_get_remote_address(tcp, tcp_client, remote_addr_str);
    printf("tcp accept: %s\n", remote_addr_str);

    // accept
    co_tcp_accept((co_thread_t*)self, tcp_client);

    // upgrade to http client
    co_http_client_t* http_client =
        co_tcp_upgrade_to_http(tcp_client, NULL);

    // callbacks
    co_http_callbacks_st* callbacks = co_http_get_callbacks(http_client);
    callbacks->on_receive_finish = (co_http_receive_finish_fn)app_on_http_request;
    callbacks->on_close = (co_http_close_fn)app_on_http_close;

    co_list_add_tail(self->http_clients, http_client);
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
        printf("http_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    // client lists
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_http_client_destroy;
    self->http_clients = co_list_create(&list_ctx);

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
    if (!co_tcp_server_start(self->tcp_server, SOMAXCONN))
    {
        return false;
    }

    printf("start server: http://127.0.0.1:%d\n", port);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_tcp_server_destroy(self->tcp_server);
    co_list_destroy(self->http_clients);
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
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    // app instance
    app_st self = { 0 };

    // start app
    return co_net_app_start(
        (co_app_t*)&self, "http-server-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
