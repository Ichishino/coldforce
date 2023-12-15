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
    co_ws_client_t* ws_client;
    co_url_st* url;

} app_st;

//---------------------------------------------------------------------------//
// websocket callback
//---------------------------------------------------------------------------//

void
app_on_ws_receive_frame(
    app_st* self,
    co_ws_client_t* ws_client,
    const co_ws_frame_t* frame,
    int error_code
)
{
    if (error_code == 0)
    {
        bool fin = co_ws_frame_get_fin(frame);
        uint8_t opcode = co_ws_frame_get_opcode(frame);
        size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
        const uint8_t* data = co_ws_frame_get_payload_data(frame);

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
        default:
        {
            co_ws_default_handler(ws_client, frame);

            break;
        }
        }
    }
    else
    {
        printf("receive error: %d\n", error_code);

        // close
        co_ws_client_destroy(ws_client);
        self->ws_client = NULL;
    }
}

void
app_on_ws_close(
    app_st* self,
    co_ws_client_t* ws_client
)
{
    printf("closed\n");

    // close
    co_ws_client_destroy(ws_client);
    self->ws_client = NULL;

    // quit app
    co_app_stop();
}

void
app_on_ws_upgrade(
    app_st* self,
    co_ws_client_t* ws_client,
    const co_http_response_t* http_response,
    int error_code
)
{
    (void)http_response;

    printf("receive upgrade response: %d\n", error_code);

    if (error_code == 0)
    {
        printf("upgrade success\n");

        // send
        co_ws_send_text(ws_client, "hello");

        return;
    }
    else
    {
        printf("upgrade error\n");
    }

    // close
    co_ws_client_destroy(ws_client);
    self->ws_client = NULL;

    // quit app
    co_app_stop();
}

void
app_on_ws_connect(
    app_st* self,
    co_ws_client_t* ws_client,
    int error_code
)
{
    if (error_code == 0)
    {
        printf("connect success\n");

        co_http_request_t* request =
            co_http_request_create_ws_upgrade(
                self->url->path_and_query, NULL, NULL);

        // send upgrade request
        co_ws_send_upgrade_request(self->ws_client, request);
    }
    else
    {
        printf("connect error: %d\n", error_code);

        // close
        co_ws_client_destroy(ws_client);
        self->ws_client = NULL;
    }
}

//---------------------------------------------------------------------------//
// tls callback
//---------------------------------------------------------------------------//


#ifdef CO_USE_TLS
int
app_on_tls_verify_peer(
    int preverify_ok,
    X509_STORE_CTX* x509_ctx
)
{
    (void)preverify_ok;
    (void)x509_ctx;

    // always OK for debug
    return 1;
}
#endif

//---------------------------------------------------------------------------//
// app callback
//---------------------------------------------------------------------------//

bool
app_on_create(
    app_st* self
)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count < 2)
    {
        printf("<Usage>\n");
        printf("ws_client <url>\n");

        return false;
    }

    // server url
    self->url = co_url_create(args->values[1]);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

    co_tls_ctx_st tls_ctx = { 0 };

    // tls setting
#ifdef CO_USE_TLS
    if (strcmp(self->url->scheme, "wss") == 0)
    {
        SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_default_verify_paths(ssl_ctx);
        #if defined(CO_USE_WOLFSSL) && defined(_WIN32)
        SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_OFF); // TODO
        #endif
        SSL_CTX_set_verify(
            ssl_ctx, SSL_VERIFY_PEER, app_on_tls_verify_peer);
        tls_ctx.ssl_ctx = ssl_ctx;
    }
#endif

    // create websocket client
    self->ws_client =
        co_ws_client_create(self->url->origin, &local_net_addr, &tls_ctx);

    if (self->ws_client == NULL)
    {
        printf("error: faild to resolve hostname or SSL/TLS library is not installed\n");

        return false;
    }

    // callbacks
    co_ws_callbacks_st* callbacks = co_ws_get_callbacks(self->ws_client);
    callbacks->on_connect = (co_ws_connect_fn)app_on_ws_connect;
    callbacks->on_upgrade = (co_ws_upgrade_fn)app_on_ws_upgrade;
    callbacks->on_receive_frame = (co_ws_receive_frame_fn)app_on_ws_receive_frame;
    callbacks->on_close = (co_ws_close_fn)app_on_ws_close;

    // start connect
    co_ws_connect_start(self->ws_client);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_ws_client_destroy(self->ws_client);
    co_url_destroy(self->url);
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
//    co_ws_log_set_level(CO_LOG_LEVEL_MAX);

    // app instance
    app_st self = { 0 };

    // start app
    return co_net_app_start(
        (co_app_t*)&self, "ws-client-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
