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
    co_http2_client_t* http2_client;
    co_url_st* url;
    char* save_file_path;
    FILE* fp;

} app_st;

//---------------------------------------------------------------------------//
// http2 callback
//---------------------------------------------------------------------------//

bool
app_on_http2_receive_start(
    app_st* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_http2_header_t* response_header
)
{
    (void)http2_client;
    (void)stream;
    (void)response_header;

    self->fp = fopen(self->save_file_path, "wb");

    return true;
}

bool
app_on_http2_receive_data(
    app_st* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_http2_header_t* response_header,
    const co_http2_data_st* data_block
)
{
    (void)http2_client;
    (void)stream;
    (void)response_header;

    fwrite(data_block->ptr, data_block->size, 1, self->fp);

    return true;
}

void
app_on_http2_receive_finish(
    app_st* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_http2_header_t* response_header,
    const co_http2_data_st* response_data,
    int error_code
)
{
    printf("response: (%d)\n", error_code);

    if (error_code == 0)
    {
        uint16_t status_code =
            co_http2_header_get_status_code(response_header);
        printf("status code: %d\n", status_code);

        if (response_data->ptr != NULL)
        {
            printf("%s\n", (const char*)response_data->ptr);
        }
    }

    if (self->fp != NULL)
    {
        fclose(self->fp);
        self->fp = NULL;
    }

    if (!co_http2_is_running(http2_client))
    {
        // quit app
        co_app_stop();
    }
}

void
app_on_http2_close(
    app_st* self,
    co_http2_client_t* http2_client,
    int error_code
)
{
    (void)self;
    (void)http2_client;

    printf("closed: (%d)\n", error_code);

    // quit app
    co_app_stop();
}

void
app_on_http2_connect(
    app_st* self,
    co_http2_client_t* http2_client,
    int error_code
)
{
    printf("connect: (%d)\n", error_code);

    if (error_code == 0)
    {
        co_http2_header_t* header =
            co_http2_header_create_request("GET", self->url->path_and_query);

        co_http2_header_add_field(header, "accept", "text/html");

        co_http2_stream_t* stream = co_http2_create_stream(http2_client);

        // send request
        co_http2_stream_send_header(stream, true, header);
    }
    else
    {
        printf("connect failed\n");

        // quit app
        co_app_stop();
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
        printf("http2_client <url> [save_file_path]\n");

        return false;
    }

    // url
    self->url = co_url_create(args->values[1]);

    if (args->count >= 3)
    {
        // download file name
        self->save_file_path = args->values[2];
    }

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

    co_tls_ctx_st tls_ctx = { 0 };

#ifdef CO_USE_TLS
    if (strcmp(self->url->scheme, "https") == 0)
    {
        // setup tls

        SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_default_verify_paths(ssl_ctx);
        #if defined(CO_USE_WOLFSSL) && defined(_WIN32)
        SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_OFF); // TODO
        #endif
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, app_on_tls_verify_peer);
        tls_ctx.ssl_ctx = ssl_ctx;
    }
#endif

    // create http2 client
    self->http2_client = co_http2_client_create(
        self->url->origin, &local_net_addr, &tls_ctx);

    if (self->http2_client == NULL)
    {
        printf("error: faild to resolve hostname or SSL/TLS library is not installed\n");

        return false;
    }

    // http2 settings (optinal)
    co_http2_setting_param_st params[1];
    params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
    params[0].value = CO_HTTP2_SETTING_MAX_WINDOW_SIZE;
    co_http2_init_settings(self->http2_client, params, 1);

    // callbacks
    co_http2_callbacks_st* callback = co_http2_get_callbacks(self->http2_client);
    callback->on_connect = (co_http2_connect_fn)app_on_http2_connect;
    callback->on_receive_finish = (co_http2_receive_finish_fn)app_on_http2_receive_finish;
    callback->on_close = (co_http2_close_fn)app_on_http2_close;

    if (self->save_file_path != NULL)
    {
        callback->on_receive_start =
            (co_http2_receive_start_fn)app_on_http2_receive_start;
        callback->on_receive_data =
            (co_http2_receive_data_fn)app_on_http2_receive_data;
    }

    // start connect
    co_http2_connect_start(self->http2_client);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_http2_client_destroy(self->http2_client);
    co_url_destroy(self->url);

    if (self->fp != NULL)
    {
        fclose(self->fp);
    }
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
        (co_app_t*)&self, "http2-client-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
