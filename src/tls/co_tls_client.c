#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_event.h>

#include <coldforce/tls/co_tls_client.h>
#include <coldforce/tls/co_tls_log.h>

#ifdef CO_CAN_USE_TLS

//---------------------------------------------------------------------------//
// tls tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static void
co_tls_on_info(
    const SSL* ssl,
    int where,
    int ret
)
{
    co_tcp_client_t* client =
        (co_tcp_client_t*)SSL_get_ex_data(ssl, 0);

    co_tls_log_info(
        &client->sock.local_net_addr, "---", &client->remote_net_addr,
        "0x%08X-%s %s (%d) %s",
        where, SSL_alert_type_string(ret), SSL_state_string_long(ssl),
        ret, SSL_alert_desc_string_long(ret));

    if (where == SSL_CB_HANDSHAKE_DONE)
    {
        co_tls_log_info(
            &client->sock.local_net_addr, "---", &client->remote_net_addr,
            "%s %s", SSL_get_version(ssl), SSL_get_cipher_name(ssl));
    }
}

void
co_tls_client_setup(
    co_tls_client_t* tls,
    co_tls_ctx_st* tls_ctx,
    co_tcp_client_t* client
)
{
    SSL_CTX* ssl_ctx = NULL;

    if ((tls_ctx == NULL) || (tls_ctx->ssl_ctx == NULL))
    {
        ssl_ctx = SSL_CTX_new(TLS_client_method());
    }
    else
    {
        ssl_ctx = tls_ctx->ssl_ctx;
    }

    tls->ctx.ssl_ctx = ssl_ctx;
    tls->ssl = SSL_new(tls->ctx.ssl_ctx);

    SSL_set_ex_data(tls->ssl, 0, client);
    SSL_CTX_set_info_callback(ssl_ctx, co_tls_on_info);

    BIO* internal_bio = BIO_new(BIO_s_bio());
    tls->network_bio = BIO_new(BIO_s_bio());

    BIO_make_bio_pair(internal_bio, tls->network_bio);
    SSL_set_bio(tls->ssl, internal_bio, internal_bio);

    tls->on_connect = NULL;
    tls->on_receive_ready = NULL;
    tls->on_handshake_complete = NULL;
    tls->receive_data_queue = co_queue_create(sizeof(uint8_t), NULL);
}

void
co_tls_client_cleanup(
    co_tls_client_t* tls
)
{
    if (tls != NULL)
    {
        co_queue_destroy(tls->receive_data_queue);
        tls->receive_data_queue = NULL;

        if (tls->network_bio != NULL)
        {
            BIO_free(tls->network_bio);
            tls->network_bio = NULL;
        }

        if (tls->ssl != NULL)
        {
            SSL_free(tls->ssl);
            tls->ssl = NULL;
        }

        if (tls->ctx.ssl_ctx != NULL)
        {
            SSL_CTX_free(tls->ctx.ssl_ctx);
            tls->ctx.ssl_ctx = NULL;
        }
    }
}

static bool
co_tls_send_handshake(
    co_tcp_client_t* client
)
{
    co_tls_log_info(
        &client->sock.local_net_addr,
        "-->",
        &client->remote_net_addr,
        "tls handshake send");

    co_tls_client_t* tls = co_tcp_client_get_tls(client);

    while (BIO_ctrl_pending(tls->network_bio) > 0)
    {
        char buffer[8192];

        int bio_result =
            BIO_read(tls->network_bio, buffer, sizeof(buffer));

        co_socket_handle_set_blocking(client->sock.handle, true);

        co_tcp_log_debug_hex_dump(
            &client->sock.local_net_addr,
            "-->",
            &client->remote_net_addr,
            buffer, bio_result,
            "tcp send %d bytes", bio_result);

        ssize_t sent_size = co_socket_handle_send(
            client->sock.handle, buffer, (size_t)bio_result, 0);

        co_socket_handle_set_blocking(client->sock.handle, false);

        if (sent_size <= 0)
        {
            return false;
        }
    }

    return true;
}

