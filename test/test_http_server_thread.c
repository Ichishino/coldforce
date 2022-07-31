#include "test_http_server_thread.h"

//---------------------------------------------------------------------------//
// websocket over http2
//---------------------------------------------------------------------------//

static void
http_server_on_ws_http2_receive_frame(
    http_server_thread* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_ws_frame_t* frame
)
{
    bool fin = co_ws_frame_get_fin(frame);
    uint8_t opcode = co_ws_frame_get_opcode(frame);
    size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
    const uint8_t* data = co_ws_frame_get_payload_data(frame);

    switch (opcode)
    {
    case CO_WS_OPCODE_TEXT:
    case CO_WS_OPCODE_BINARY:
    case CO_WS_OPCODE_CONTINUATION:
    {
        co_http2_stream_send_ws_frame(stream,
            fin, opcode, false, data, (size_t)data_size);

        break;
    }
    case CO_WS_OPCODE_CLOSE:
    default:
    {
        co_http2_stream_ws_default_handler(stream, frame);

        break;
    }
    }
}

//---------------------------------------------------------------------------//
// websocket
//---------------------------------------------------------------------------//

static void
http_server_on_ws_receive_frame(
    http_server_thread* self,
    co_ws_client_t* ws_client,
    const co_ws_frame_t* frame,
    int error_code
)
{
    if (error_code != 0)
    {
        co_list_remove(self->ws_clients, ws_client);

        return;
    }

    bool fin = co_ws_frame_get_fin(frame);
    uint8_t opcode = co_ws_frame_get_opcode(frame);
    size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
    const uint8_t* data = co_ws_frame_get_payload_data(frame);

    switch (opcode)
    {
    case CO_WS_OPCODE_TEXT:
    case CO_WS_OPCODE_BINARY:
    case CO_WS_OPCODE_CONTINUATION:
    {
        co_ws_send(ws_client,
            fin, opcode, data, (size_t)data_size);

        break;
    }
    default:
    {
        co_ws_default_handler(ws_client, frame);

        break;
    }
    }
}

static void
http_server_on_ws_close(
    http_server_thread* self,
    co_ws_client_t* ws_client
)
{
    co_list_remove(self->ws_clients, ws_client);
}

//---------------------------------------------------------------------------//
// http2
//---------------------------------------------------------------------------//

static void
http_server_on_http2_request(
    http_server_thread* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_http2_header_t* request_header,
    const co_http2_data_st* request_data,
    int error_code
)
{
    if (error_code != 0)
    {
        co_list_remove(self->http2_clients, http2_client);

        return;
    }

    const co_http_url_st* url =
        co_http2_header_get_path_url(request_header);

    if (strcmp(url->path, "/stop") == 0)
    {
        bool is_digest_ok = false;

        const char* realm = "Coldforce Test Server";
        const char* user = "admin";
        const char* password = "12345";
        const char* nonce = "random_nonce";

        const char* request_auth_str =
            co_http2_header_get_field(
                request_header, CO_HTTP2_HEADER_AUTHORIZATION);

        if (request_auth_str != NULL)
        {
            co_http_auth_t* request_auth =
                co_http_auth_create_request(request_auth_str);

            if (request_auth != NULL)
            {
                if (co_http_digest_auth_validate(
                    request_auth, "GET", "/stop", realm,
                    user, password,
                    nonce, 0))
                {
                    is_digest_ok = true;
                }

                co_http_auth_destroy(request_auth);
            }
        }

        co_http2_header_t* response_header = NULL;
        const char* data  = NULL;

        if (is_digest_ok)
        {
            response_header =
                co_http2_header_create_response(200);

            data =
                "<html>"
                "<title>Coldforce Test</title>"
                "<body> http2 STOP OK </body>"
                "</html>";

            co_app_stop();
        }
        else
        {
            response_header =
                co_http2_header_create_response(401);

            data =
                "<html>"
                "<title>Coldforce Test</title>"
                "<body> http2 Unauthorized </body>"
                "</html>";

            co_http_auth_t* response_auth =
                co_http_digest_auth_create_response(realm, nonce, NULL);

            co_http2_header_apply_auth(response_header,
                CO_HTTP2_HEADER_WWW_AUTHENTICATE, response_auth);

            co_http_auth_destroy(response_auth);
        }

        co_http2_stream_send_header(
            stream, false, response_header);
        co_http2_stream_send_data(
            stream, true, data, (uint32_t)strlen(data));
    }
    else if (strcmp(url->path, "/server-push") == 0)
    {
    }
    else if (strcmp(url->path, "/ws-over-http2") == 0)
    {
        if (request_header != NULL)
        {
            if (co_http2_header_validate_ws_connect_request(request_header))
            {
                co_http2_header_t* response_header =
                    co_http2_header_create_ws_connect_response(NULL, NULL);

                co_http2_stream_send_header(stream, true, response_header);

                co_http2_stream_set_protocol_mode(stream, "websocket");
            }
            else
            {
                co_http2_header_t* response_header =
                    co_http2_header_create_response(404);

                co_http2_stream_send_header(
                    stream, true, response_header);
            }
        }
        else
        {
            co_ws_frame_t* ws_frame =
                co_http2_create_ws_frame(request_data);

            if (ws_frame == NULL)
            {
                return;
            }

            http_server_on_ws_http2_receive_frame(
                self, http2_client, stream, ws_frame);

            co_ws_frame_destroy(ws_frame);
        }
    }
    else if (strcmp(url->path, "/favicon.ico") == 0)
    {
        co_http2_header_t* response_header =
            co_http2_header_create_response(404);
        co_http2_stream_send_header(
            stream, true, response_header);
    }
    else
    {
        co_http2_header_t* response_header =
            co_http2_header_create_response(200);

        co_http2_header_add_field(
            response_header, "content-type", "text/html");
        co_http2_header_add_field(
            response_header, "cache-control", "no-store");

        const char* data =
            "<html>"
            "<title>Coldforce Test</title>"
            "<body> http2 Hello !!! </body>"
            "</html>";
        size_t data_length = strlen(data);

        co_http2_stream_send_header(
            stream, false, response_header);
        co_http2_stream_send_data(
            stream, true, data, (uint32_t)data_length);
    }
}

