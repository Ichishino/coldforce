#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <coldforce/coldforce_http2.h>

// openssl
#ifdef _WIN32
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http_server_t* server;
    co_list_t* http2_clients;

} my_app;

#define my_client_log(protocol, client, str) \
    do { \
        char remote_str[64]; \
        co_net_addr_get_as_string( \
            co_##protocol##_get_remote_net_addr(client), remote_str); \
        printf("%s: %s\n", str, remote_str); \
    } while(0)

//---------------------------------------------------------------------------//
// http/2
//---------------------------------------------------------------------------//

void on_my_http2_stop_app_request(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream)
{
    (void)self;
    (void)http2_client;

    my_client_log(http2, http2_client, "=== server stop ===");

    co_http2_header_t* response_header = co_http2_header_create_response(200);

    co_http2_header_add_field(response_header, "content-type", "text/html");

    const char* response_content =
        "<html>\n"
        "<head><title>Server Stop</title></head>\n"
        "<body>OK</body>\n"
        "</html>\n";
    uint32_t response_content_size = (uint32_t)strlen(response_content);

    // send response
    co_http2_stream_send_header(stream, false, response_header);
    co_http2_stream_send_data(stream, true, response_content, response_content_size);

    // quit app
    co_net_app_stop();
}

void on_my_http2_default_request(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream,
    const co_http2_header_t* request_header, const co_http2_data_st* receive_data)
{
    (void)self;
    (void)http2_client;
    (void)receive_data;

    const char* method = co_http2_header_get_method(request_header);
    const char* authority = co_http2_header_get_authority(request_header);
    const co_http_url_st* url = co_http2_header_get_path_url(request_header);
    const char* path = url->path;
    const char* query = ((url->query != NULL) ? url->query : "");

    co_http2_header_t* response_header = co_http2_header_create_response(200);

    co_http2_header_add_field(response_header, "content-type", "text/html");

    char response_content[8192];
    sprintf(response_content,
        "<html>\n"
        "<head>\n"
        "<title>Http2 Test</title>\n"
        "</head>\n"
        "<body>\n"
        "method: %s<br>\n"
        "request url: %s<br>\n"
        "query: %s<br>\n"
        "authority: %s<br>\n"
        "</body>\n"
        "</html>\n",
        method, path, query, authority);
    uint32_t response_content_size = (uint32_t)strlen(response_content);

    // send response
    co_http2_stream_send_header(stream, false, response_header);
    co_http2_stream_send_data(stream, true, response_content, response_content_size);
}

void on_my_http2_request(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream,
    const co_http2_header_t* request_header, const co_http2_data_st* request_data,
    int error_code)
{
    (void)self;

    my_client_log(http2, http2_client, "http2 request");

    if (error_code == 0)
    {
        const co_http_url_st* url = co_http2_header_get_path_url(request_header);

        if (strcmp(url->path, "/stop") == 0)
        {
            // server stop request
            // https://127.0.0.1:9443/stop

            on_my_http2_stop_app_request(self, http2_client, stream);
        }
        else
        {
            on_my_http2_default_request(
                self, http2_client, stream, request_header, request_data);
        }
    }
    else
    {
        // close
        co_list_remove(self->http2_clients, (uintptr_t)http2_client);
    }
}

void on_my_http2_close(my_app* self, co_http2_client_t* http2_client, int error_code)
{
    (void)self;
    (void)error_code;

    my_client_log(http2, http2_client, "http2 close");

    // close
    co_list_remove(self->http2_clients, (uintptr_t)http2_client);
}

//---------------------------------------------------------------------------//
// http/1.1
//---------------------------------------------------------------------------//

void on_my_http_request(my_app* self, co_http_client_t* http1_client,
    const co_http_request_t* request, int error_code)
{
    (void)self;
    (void)request;

    my_client_log(http, http1_client, "http1 request");

    if (error_code == 0)
    {
        co_http_response_t* response =
            co_http_response_create_with(200, "OK");

        const char* content =
            "<html><head><title>HTTP/1.1</title></head>"
                "<body>HTTP/1.1 OK</body></html>";
        size_t content_size = strlen(content);

        co_http_header_t* header = co_http_response_get_header(response);
        co_http_header_set_content_length(header, content_size);

        co_http_send_response(http1_client, response);
        co_http_send_data(http1_client, content, content_size);
    }

    co_http_client_destroy(http1_client);
}

void on_my_http_close(my_app* self, co_http_client_t* http1_client)
{
    (void)self;

    my_client_log(http, http1_client, "http1 close");

    co_http_client_destroy(http1_client);
}

//---------------------------------------------------------------------------//
// tls
//---------------------------------------------------------------------------//

void on_my_tls_tcp_handshake(my_app* self, co_tcp_client_t* tcp_client, int error_code)
{
    (void)self;

    if (error_code == 0)
    {
        my_client_log(tcp, tcp_client, "TLS handshake success");

        char protocol[32] = { 0 };
        co_tls_tcp_get_alpn_selected_protocol(
            tcp_client, protocol, sizeof(protocol));

        if (strcmp(protocol, CO_HTTP2_PROTOCOL) == 0)
        {
            // http/2

            co_http2_client_t* http2_client = co_http2_client_create_with(tcp_client);

            co_http2_set_message_handler(http2_client, (co_http2_message_fn)on_my_http2_request);
            co_http2_set_close_handler(http2_client, (co_http2_close_fn)on_my_http2_close);

            co_list_add_tail(self->http2_clients, (uintptr_t)http2_client);
        }
        else
        {
            // http/1.1

            co_http_client_t* http1_client = co_http_client_create_with(tcp_client);

            co_http_set_receive_handler(http1_client, (co_http_receive_fn)on_my_http_request);
            co_http_set_close_handler(http1_client, (co_http_close_fn)on_my_http_close);
        }
    }
    else
    {
        my_client_log(tcp, tcp_client, "TLS handshake failed");

        co_tcp_client_destroy(tcp_client);
    }
}

//---------------------------------------------------------------------------//
// tcp
//---------------------------------------------------------------------------//

void on_my_tcp_accept(my_app* self, co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    (void)tcp_server;

    my_client_log(tcp, tcp_client, "accept");

    // accept
    co_tcp_accept((co_thread_t*)self, tcp_client);

    // TLS handshake
    co_tls_tcp_start_handshake(
        tcp_client, (co_tls_tcp_handshake_fn)on_my_tls_tcp_handshake);
}

//---------------------------------------------------------------------------//
// app
//---------------------------------------------------------------------------//

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_free_fn)co_http2_client_destroy; // auto destroy
    self->http2_clients = co_list_create(&list_ctx);

    uint16_t port = 9443;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);
#if 1
    // TLS setting (openssl)
    co_tls_ctx_st tls_ctx = { 0 };
    tls_ctx.ssl_ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_use_certificate_file(tls_ctx.ssl_ctx, "server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(tls_ctx.ssl_ctx, "server.key", SSL_FILETYPE_PEM);

    // https server
    self->server = co_http_tls_server_create(&local_net_addr, &tls_ctx);

    // available protocols
    size_t protocol_count = 2;
    const char* protocols[] = { CO_HTTP2_PROTOCOL, CO_HTTP_PROTOCOL };
    co_tls_tcp_server_set_alpn_available_protocols(
        self->server->tcp_server, protocols, protocol_count);
#else
    // http server
    self->server = co_http_server_create(&local_net_addr);
#endif

    // socket option
    co_socket_option_set_reuse_addr(
        co_http_server_get_socket(self->server), true);

    // listen start
    co_http_server_start(self->server,
        (co_tcp_accept_fn)on_my_tcp_accept, SOMAXCONN);

    char local_str[64];
    co_net_addr_get_as_string(&local_net_addr, local_str);
    printf("listen %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->http2_clients);
    co_http_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
    co_tls_setup();

    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_create_fn)on_my_app_create,
        (co_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}