static bool
co_tls_receive_handshake(
    co_tcp_client_t* client
)
{
    co_tls_client_t* tls = co_tcp_client_get_tls(client);

#ifdef CO_OS_WIN

    co_queue_push_array(
        tls->receive_data_queue,
        co_win_tcp_get_receive_buffer(client),
        co_win_tcp_get_receive_data_size(client));

    co_win_tcp_clear_receive_buffer(client);

#else

    for (;;)
    {
        char buffer[8192];

        ssize_t data_size =
            co_socket_handle_receive(
                client->sock.handle, buffer, sizeof(buffer), 0);

        if (data_size <= 0)
        {
            break;
        }

        co_tcp_log_debug_hex_dump(
            &client->sock.local_net_addr,
            "<--",
            &client->remote_net_addr,
            buffer, data_size,
            "tcp receive %d bytes", data_size);

        co_queue_push_array(
            tls->receive_data_queue, buffer, data_size);
    }

#endif

    co_tls_log_info(
        &client->sock.local_net_addr,
        "<--",
        &client->remote_net_addr,
        "tls handshake receive");

    while (co_queue_get_count(tls->receive_data_queue) > 0)
    {
        char buffer[8192];

        size_t size = co_queue_peek_array(
            tls->receive_data_queue, buffer, sizeof(buffer));

        int bio_result = BIO_write(tls->network_bio, buffer, (int)size);

        if (bio_result > 0)
        {
            co_queue_remove(tls->receive_data_queue, bio_result);
        }
        else if (BIO_should_retry(tls->network_bio))
        {
            break;
        }
        else
        {
            return false;
        }
    }
    
    return true;
}

static bool
co_tls_encrypt_data(
    co_tcp_client_t* client,
    const void* plain_data,
    size_t plain_data_size,
    co_byte_array_t* enc_data
)
{
    co_tls_client_t* tls = co_tcp_client_get_tls(client);

    size_t plain_index = 0;
    size_t enc_index = 0;

    for (;;)
    {
        if (plain_index < plain_data_size)
        {
            int ssl_result = SSL_write(tls->ssl,
                &((const uint8_t*)plain_data)[plain_index],
                (int)(plain_data_size - plain_index));

            if (ssl_result > 0)
            {
                plain_index += (size_t)ssl_result;
            }
        }

        size_t pending_size = BIO_ctrl_pending(tls->network_bio);

        if (pending_size > 0)
        {
            co_byte_array_set_count(enc_data,
                co_byte_array_get_count(enc_data) + pending_size);
        }
        else
        {
            break;
        }
        
        int bio_result = BIO_read(tls->network_bio,
            co_byte_array_get_ptr(enc_data, enc_index), (int)pending_size);

        if (bio_result > 0)
        {
            enc_index += (size_t)bio_result;
        }
    }

    return true;
}

static void
co_tls_on_handshake_complete(
    co_thread_t* thread,
    co_tcp_client_t* client,
    int error_code
)
{
    if (error_code != 0)
    {
        co_tls_client_close(client);
    }

    co_tls_client_t* tls = co_tcp_client_get_tls(client);

    if (tls->on_connect != NULL)
    {
        co_tcp_connect_fn handler = tls->on_connect;

        handler(thread, client, error_code);
    }
}

static void
co_tls_on_connect(
    co_thread_t* thread,
    co_tcp_client_t* client,
    int error_code
)
{
    if (error_code == 0)
    {
        co_tls_start_handshake(client,
            (co_tls_handshake_fn)co_tls_on_handshake_complete);
    }
    else
    {
        co_tls_client_t* tls = co_tcp_client_get_tls(client);

        if (tls->on_connect != NULL)
        {
            co_tcp_connect_fn handler = tls->on_connect;

            handler(thread, client, error_code);
        }
    }
}

