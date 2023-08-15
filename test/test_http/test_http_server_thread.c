#include "test_http_server_thread.h"
#include "test_http_server_http_connection.h"
#include "test_http_server_http2_connection.h"

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
            // http2
            add_http2_server_connection(self, tcp_client);

            return;
        }
        else if (strcmp(protocol, CO_HTTP_PROTOCOL) != 0)
        {
            // unknown
            co_tls_client_destroy(tcp_client);

            return;
        }
    }

    // http (default)
    add_http_server_connection(self, tcp_client);
}

//---------------------------------------------------------------------------//
// tcp
//---------------------------------------------------------------------------//

static void
http_server_on_tcp_close(
    http_server_thread* self,
    co_tcp_client_t* tcp_client
)
{
    (void)self;

    co_tls_client_destroy(tcp_client);
}

static void
http_server_on_tcp_accept(
    http_server_thread* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
)
{
    (void)tcp_server;

    co_tcp_accept((co_thread_t*)self, tcp_client);

    co_tcp_callbacks_st* tcp_callbacks =
        co_tcp_get_callbacks(tcp_client);
    tcp_callbacks->on_close =
        (co_tcp_close_fn)http_server_on_tcp_close;

    co_tls_callbacks_st* tls_callbacks =
        co_tls_get_callbacks(tcp_client);
    tls_callbacks->on_handshake =
        (co_tls_handshake_fn)http_server_on_tls_handshake;

    co_tls_start_handshake(tcp_client);
}

//---------------------------------------------------------------------------//
// thread
//---------------------------------------------------------------------------//

static bool
test_server_tls_setup(
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
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, self->port);

    co_tls_ctx_st tls_ctx = { 0 };
    
    if (!test_server_tls_setup(self, &tls_ctx))
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