static void
http_server_on_http2_close(
    http_server_thread* self,
    co_http2_client_t* http2_client,
    int error_code
)
{
    co_list_remove(self->http2_clients, http2_client);
}

//---------------------------------------------------------------------------//
// http
//---------------------------------------------------------------------------//

static void
http_server_on_http_request(
    http_server_thread* self,
    co_http_client_t* htttp_client,
    const co_http_request_t* request,
    const co_http_response_t* unused,
    int error_code
)
{
    (void)unused;

    if (error_code != 0)
    {
        co_list_remove(self->http_clients, htttp_client);

        return;
    }

    bool not_found = false;

    const co_http_url_st* url = co_http_request_get_url(request);

    if (strcmp(url->path, "/ws") == 0)
    {
        if (co_http_request_validate_ws_upgrade(request))
        {
            co_http_response_t* response =
                co_http_response_create_ws_upgrade(
                    request, NULL, NULL);
            co_http_connection_send_response(
                (co_http_connection_t*)htttp_client, response);
            co_http_response_destroy(response);

            co_list_iterator_t* it =
                co_list_find(self->http_clients, htttp_client);
            co_list_data_st* data =
                co_list_get(self->http_clients, it);
            data->value = NULL;
            co_list_remove_at(self->http_clients, it);

            co_ws_client_t* ws_client =
                co_http_upgrade_to_ws(htttp_client);
            co_ws_callbacks_st* callbacks =
                co_ws_get_callbacks(ws_client);
            callbacks->on_receive_frame =
                (co_ws_receive_frame_fn)http_server_on_ws_receive_frame;
            callbacks->on_close =
                (co_ws_close_fn)http_server_on_ws_close;

            co_list_add_tail(self->ws_clients, ws_client);

            return;
        }
        else
        {
            not_found = true;
        }
    }
    else if (strcmp(url->path, "/favicon.ico") == 0)
    {
        not_found = true;
    }

    if (!not_found)
    {
        const char* data =
            "<html><title>coldforce test</title><body>http OK</body></html>";
        size_t data_length = strlen(data);

        co_http_response_t* response =
            co_http_response_create_with(200, "OK");
        co_http_header_t* response_header =
            co_http_response_get_header(response);
        co_http_header_add_field(
            response_header, "Content-Type", "text/html");
        co_http_header_add_field(
            response_header, "Cache-Control", "no-store");
        co_http_header_set_content_length(
            response_header, data_length);

        const co_http_header_t* request_header =
            co_http_request_get_const_header(request);

        bool keep_alive =
            co_http_header_get_keep_alive(request_header);

        if (!keep_alive)
        {
            co_http_header_add_field(
                response_header, CO_HTTP_HEADER_CONNECTION, "close");
        }

        co_http_send_response(htttp_client, response);
        co_http_send_data(htttp_client, data, data_length);

        if (!keep_alive)
        {
            co_list_remove(self->http_clients, htttp_client);
        }
    }
    else
    {
        co_http_response_t* response =
            co_http_response_create_with(404, "NotFound");
        co_http_header_t* response_header =
            co_http_response_get_header(response);
        co_http_header_add_field(
            response_header, "Content-Type", "text/html");
        const char* data =
            "<html><title>coldforce test</title><body>http NotFound</body></html>";
        size_t data_length = strlen(data);
        co_http_header_set_content_length(response_header, data_length);
        co_http_send_response(htttp_client, response);
        co_http_send_data(htttp_client, data, data_length);

        co_list_remove(self->http_clients, htttp_client);
    }
}

static void
http_server_on_http_close(
    http_server_thread* self,
    co_http_client_t* client
)
{
    co_list_remove(self->http_clients, client);
}

//---------------------------------------------------------------------------//
// tls
//---------------------------------------------------------------------------//

