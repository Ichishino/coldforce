#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#   ifdef CO_USE_WOLFSSL
#       pragma comment(lib, "wolfssl.lib")
#   elif defined(CO_USE_OPENSSL)
#       pragma comment(lib, "libssl.lib")
#       pragma comment(lib, "libcrypto.lib")
#   endif
#endif

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_tcp_server_t* server;
    co_list_t* http2_clients;
    uint16_t port;
    char authority[256];

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
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>Server Stop</title></head>\n"
        "<body>OK</body>\n"
        "</html>\n";
    uint32_t response_content_size = (uint32_t)strlen(response_content);

    // send response
    co_http2_stream_send_header(stream, false, response_header);
    co_http2_stream_send_data(stream, true, response_content, response_content_size);

    // quit app
    co_app_stop();
}

void on_my_http2_server_push_request(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream,
    const co_http2_header_t* request_header, const co_http2_data_st* receive_data)
{
    (void)request_header;
    (void)receive_data;

    const co_http2_settings_st* settings =
        co_http2_get_remote_settings(http2_client);

    if (settings->enable_push == 1)
    {
        //-----------------------------------------------------------------------//
        // push "/test.css"
        //-----------------------------------------------------------------------//

        // push request

        co_http2_header_t* request_header_1 = co_http2_header_create_request("GET", "/test.css");
        co_http2_header_set_authority(request_header_1, self->authority);

        co_http2_stream_t* response_stream_1 =
            co_http2_stream_send_server_push_request(stream, request_header_1);

        // push response

        co_http2_header_t* response_header_1 = co_http2_header_create_response(200);
        co_http2_header_add_field(response_header_1, "content-type", "text/css");
        co_http2_header_add_field(response_header_1, "cache-control", "no-store");

        const char* response_content_1 = "h1{ color:blue; }";
        uint32_t response_content_size_1 = (uint32_t)strlen(response_content_1);

        co_http2_stream_send_header(response_stream_1, false, response_header_1);
        co_http2_stream_send_data(response_stream_1, true, response_content_1, response_content_size_1);

        //-----------------------------------------------------------------------//
        // push "/test.js"
        //-----------------------------------------------------------------------//

        // push request

        co_http2_header_t* request_header_2 = co_http2_header_create_request("GET", "/test.js");
        co_http2_header_set_authority(request_header_2, self->authority);

        co_http2_stream_t* response_stream_2 =
            co_http2_stream_send_server_push_request(stream, request_header_2);

        // push response

        co_http2_header_t* response_header_2 = co_http2_header_create_response(200);
        co_http2_header_add_field(response_header_2, "content-type", "text/javascript");
        co_http2_header_add_field(response_header_2, "cache-control", "no-store");

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
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Http2 Server Push Test</title>\n"
        "<link rel='stylesheet' type='text/css' href='/test.css' />\n"
        "</head>\n"
        "<body>\n"
        "<h1>Server Push Test OK</h1>\n"
        "<script src='/test.js'></script>\n"
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

    co_http2_header_t* response_header = co_http2_header_create_response(200);

    co_http2_header_add_field(response_header, "content-type", "text/html");
    co_http2_header_add_field(response_header, "cache-control", "no-store");

    char response_content[8192] = { 0 };
    char temp[1024];

    strcpy(response_content,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Http2 Test</title>\n"
        "</head>\n"
        "<body>\n");

    strcat(response_content, "<h2>Client</h2>\n");

    const co_net_addr_t* remote_net_addr =
        co_http2_get_remote_net_addr(http2_client);
    char remote_str[256];
    co_net_addr_to_string(
        remote_net_addr, remote_str, sizeof(remote_str));

    sprintf(temp, "IP Address: %s<br>\n", remote_str);
    strcat(response_content, temp);

    strcat(response_content, "<h2>RequestHeader</h2>\n");

    if (request_header->pseudo.authority != NULL)
    {
        sprintf(temp, ":authority: %s<br>\n",
            request_header->pseudo.authority);
        strcat(response_content, temp);
    }

    if (request_header->pseudo.method != NULL)
    {
        sprintf(temp, ":method: %s<br>\n",
            request_header->pseudo.method);
        strcat(response_content, temp);
    }

    if (request_header->pseudo.url != NULL)
    {
        sprintf(temp, ":path: %s<br>\n",
            request_header->pseudo.url->src);
        strcat(response_content, temp);
    }

    if (request_header->pseudo.scheme != NULL)
    {
        sprintf(temp, ":scheme: %s<br>\n",
            request_header->pseudo.scheme);
        strcat(response_content, temp);
    }

    if (request_header->pseudo.status_code != 0)
    {
        sprintf(temp, ":status: %u<br>\n",
            request_header->pseudo.status_code);
        strcat(response_content, temp);
    }

    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(request_header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_const_next(request_header->field_list, &it);
        const co_http2_header_field_t* field =
            (const co_http2_header_field_t*)data->value;

        sprintf(temp, "%s: %s<br>\n",
            field->name, field->value);
        strcat(response_content, temp);
    }

    strcat(response_content, "<h2>Contents</h2>\n");
    sprintf(temp, "size: %zd<br><br>\n", receive_data->size);
    strcat(response_content, temp);

    if (receive_data->ptr != NULL)
    {
        strncat(response_content,
            (const char*)receive_data->ptr, receive_data->size);
    }

    strcat(response_content, "</body>\n</html>\n");

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
        const co_url_st* url = co_http2_header_get_path_url(request_header);

        if (strcmp(url->path, "/stop") == 0)
        {
            // server stop request
            // http(s)://xxxxxxxxxxx/stop

            on_my_http2_stop_app_request(self, http2_client, stream);
        }
        else if (strcmp(url->path, "/serverpush") == 0)
        {
            // server push request
            // http(s)://xxxxxxxxxxx/serverpush

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
        co_list_remove(self->http2_clients, http2_client);
    }
}

void on_my_http2_close(my_app* self, co_http2_client_t* http2_client, int error_code)
{
    (void)self;
    (void)error_code;

    my_client_log(http2, http2_client, "http2 close");

    // close
    co_list_remove(self->http2_clients, http2_client);
}

//---------------------------------------------------------------------------//
// tls
//---------------------------------------------------------------------------//

void on_my_tls_handshake(my_app* self, co_tcp_client_t* tcp_client, int error_code)
{
    (void)self;

    if (error_code == 0)
    {
        my_client_log(tcp, tcp_client, "TLS handshake success");

        // create http2 client
        co_http2_client_t* http2_client = co_tcp_upgrade_to_http2(tcp_client, NULL);

        // settings (optional)
        co_http2_setting_param_st params[2];
        params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
        params[0].value = 1024 * 1024 * 10;
        params[1].id = CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS;
        params[1].value = 200;
        co_http2_init_settings(http2_client, params, 2);

        // callback
        co_http2_callbacks_st* callback = co_http2_get_callbacks(http2_client);
        callback->on_receive_finish = (co_http2_receive_finish_fn)on_my_http2_request;
        callback->on_close = (co_http2_close_fn)on_my_http2_close;

        co_list_add_tail(self->http2_clients, http2_client);
    }
    else
    {
        my_client_log(tcp, tcp_client, "TLS handshake failed");

        // close
        co_tls_client_destroy(tcp_client);
    }
}

bool my_tls_setup(co_tls_ctx_st* tls_ctx)
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
// tcp
//---------------------------------------------------------------------------//

void on_my_tcp_close(my_app* self, co_tcp_client_t* tcp_client)
{
    (void)self;

    co_tls_client_destroy(tcp_client);
}

void on_my_tcp_accept(my_app* self, co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    (void)tcp_server;

    my_client_log(tcp, tcp_client, "accept");

    // accept
    co_tcp_accept((co_thread_t*)self, tcp_client);

    // callback
    co_tcp_callbacks_st* tcp_callbacks = co_tcp_get_callbacks(tcp_client);
    tcp_callbacks->on_close = (co_tcp_close_fn)on_my_tcp_close;
    co_tls_callbacks_st* tls_callbacks = co_tls_get_callbacks(tcp_client);
    tls_callbacks->on_handshake = (co_tls_handshake_fn)on_my_tls_handshake;

    // TLS handshake
    co_tls_start_handshake(tcp_client);
}

//---------------------------------------------------------------------------//
// app
//---------------------------------------------------------------------------//

bool on_my_app_create(my_app* self)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count <= 2)
    {
        printf("<Usage>\n");
        printf("http2_server <hostname> <port_number>\n");
        printf("ex. http2_server localhost 8080\n");

        return false;
    }

    self->port = (uint16_t)atoi(args->values[2]);

    sprintf(self->authority, "%s:%d", args->values[1], self->port);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_http2_client_destroy; // auto destroy
    self->http2_clients = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, self->port);

    // tls setting
    co_tls_ctx_st tls_ctx = { 0 };
    if (!my_tls_setup(&tls_ctx))
    {
        return false;
    }

    // https server
    self->server = co_tls_server_create(&local_net_addr, &tls_ctx);
    if (self->server == NULL)
    {
        printf("Failed to create tls server (maybe OpenSSL was not found)\n");

        return false;
    }

    // available protocols
    size_t protocol_count = 1;
    const char* protocols[] = { CO_HTTP2_PROTOCOL };
    co_tls_server_set_available_protocols(
        self->server, protocols, protocol_count);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    // callback
    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_my_tcp_accept;

    // listen start
    co_tls_server_start(self->server, SOMAXCONN);

    printf("https://%s\n", self->authority);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->http2_clients);
    co_tls_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_http2_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    return co_net_app_start(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);
}
