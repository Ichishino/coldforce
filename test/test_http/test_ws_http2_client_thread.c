#include "test_ws_http2_client_thread.h"

void
ws_http2_client_on_receive_finish(
    ws_http2_client_thread* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_http2_header_t* response_header,
    const co_http2_data_st* response_data,
    int error_code
)
{
    if (error_code != 0)
    {
        co_http2_client_destroy(http2_client);

        self->http2_client = NULL;
        self->http2_stream1 = NULL;
        self->http2_stream2 = NULL;

        return;
    }

    if (co_http2_stream_get_protocol_mode(stream) == NULL)
    {
        if (co_http2_header_validate_ws_connect_response(response_header))
        {
            co_http2_stream_set_protocol_mode(stream, "websocket");

            // websocket mode is ready

            co_http2_stream_send_ws_text(stream, true, "hello");
        }
        else
        {
            co_http2_client_destroy(http2_client);
            self->http2_client = NULL;
        }
    }
    else
    {
        co_ws_frame_t* ws_frame =
            co_http2_stream_receive_ws_frame(stream, response_data);

        if (ws_frame == NULL)
        {

            return;
        }

        bool fin = co_ws_frame_get_fin(ws_frame);
        uint8_t opcode = co_ws_frame_get_opcode(ws_frame);
        size_t data_size = (size_t)co_ws_frame_get_payload_size(ws_frame);
        const uint8_t* data = co_ws_frame_get_payload_data(ws_frame);

        switch (opcode)
        {
        case CO_WS_OPCODE_TEXT:
        {
            printf("receive text(%d): %*.*s\n",
                fin, (int)data_size, (int)data_size, (char*)data);

            break;
        }
        case CO_WS_OPCODE_BINARY:
        {
            printf("receive binary(%d): %zu bytes\n",
                fin, data_size);

            break;
        }
        case CO_WS_OPCODE_CONTINUATION:
        {
            printf("receive continuation(%d): %zu bytes\n",
                fin, data_size);

            break;
        }
        case CO_WS_OPCODE_CLOSE:
        {
            if (self->http2_stream1 == stream)
            {
                self->http2_stream1 = NULL;
            }
            else if (self->http2_stream2 == stream)
            {
                self->http2_stream2 = NULL;
            }
        }
        default:
        {
            co_http2_stream_ws_default_handler(stream, ws_frame);

            break;
        }
        }

        co_ws_frame_destroy(ws_frame);
    }
}

void
ws_http2_client_on_close(
    ws_http2_client_thread* self,
    co_http2_client_t* http2_client,
    int error_code
)
{
    (void)error_code;

    co_assert(http2_client == self->http2_client);

    co_http2_client_destroy(http2_client);

    self->http2_client = NULL;
    self->http2_stream1 = NULL;
    self->http2_stream2 = NULL;
}

void 
ws_http2_client_on_connect(
    ws_http2_client_thread* self,
    co_http2_client_t* http2_client,
    int error_code
)
{
    if (error_code == 0)
    {
        {
            co_http2_header_t* header =
                co_http2_header_create_ws_connect_request(self->url->path_and_query, NULL, NULL);

            self->http2_stream1 = co_http2_create_stream(http2_client);

            co_http2_stream_send_header(self->http2_stream1, true, header);
        }
        /*
        {
            co_http2_header_t* header =
                co_http2_header_create_ws_connect_request(self->url->path_and_query, NULL, NULL);

            self->http2_stream2 = co_http2_create_stream(http2_client);

            co_http2_stream_send_header(self->http2_stream2, true, header);
        }
        */
    }
    else
    {
        co_http2_client_destroy(http2_client);
        self->http2_client = NULL;
    }
}

static int
ws_http2_client_on_tls_verify_callback(
    int preverify_ok,
    X509_STORE_CTX* ctx
)
{
    (void)preverify_ok;
    (void)ctx;

    return 1;
}

static bool
ws_http2_client_tls_setup(
    ws_http2_client_thread* self,
    co_tls_ctx_st* tls_ctx
)
{
    (void)self;

#ifdef CO_USE_TLS
    if (strcmp(self->url->scheme, "https") == 0)
    {
        SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_default_verify_paths(ssl_ctx);
#if defined(CO_USE_WOLFSSL) && defined(_WIN32)
        SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_OFF); // TODO
#endif
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, ws_http2_client_on_tls_verify_callback);
        tls_ctx->ssl_ctx = ssl_ctx;
    }
#endif

    return true;
}

static bool
ws_http2_client_on_thread_create(
    ws_http2_client_thread* self
)
{
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(
        &local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

    co_tls_ctx_st tls_ctx = { 0 };

    if (!ws_http2_client_tls_setup(self, &tls_ctx))
    {
        return false;
    }

    self->http2_client = co_http2_client_create(
        self->url->origin, &local_net_addr, &tls_ctx);

    if (self->http2_client == NULL)
    {
        return false;
    }

    co_http2_setting_param_st params[2];
    params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
    params[0].value = CO_HTTP2_SETTING_MAX_WINDOW_SIZE;
    params[1].id = CO_HTTP2_SETTING_ID_ENABLE_CONNECT_PROTOCOL;
    params[1].value = 1;
    co_http2_init_settings(self->http2_client, params, 2);

    co_http2_callbacks_st* callback = co_http2_get_callbacks(self->http2_client);
    callback->on_connect =
        (co_http2_connect_fn)ws_http2_client_on_connect;
    callback->on_receive_finish =
        (co_http2_receive_finish_fn)ws_http2_client_on_receive_finish;
    callback->on_close =
        (co_http2_close_fn)ws_http2_client_on_close;

    co_http2_connect(self->http2_client);

    return true;
}

static void
ws_http2_client_on_thread_destroy(
    ws_http2_client_thread* self
)
{
    co_http2_client_destroy(self->http2_client);
    co_url_destroy(self->url);
}

bool
ws_http2_client_thread_start(
    ws_http2_client_thread* thread
)
{
    co_net_thread_setup(
        (co_thread_t*)thread, "ws_http2_client_thread",
        (co_thread_create_fn)ws_http2_client_on_thread_create,
        (co_thread_destroy_fn)ws_http2_client_on_thread_destroy);

    return co_thread_start((co_thread_t*)thread);
}