static void
http_server_on_tls_handshake(
    http_server_thread* self,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    if (error_code != 0)
    {
        co_tls_client_destroy(tcp_client);

        return;
    }

    char protocol[32];

    if (co_tls_get_selected_protocol(
        tcp_client, protocol, sizeof(protocol)))
    {
        if (strcmp(protocol, CO_HTTP2_PROTOCOL) == 0)
        {
            co_http2_client_t* http2_client =
                co_tcp_upgrade_to_http2(tcp_client);

            co_http2_callbacks_st* callbacks =
                co_http2_get_callbacks(http2_client);
            callbacks->on_receive_finish =
                (co_http2_receive_finish_fn)http_server_on_http2_request;
            callbacks->on_close =
                (co_http2_close_fn)http_server_on_http2_close;

            co_http2_setting_param_st params[3];
            params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
            params[0].value = 1024 * 1024 * 10;
            params[1].id = CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS;
            params[1].value = 200;
            params[2].id = CO_HTTP2_SETTING_ID_ENABLE_CONNECT_PROTOCOL;
            params[2].value = 1;
            co_http2_init_settings(http2_client, params, 3);
            co_http2_send_initial_settings(http2_client);

            co_list_add_tail(self->http2_clients, http2_client);

            return;
        }
        else if (strcmp(protocol, CO_HTTP_PROTOCOL) != 0)
        {
            co_tls_client_destroy(tcp_client);

            return;
        }
    }

    co_http_client_t* http_client =
        co_tcp_upgrade_to_http(tcp_client);

    co_http_callbacks_st* callbacks =
        co_http_get_callbacks(http_client);
    callbacks->on_receive_finish =
        (co_http_receive_finish_fn)http_server_on_http_request;
    callbacks->on_close =
        (co_http_close_fn)http_server_on_http_close;

    co_list_add_tail(self->http_clients, http_client);
}

//---------------------------------------------------------------------------//
// tcp
//---------------------------------------------------------------------------//

static void
http_server_on_tcp_accept(
    http_server_thread* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
)
{
    (void)tcp_server;

    co_tcp_accept((co_thread_t*)self, tcp_client);

    co_tls_callbacks_st* callbacks =
        co_tls_get_callbacks(tcp_client);
    callbacks->on_handshake =
        (co_tls_handshake_fn)http_server_on_tls_handshake;

    co_tls_start_handshake(tcp_client);
}

static bool
setup_tls(
    http_server_thread* self,
    co_tls_ctx_st* tls_ctx
)
{
    tls_ctx->ssl_ctx = SSL_CTX_new(TLS_server_method());

    if (SSL_CTX_use_certificate_file(
        tls_ctx->ssl_ctx, self->certificate_file, SSL_FILETYPE_PEM) != 1)
    {
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(
        tls_ctx->ssl_ctx, self->private_key_file, SSL_FILETYPE_PEM) != 1)
    {
        return false;
    }

    return true;
}

static bool
on_http_server_thread_create(
    http_server_thread* self
)
{
    co_list_ctx_st http_list_ctx = { 0 };
    http_list_ctx.destroy_value =
        (co_item_destroy_fn)co_http_client_destroy;
    self->http_clients = co_list_create(&http_list_ctx);

    co_list_ctx_st http2_list_ctx = { 0 };
    http2_list_ctx.destroy_value =
        (co_item_destroy_fn)co_http2_client_destroy;
    self->http2_clients = co_list_create(&http2_list_ctx);

    co_list_ctx_st ws_list_ctx = { 0 };
    ws_list_ctx.destroy_value =
        (co_item_destroy_fn)co_ws_client_destroy;
    self->ws_clients = co_list_create(&ws_list_ctx);

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, self->port);

    co_tls_ctx_st tls_ctx = { 0 };
    
    if (!setup_tls(self, &tls_ctx))
    {
        return false;
    }

    self->server = co_tls_server_create(&local_net_addr, &tls_ctx);

    size_t protocol_count = 2;
    const char* protocols[] = { CO_HTTP2_PROTOCOL, CO_HTTP_PROTOCOL };
//    size_t protocol_count = 1;
//    const char* protocols[] = { CO_HTTP_PROTOCOL };
    co_tls_server_set_available_protocols(
        self->server, protocols, protocol_count);

    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    co_tcp_server_callbacks_st* callbacks =
        co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept =
        (co_tcp_accept_fn)http_server_on_tcp_accept;

    return co_tls_server_start(self->server, SOMAXCONN);
}

static void
on_http_server_thread_destroy(
    http_server_thread* self
)
{
    co_tls_server_destroy(self->server);

    co_list_destroy(self->http_clients);
    co_list_destroy(self->http2_clients);
    co_list_destroy(self->ws_clients);
}

bool
http_server_thread_start(
    http_server_thread* thread
)
{
    co_net_thread_init(
        (co_thread_t*)thread,
        (co_thread_create_fn)on_http_server_thread_create,
        (co_thread_destroy_fn)on_http_server_thread_destroy);

    return co_thread_start((co_thread_t*)thread);
}