static void
co_tls_on_receive_handshake(
    co_thread_t* thread,
    co_tcp_client_t* client
)
{
    (void)thread;

    co_tls_client_t* tls = co_tcp_client_get_tls(client);

    int error_code = 0;

    if (co_tls_receive_handshake(client))
    {
        int ssl_result = SSL_do_handshake(tls->ssl);
        int ssl_error = SSL_get_error(tls->ssl, ssl_result);

        if (!SSL_is_init_finished(tls->ssl))
        {
            if ((ssl_error == SSL_ERROR_WANT_READ) ||
                (ssl_error == SSL_ERROR_WANT_WRITE))
            {
                co_tls_send_handshake(client);

                return;
            }
            else
            {
                error_code = CO_TLS_ERROR_HANDSHAKE_FAILED;
            }
        }
        else
        {
            co_tls_send_handshake(client);
        }
    }
    else
    {
        error_code = CO_TLS_ERROR_HANDSHAKE_FAILED;
    }

    co_tls_log_info(
        &client->sock.local_net_addr,
        "<--",
        &client->remote_net_addr,
        "tls handshake finished (%d)", error_code);

    co_tls_log_debug_certificate(
        SSL_get_peer_certificate(tls->ssl));

    if (error_code == 0)
    {
        co_thread_send_event(
            client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_RECEIVE_READY,
            (uintptr_t)client,
            0);
    }

    if (tls->on_handshake_complete != NULL)
    {
        client->on_receive_ready = tls->on_receive_ready;
        tls->on_receive_ready = NULL;

        tls->on_handshake_complete(thread, client, error_code);
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_tcp_client_t*
co_tls_client_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_tcp_client_t* client = co_tcp_client_create(local_net_addr);

    if (client == NULL)
    {
        return NULL;
    }

    if (!co_tls_client_install(client, tls_ctx))
    {
        co_tcp_client_destroy(client);

        return NULL;
    }

    return client;
}

void
co_tls_client_destroy(
    co_tcp_client_t* client
)
{
    if (client != NULL)
    {
        if (client->sock.tls != NULL)
        {
            co_tls_client_cleanup(client->sock.tls);
            co_mem_free(client->sock.tls);

            client->sock.tls = NULL;
        }

        co_tcp_client_destroy(client);
    }
}

void
co_tls_client_close(
    co_tcp_client_t* client
)
{
    if (client != NULL)
    {
        co_tcp_client_close(client);
    }
}

bool
co_tls_client_install(
    co_tcp_client_t* client,
    co_tls_ctx_st* tls_ctx
)
{
    co_tls_client_t* tls =
        (co_tls_client_t*)co_mem_alloc(sizeof(co_tls_client_t));

    if (tls == NULL)
    {
        return false;
    }

    co_tls_client_setup(tls, tls_ctx, client);
    SSL_set_connect_state(tls->ssl);

    client->sock.tls = tls;

    return true;
}

void
co_tls_set_host_name(
    co_tcp_client_t* client,
    const char* host_name
)
{
    SSL_set_tlsext_host_name(
        co_tcp_client_get_tls(client)->ssl, host_name);
}

void
co_tls_set_available_protocols(
    co_tcp_client_t* client,
    const char* protocols[],
    size_t count
)
{
    co_byte_array_t* buffer = co_byte_array_create();

    for (size_t index = 0; index < count; ++index)
    {
        uint8_t length = (uint8_t)strlen(protocols[index]);

        co_byte_array_add(buffer, &length, 1);
        co_byte_array_add(buffer, protocols[index], length);
    }

    SSL_set_alpn_protos(
        co_tcp_client_get_tls(client)->ssl,
        co_byte_array_get_ptr(buffer, 0),
        (unsigned int)co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);
}

bool
co_tls_get_selected_protocol(
    const co_tcp_client_t* client,
    char* buffer,
    size_t buffer_size
)
{
    const unsigned char* data = NULL;
    unsigned int length = 0;

    SSL_get0_alpn_selected(
        co_tcp_client_get_tls(client)->ssl,
        &data, &length);

    if ((data != NULL) && (length > 0))
    {
        length = (unsigned int)co_min(length, (buffer_size - 1));

        memcpy(buffer, data, length);
        buffer[length] = '\0';

        return true;
    }
    else
    {
        return false;
    }
}

bool
co_tls_connect(
    co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr,
    co_tcp_connect_fn handler
)
{
    co_tls_client_t* tls = co_tcp_client_get_tls(client);

    tls->on_connect = handler;

    return co_tcp_connect(
        client, remote_net_addr,
        (co_tcp_connect_fn)co_tls_on_connect);
}

bool
co_tls_start_handshake(
    co_tcp_client_t* client,
    co_tls_handshake_fn handler
)
{
    co_tls_log_info(
        &client->sock.local_net_addr,
        "-->",
        &client->remote_net_addr,
        "tls handshake start");

    co_tls_client_t* tls = co_tcp_client_get_tls(client);

    if (tls == NULL)
    {
        return false;
    }

    tls->on_handshake_complete = handler;
    tls->on_receive_ready = client->on_receive_ready;
    client->on_receive_ready =
        (co_tcp_receive_fn)co_tls_on_receive_handshake;

    int ssl_result = SSL_do_handshake(tls->ssl);
    int ssl_error = SSL_get_error(tls->ssl, ssl_result);

    if (ssl_error == SSL_ERROR_WANT_READ)
    {
        if (SSL_is_server(tls->ssl) == 0)
        {
            return co_tls_send_handshake(client);
        }

        return true;
    }

    return false;
}

bool
co_tls_send(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
)
{
    co_tls_log_debug_hex_dump(
        &client->sock.local_net_addr,
        "-->",
        &client->remote_net_addr,
        data, data_size,
        "tls send %zd bytes", data_size);

    bool result = false;

    co_byte_array_t* send_data = co_byte_array_create();

    if (co_tls_encrypt_data(
        client, data, data_size, send_data))
    {
        result = co_tcp_send(client,
            co_byte_array_get_ptr(send_data, 0),
            co_byte_array_get_count(send_data));
    }
    
    co_byte_array_destroy(send_data);

    return result;
}

bool
co_tls_send_async(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
)
{
    co_tls_log_debug_hex_dump(
        &client->sock.local_net_addr,
        "-->",
        &client->remote_net_addr,
        data, data_size,
        "tls send async %zd bytes", data_size);

    bool result = false;

    co_byte_array_t* send_data = co_byte_array_create();

    if (co_tls_encrypt_data(
        client, data, data_size, send_data))
    {
        result = co_tcp_send_async(client,
            co_byte_array_get_ptr(send_data, 0),
            co_byte_array_get_count(send_data));
    }

    co_byte_array_destroy(send_data);

    return result;
}

ssize_t
co_tls_receive(
    co_tcp_client_t* client,
    void* buffer,
    size_t buffer_size
)
{
    co_tls_client_t* tls = co_tcp_client_get_tls(client);

    ssize_t raw_data_size =
        co_tcp_receive(client, buffer, buffer_size);

    if (raw_data_size > 0)
    {
        co_queue_push_array(
            tls->receive_data_queue, buffer, raw_data_size);
    }

    if (co_queue_get_count(tls->receive_data_queue) > 0)
    {
        size_t data_size = co_queue_peek_array(
            tls->receive_data_queue, buffer, buffer_size);

        int bio_result =
            BIO_write(tls->network_bio, buffer, (int)data_size);

        if (bio_result > 0)
        {
            co_queue_remove(tls->receive_data_queue, (size_t)bio_result);
        }
    }

    int ssl_result =
        SSL_read(tls->ssl, buffer, (int)buffer_size);

    if (ssl_result > 0)
    {
        co_tls_log_debug_hex_dump(
            &client->sock.local_net_addr,
            "<--",
            &client->remote_net_addr,
            buffer, ssl_result,
            "tls receive %d bytes", ssl_result);
    }

    return ssl_result;
}

ssize_t
co_tls_receive_all(
    co_tcp_client_t* client,
    co_byte_array_t* byte_array
)
{
    size_t before = co_byte_array_get_count(byte_array);

    for (;;)
    {
        char buffer[8192];

        ssize_t result =
            co_tls_receive(client, buffer, sizeof(buffer));

        if (result <= 0)
        {
            break;
        }
        else
        {
            co_byte_array_add(byte_array, buffer, result);
        }
    }

    return (ssize_t)(co_byte_array_get_count(byte_array) - before);
}

bool
co_tls_is_open(
    const co_tcp_client_t* client
)
{
    return co_tcp_is_open(client);
}

void
co_tls_set_send_complete_handler(
    co_tcp_client_t* client,
    co_tcp_send_fn handler
)
{
    co_tcp_set_send_complete_handler(client, handler);
}

void
co_tls_set_receive_handler(
    co_tcp_client_t* client,
    co_tcp_receive_fn handler
)
{
    co_tcp_set_receive_handler(client, handler);
}

void
co_tls_set_close_handler(
    co_tcp_client_t* client,
    co_tcp_close_fn handler
)
{
    co_tcp_set_close_handler(client, handler);
}

const co_net_addr_t*
co_tls_get_remote_net_addr(
    const co_tcp_client_t* client
)
{
    return co_tcp_get_remote_net_addr(client);
}

#endif // CO_CAN_USE_TLS
