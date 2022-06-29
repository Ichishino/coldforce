#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <coldforce/coldforce_http2.h>

#include <string.h>

#ifdef CO_CAN_USE_TLS
// openssl
#ifdef _WIN32
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif
#endif // CO_CAN_USE_TLS

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
        co_net_addr_to_string( \
            co_##protocol##_get_remote_net_addr(client), remote_str, sizeof(remote_str)); \
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
    co_http2_header_add_field(response_header, "cache-control", "no-store");

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

void on_my_http2_server_push_request(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream,
    const co_http2_header_t* request_header, const co_http2_data_st* receive_data)
{
    (void)self;
    (void)request_header;
    (void)receive_data;

    const co_http2_settings_st* settings =
        co_http2_get_remote_settings(http2_client);

    if (settings->enable_push == 1)
    {
        const char* authority = "127.0.0.1:9443";

        //-----------------------------------------------------------------------//
        // push "/test.css"
        //-----------------------------------------------------------------------//

        // push request

        co_http2_header_t* request_header_1 = co_http2_header_create_request("GET", "/test.css");
        co_http2_header_set_authority(request_header_1, authority);

        co_http2_stream_t* response_stream_1 =
            co_http2_stream_send_server_push_request(stream, request_header_1);

        // push response

        co_http2_header_t* response_header_1 = co_http2_header_create_response(200);

        const char* response_content_1 = "h1{ font-size:20px; }";
        uint32_t response_content_size_1 = (uint32_t)strlen(response_content_1);

        co_http2_stream_send_header(response_stream_1, false, response_header_1);
        co_http2_stream_send_data(response_stream_1, true, response_content_1, response_content_size_1);

        //-----------------------------------------------------------------------//
        // push "/test.js"
        //-----------------------------------------------------------------------//

        // push request

        co_http2_header_t* request_header_2 = co_http2_header_create_request("GET", "/test.js");
        co_http2_header_set_authority(request_header_2, authority);

        co_http2_stream_t* response_stream_2 =
            co_http2_stream_send_server_push_request(stream, request_header_2);

        // push response

        co_http2_header_t* response_header_2 = co_http2_header_create_response(200);

        const char* response_content_2 = "document.write('Hello !!');";
        uint32_t response_content_size_2 = (uint32_t)strlen(response_content_2);

        co_http2_stream_send_header(response_stream_2, false, response_header_2);
        co_http2_stream_send_data(response_stream_2, true, response_content_2, response_content_size_2);
    }

    //-----------------------------------------------------------------------//
    // response "/serverpush"
    //-----------------------------------------------------------------------//

    co_http2_header_t* response_header = co_http2_header_create_response(200);

    co_http2_header_add_field(response_header, "content-type", "text/html");
    co_http2_header_add_field(response_header, "cache-control", "no-store");

    char response_content[8192];
    sprintf(response_content,
        "<html>\n"
        "<head>\n"
        "<title>Http2 Server Push Test</title>\n"
        "<link rel='stylesheet' href='test.css'>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Server Push Test OK</h1>\n"
        "<script src='test.js'></script>\n"
        "</body>\n"
        "</html>\n");
    uint32_t response_content_size = (uint32_t)strlen(response_content);

    // send response
    co_http2_stream_send_header(stream, false, response_header);
    co_http2_stream_send_data(stream, true, response_content, response_content_size);
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
    co_http2_header_add_field(response_header, "cache-control", "no-store");

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
        else if (strcmp(url->path, "/serverpush") == 0)
        {
            // server push request
            // https://127.0.0.1:9443/serverpush

            on_my_http2_server_push_request(
                self, http2_client, stream, request_header, request_data);
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
// tls
//---------------------------------------------------------------------------//

#ifdef CO_CAN_USE_TLS

void on_my_tls_handshake(my_app* self, co_tcp_client_t* tcp_client, int error_code)
{
    (void)self;

    if (error_code == 0)
    {
        my_client_log(tcp, tcp_client, "TLS handshake success");

        // create http2 client
        co_http2_client_t* http2_client = co_http2_client_create_with(tcp_client);

        // settings (optional)
        co_http2_setting_param_st params[2];
        params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
        params[0].value = 1024 * 1024 * 10;
        params[1].id = CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS;
        params[1].value = 200;
        co_http2_init_settings(http2_client, params, 2);

        // set callback
        co_http2_set_message_handler(http2_client, (co_http2_message_fn)on_my_http2_request);
        co_http2_set_close_handler(http2_client, (co_http2_close_fn)on_my_http2_close);

        co_list_add_tail(self->http2_clients, (uintptr_t)http2_client);
    }
    else
    {
        my_client_log(tcp, tcp_client, "TLS handshake failed");

        // close
        co_tls_client_destroy(tcp_client);
    }
}

#endif // CO_CAN_USE_TLS

//---------------------------------------------------------------------------//
// tcp
//---------------------------------------------------------------------------//

void on_my_tcp_accept(my_app* self, co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    (void)tcp_server;

    my_client_log(tcp, tcp_client, "accept");

    // accept
    co_tcp_accept((co_thread_t*)self, tcp_client);

#ifdef CO_CAN_USE_TLS

    // TLS handshake
    co_tls_start_handshake(
        tcp_client, (co_tls_handshake_fn)on_my_tls_handshake);

#else

    // create http2 client
    co_http2_client_t* http2_client = co_http2_client_create_with(tcp_client);

    // settings (optional)
    co_http2_setting_param_st params[2];
    params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
    params[0].value = 1024 * 1024 * 10;
    params[1].id = CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS;
    params[1].value = 200;
    co_http2_init_settings(http2_client, params, 2);

    // set callback
    co_http2_set_message_handler(http2_client, (co_http2_message_fn)on_my_http2_request);
    co_http2_set_close_handler(http2_client, (co_http2_close_fn)on_my_http2_close);

    co_list_add_tail(self->http2_clients, (uintptr_t)http2_client);

#endif // CO_CAN_USE_TLS
}

//---------------------------------------------------------------------------//
// app
//---------------------------------------------------------------------------//

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    if (arg->argc <= 1)
    {
        printf("<Usage>\n");
        printf("http2_server port_number\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(arg->argv[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_item_free_fn)co_http2_client_destroy; // auto destroy
    self->http2_clients = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

#ifdef CO_CAN_USE_TLS

    // TLS setting (openssl)
    co_tls_ctx_st tls_ctx = { 0 };
    tls_ctx.ssl_ctx = SSL_CTX_new(TLS_server_method());

    const char* certificate_file = "server.crt";
    const char* private_key_file = "server.key";

    if (SSL_CTX_use_certificate_file(
        tls_ctx.ssl_ctx, certificate_file, SSL_FILETYPE_PEM) != 1)
    {
        printf("SSL_CTX_use_certificate_file failed\n");

        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(
        tls_ctx.ssl_ctx, private_key_file, SSL_FILETYPE_PEM) != 1)
    {
        printf("SSL_CTX_use_PrivateKey_file failed\n");

        return false;
    }

    // https server
    self->server = co_http_tls_server_create(&local_net_addr, &tls_ctx);

    // available protocols
    size_t protocol_count = 1;
    const char* protocols[] = { CO_HTTP2_PROTOCOL };
    co_http_tls_server_set_available_protocols(
        self->server, protocols, protocol_count);

#else

    // http server
    self->server = co_http_server_create(&local_net_addr);

#endif // CO_CAN_USE_TLS

    // socket option
    co_socket_option_set_reuse_addr(
        co_http_server_get_socket(self->server), true);

    // listen start
    co_http_server_start(self->server,
        (co_tcp_accept_fn)on_my_tcp_accept, SOMAXCONN);

#ifdef CO_CAN_USE_TLS
    printf("https://127.0.0.1:%d\n", port);
#else
    printf("http://127.0.0.1:%d\n", port);
#endif

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->http2_clients);
    co_http_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_http2_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    co_tls_setup();

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}
