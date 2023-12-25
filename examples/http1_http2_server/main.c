#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#   ifdef CO_USE_WOLFSSL
#       pragma comment(lib, "wolfssl.lib")
#   elif defined(CO_USE_OPENSSL)
#       pragma comment(lib, "libssl.lib")
#       pragma comment(lib, "libcrypto.lib")
#   endif
#endif

//---------------------------------------------------------------------------//
// app object
//---------------------------------------------------------------------------//

typedef struct
{
    co_app_t base_app;

    // app data
    co_tcp_server_t* tcp_server;
    co_list_t* tcp_clients;
    co_list_t* http_clients;
    co_list_t* http2_clients;

} app_st;

#define app_get_remote_address(protocol, net_unit, buffer) \
    co_net_addr_to_string( \
        co_socket_get_remote_net_addr( \
            co_##protocol##_get_socket(net_unit)), \
        buffer, sizeof(buffer));

#define app_client_log(protocol, client, text) \
    do { \
        char remote_str[64]; \
        app_get_remote_address(protocol, client, remote_str); \
        printf("%s: %s\n", text, remote_str); \
    } while(0)

//---------------------------------------------------------------------------//
// http2 callback
//---------------------------------------------------------------------------//

void
app_on_http2_request(
    app_st* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_http2_header_t* request_header,
    const co_http2_data_st* request_data,
    int error_code
)
{
    app_client_log(http2, http2_client, "http2 request");

    if (error_code == 0)
    {
        const co_url_st* url = co_http2_header_get_path_url(request_header);

        co_http2_header_t* response_header = co_http2_header_create_response(200);
        co_http2_header_add_field(response_header, "content-type", "text/html");
        co_http2_header_add_field(response_header, "cache-control", "no-store");

        const char* response_content;

        if (strcmp(url->path, "/stop") == 0)
        {
            // server stop request

            response_content =
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head><title>Coldforce Http Server</title></head>\n"
                "<body>http2 Server Stopped</body>\n"
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
                "<body>http2 Hello</body>\n"
                "</html>\n";
        }

        uint32_t response_content_size = (uint32_t)strlen(response_content);

        // send response
        co_http2_stream_send_header(stream, false, response_header);
        co_http2_stream_send_data(stream, true, response_content, response_content_size);
    }
    else
    {
        // error close
        co_list_remove(self->http2_clients, http2_client);
    }
}

void
app_on_http2_close(
    app_st* self,
    co_http2_client_t* http2_client,
    int error_code
)
{
    (void)error_code;

    app_client_log(http2, http2_client, "http2 closed");

    co_list_remove(self->http2_clients, http2_client);
}

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
    (void)request;
    (void)unused;

    app_client_log(http, http_client, "http request");

    if (error_code == 0)
    {
        co_http_response_t* response =
            co_http_response_create(200, "OK");

        co_http_header_t* response_header = co_http_response_get_header(response);

        co_http_header_add_field(response_header, "Content-Type", "text/html");
        co_http_header_add_field(response_header, "Cache-Control", "no-store");

        const char* content =
            "<html><head><title>Coldforce Http Server</title></head>"
                "<body>http/1.1 Hello</body></html>";
        size_t content_size = strlen(content);

        co_http_header_set_content_length(response_header, content_size);

        // send response
        co_http_send_response(http_client, response);
        co_http_send_data(http_client, content, content_size);
    }

    // close
    co_list_remove(self->http_clients, http_client);
}

void
app_on_http_close(
    app_st* self,
    co_http_client_t* http_client
)
{
    app_client_log(http, http_client, "http closed");

    co_list_remove(self->http_clients, http_client);
}

//---------------------------------------------------------------------------//
// tls callback
//---------------------------------------------------------------------------//

void
app_on_tls_handshake(
    app_st* self,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    co_list_remove(self->tcp_clients, tcp_client);

    if (error_code == 0)
    {
        app_client_log(tcp, tcp_client, "tls handshake success");

        char protocol[32] = { 0 };
        co_tls_tcp_get_selected_protocol(
            tcp_client, protocol, sizeof(protocol));

        if (strcmp(protocol, CO_HTTP2_PROTOCOL) == 0)
        {
            // http2

            // upgrade to http2
            co_http2_client_t* http2_client = co_tcp_upgrade_to_http2(tcp_client, NULL);

            // callbacks
            co_http2_callbacks_st* callbacks = co_http2_get_callbacks(http2_client);
            callbacks->on_receive_finish = (co_http2_receive_finish_fn)app_on_http2_request;
            callbacks->on_close = (co_http2_close_fn)app_on_http2_close;

            co_list_add_tail(self->http2_clients, http2_client);
        }
        else
        {
            // http

            // upgrade to http
            co_http_client_t* http_client = co_tcp_upgrade_to_http(tcp_client, NULL);

            // callbacks
            co_http_callbacks_st* callbacks = co_http_get_callbacks(http_client);
            callbacks->on_receive_finish = (co_http_receive_finish_fn)app_on_http_request;
            callbacks->on_close = (co_http_close_fn)app_on_http_close;

            co_list_add_tail(self->http_clients, http_client);
        }
    }
    else
    {
        app_client_log(tcp, tcp_client, "tls handshake failed");

        co_tls_tcp_client_destroy(tcp_client);
    }
}

//---------------------------------------------------------------------------//
// tls setup
//---------------------------------------------------------------------------//

bool
app_tls_setup(
    co_tls_ctx_st* tls_ctx
)
{
#ifdef CO_USE_TLS
    const char* certificate_file = "../../../test_file/server.crt";
    const char* private_key_file = "../../../test_file/server.key";

    SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_server_method());

    if (SSL_CTX_use_certificate_file(
        ssl_ctx, certificate_file, SSL_FILETYPE_PEM) != 1)
    {
        SSL_CTX_free(ssl_ctx);

        printf("SSL_CTX_use_certificate_file failed\n");

        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(
        ssl_ctx, private_key_file, SSL_FILETYPE_PEM) != 1)
    {
        SSL_CTX_free(ssl_ctx);

        printf("SSL_CTX_use_PrivateKey_file failed\n");

        return false;
    }

    tls_ctx->ssl_ctx = ssl_ctx;
#endif

    return true;
}

//---------------------------------------------------------------------------//
// tcp callback
//---------------------------------------------------------------------------//

void
app_on_tcp_close(
    app_st* self,
    co_tcp_client_t* tcp_client
)
{
    app_client_log(tcp, tcp_client, "tcp closed");

    co_tls_tcp_client_destroy(tcp_client);
    co_list_remove(self->tcp_clients, tcp_client);
}

void
app_on_tcp_accept(
    app_st* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
)
{
    (void)tcp_server;

    app_client_log(tcp, tcp_client, "tcp accept");

    // accept
    co_tcp_accept((co_thread_t*)self, tcp_client);

    // callback
    co_tcp_callbacks_st* tcp_callbacks = co_tcp_get_callbacks(tcp_client);
    tcp_callbacks->on_close = (co_tcp_close_fn)app_on_tcp_close;
    co_tls_callbacks_st* tls_callbacks = co_tls_tcp_get_callbacks(tcp_client);
    tls_callbacks->on_handshake = (co_tls_handshake_fn)app_on_tls_handshake;

    // start tls handshake
    co_tls_tcp_start_handshake(tcp_client);

    co_list_add_tail(self->tcp_clients, tcp_client);
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
        printf("http1_http2_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    // client lists
    self->tcp_clients = co_list_create(NULL);
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_http_client_destroy;
    self->http_clients = co_list_create(&list_ctx);
    list_ctx.destroy_value = (co_item_destroy_fn)co_http2_client_destroy;
    self->http2_clients = co_list_create(&list_ctx);

    // tls setup
    co_tls_ctx_st tls_ctx = { 0 };
    if (!app_tls_setup(&tls_ctx))
    {
        return false;
    }

    // create tls server
    self->tcp_server =
        co_tls_tcp_server_create(&local_net_addr, &tls_ctx);

    if (self->tcp_server == NULL)
    {
        printf("Failed to create tls server (maybe SSL/TLS library was not found)\n");

        return false;
    }

    // available protocols
    size_t protocol_count = 2;
    const char* protocols[] = {
        CO_HTTP2_PROTOCOL,  // http 2
        CO_HTTP_PROTOCOL    // http 1.1
    };
    co_tls_tcp_server_set_available_protocols(
        self->tcp_server, protocols, protocol_count);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->tcp_server), true);

    // callbacks
    co_tcp_server_callbacks_st* callbacks =
        co_tcp_server_get_callbacks(self->tcp_server);
    callbacks->on_accept = (co_tcp_accept_fn)app_on_tcp_accept;

    // start listen
    if (!co_tls_tcp_server_start(self->tcp_server, SOMAXCONN))
    {
        return false;
    }

    printf("start server https://127.0.0.1:%d\n", port);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_tls_tcp_server_destroy(self->tcp_server);

    co_list_iterator_t* it =
        co_list_get_head_iterator(self->tcp_clients);
    while (it != NULL)
    {
        co_list_data_st* data =
            co_list_get_next(self->tcp_clients, &it);
        co_tls_tcp_client_destroy((co_tcp_client_t*)data->value);
    }
    co_list_destroy(self->tcp_clients);

    co_list_destroy(self->http_clients);
    co_list_destroy(self->http2_clients);
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

//    co_http2_log_set_level(CO_LOG_LEVEL_MAX);
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    // app instance
    app_st self = { 0 };

    // start app
    return co_net_app_start(
        (co_app_t*)&self, "http1-http2-server-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
