#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_tcp_server.h>
#include <coldforce/net/co_udp.h>

#include <coldforce/tls/co_tls_client.h>
#include <coldforce/tls/co_tls_log.h>

//---------------------------------------------------------------------------//
// tls client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#ifdef CO_USE_OPENSSL_COMPATIBLE

static void
co_tls_on_info(
    const SSL* ssl,
    int where,
    int ret
)
{
    co_socket_t* sock =
        (co_socket_t*)SSL_get_ex_data(ssl, 0);

    if (where & SSL_CB_ALERT)
    {
        co_tls_log_warning(
            &sock->local.net_addr, "---", &sock->remote.net_addr,
            "0x%04X-%s %s (%d) %s",
            where, SSL_alert_type_string_long(ret), SSL_state_string_long(ssl),
            ret, SSL_alert_desc_string_long(ret));
    }
    else
    {
        co_tls_log_info(
            &sock->local.net_addr, "---", &sock->remote.net_addr,
            "0x%04X %s",
            where, SSL_state_string_long(ssl));
    }

    if (where & SSL_CB_HANDSHAKE_DONE)
    {
        co_tls_log_info(
            &sock->local.net_addr, "---", &sock->remote.net_addr,
            "tls handshake done");
    }
}

bool
co_tls_client_setup_internal(
    co_tls_client_t* tls,
    co_tls_ctx_st* tls_ctx,
    co_socket_t* sock
)
{
    if ((tls_ctx == NULL) || (tls_ctx->ssl_ctx == NULL))
    {
        return false;
    }
    
    tls->ctx.ssl_ctx = tls_ctx->ssl_ctx;
    tls->ssl = SSL_new(tls->ctx.ssl_ctx);

    SSL_set_ex_data(tls->ssl, 0, sock);
    SSL_CTX_set_info_callback(tls->ctx.ssl_ctx, co_tls_on_info);

    BIO* internal_bio = BIO_new(BIO_s_bio());
    tls->network_bio = BIO_new(BIO_s_bio());

    (void)BIO_make_bio_pair(internal_bio, tls->network_bio);
    SSL_set_bio(tls->ssl, internal_bio, internal_bio);

    tls->callbacks.on_handshake = NULL;
    tls->on_receive_origin = NULL;
    tls->handshake_timer = NULL;
    tls->send_data = co_byte_array_create();
    tls->receive_data_queue = co_queue_create(sizeof(uint8_t), NULL);

    return true;
}

void
co_tls_client_cleanup_internal(
    co_tls_client_t* tls
)
{
    if (tls != NULL)
    {
        co_byte_array_destroy(tls->send_data);
        tls->send_data = NULL;

        co_queue_destroy(tls->receive_data_queue);
        tls->receive_data_queue = NULL;

        co_timer_destroy(tls->handshake_timer);
        tls->handshake_timer = NULL;

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

bool
co_tls_client_setup(
    co_socket_t* sock,
    co_tls_ctx_st* tls_ctx
)
{
    co_tls_client_t* tls =
        (co_tls_client_t*)co_mem_alloc(sizeof(co_tls_client_t));

    if (tls == NULL)
    {
        return false;
    }

    if (!co_tls_client_setup_internal(tls, tls_ctx, sock))
    {
        co_mem_free(tls);

        return false;
    }
    
    SSL_set_connect_state(tls->ssl);

    sock->tls = tls;

    return true;
}

void
co_tls_client_cleanup(
    co_socket_t* sock
)
{
    if (sock != NULL && sock->tls != NULL)
    {
        co_tls_client_cleanup_internal(sock->tls);
        co_mem_free(sock->tls);

        sock->tls = NULL;
    }
}

static bool
co_tls_send_handshake(
    co_socket_t* sock
)
{
    co_tls_log_info(
        &sock->local.net_addr,
        "-->",
        &sock->remote.net_addr,
        "tls handshake send");

    co_tls_client_t* tls = (co_tls_client_t*)sock->tls;

    while (BIO_ctrl_pending(tls->network_bio) > 0)
    {
        char buffer[8192];

        int bio_result =
            BIO_read(tls->network_bio, buffer, sizeof(buffer));

        co_socket_handle_set_blocking(sock->handle, true);

        if (co_socket_type_is_tcp(sock))
        {
            co_tcp_log_debug_hex_dump(
                &sock->local.net_addr,
                "-->",
                &sock->remote.net_addr,
                buffer, bio_result,
                "tcp send %d bytes", bio_result);
        }
        else
        {
            co_udp_log_debug_hex_dump(
                &sock->local.net_addr,
                "-->",
                &sock->remote.net_addr,
                buffer, bio_result,
                "udp send %d bytes", bio_result);
        }

        ssize_t sent_size = co_socket_handle_send(
            sock->handle, buffer, (size_t)bio_result, 0);

        co_socket_handle_set_blocking(sock->handle, false);

        if (sent_size <= 0)
        {
            return false;
        }
    }

    return true;
}

static void
co_tls_finished_handshake(
    co_thread_t* thread,
    co_socket_t* sock,
    int error_code
)
{
    co_tls_client_t* tls = (co_tls_client_t*)sock->tls;

    co_timer_destroy(tls->handshake_timer);
    tls->handshake_timer = NULL;

    if (error_code == 0)
    {
        co_tls_log_info(
            &sock->local.net_addr, "---", &sock->remote.net_addr,
            "%s %s", SSL_get_version(tls->ssl), SSL_get_cipher_name(tls->ssl));

        co_event_id_t event_id;

        if (co_socket_type_is_tcp(sock))
        {
            event_id = CO_NET_EVENT_ID_TCP_RECEIVE_READY;
        }
        else
        {
            event_id = CO_NET_EVENT_ID_UDP_RECEIVE_READY;
        }

        co_thread_send_event(
            sock->owner_thread,
            event_id,
            (uintptr_t)sock,
            0);
    }

    co_tls_log_debug_certificate(tls->ssl);

    co_tls_log_info(
        &sock->local.net_addr,
        "---",
        &sock->remote.net_addr,
        "tls handshake finished (%d)", error_code);

    if (co_socket_type_is_tcp(sock))
    {
        ((co_tcp_client_t*)sock)->callbacks.on_receive =
            (co_tcp_receive_fn)tls->on_receive_origin;
    }
    else
    {
        ((co_udp_t*)sock)->callbacks.on_receive =
            (co_udp_receive_fn)tls->on_receive_origin;
    }

    tls->on_receive_origin = NULL;

    if (thread != NULL && tls->callbacks.on_handshake != NULL)
    {
        tls->callbacks.on_handshake(thread, sock, error_code);
    }
}

bool
co_tls_receive_handshake(
    co_thread_t* thread,
    co_socket_t* sock
)
{
    co_tls_log_info(
        &sock->local.net_addr,
        "<--",
        &sock->remote.net_addr,
        "tls handshake receive");

    int error_code = 0;

    co_tls_client_t* tls = (co_tls_client_t*)sock->tls;

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
        else
        {
            error_code = CO_TLS_ERROR_HANDSHAKE_FAILED;

            break;
        }
    }

    if (error_code == 0)
    {
        int ssl_result = SSL_do_handshake(tls->ssl);
        int ssl_error = SSL_get_error(tls->ssl, ssl_result);

        if (!SSL_is_init_finished(tls->ssl))
        {
            if ((ssl_error == SSL_ERROR_WANT_READ) ||
                (ssl_error == SSL_ERROR_WANT_WRITE))
            {
                co_tls_send_handshake(sock);

                return false;
            }
            else
            {
                co_tls_log_error(
                    &sock->local.net_addr,
                    "---",
                    &sock->remote.net_addr,
                    "tls handshake error: (%d)",
                    ssl_error);

                error_code = CO_TLS_ERROR_HANDSHAKE_FAILED;
            }
        }
        else
        {
            co_tls_send_handshake(sock);
        }
    }

    co_tls_finished_handshake(thread, sock, error_code);

    return true;
}

void
co_tls_on_receive_handshake(
    co_thread_t* thread,
    co_socket_t* sock
)
{
    co_tls_client_t* tls = (co_tls_client_t*)sock->tls;

#ifdef CO_OS_WIN

    co_queue_push_array(
        tls->receive_data_queue,
        co_win_socket_get_receive_buffer(sock),
        co_win_socket_get_receive_data_size(sock));

    if (co_socket_type_is_tcp(sock))
    {
        co_tcp_log_debug_hex_dump(
            &sock->local.net_addr,
            "<--",
            &sock->remote.net_addr,
            co_win_socket_get_receive_buffer(sock),
            co_win_socket_get_receive_data_size(sock),
            "tcp receive %d bytes",
            co_win_socket_get_receive_data_size(sock));
    }
    else
    {
        co_udp_log_debug_hex_dump(
            &sock->local.net_addr,
            "<--",
            &sock->remote.net_addr,
            co_win_socket_get_receive_buffer(sock),
            co_win_socket_get_receive_data_size(sock),
            "udp receive %d bytes",
            co_win_socket_get_receive_data_size(sock));
    }

    co_win_socket_clear_receive_buffer(sock);

#else

    for (;;)
    {
        char buffer[8192];

        ssize_t data_size =
            co_socket_handle_receive(
                sock->handle, buffer, sizeof(buffer), 0);

        if (data_size <= 0)
        {
            break;
        }

        if (co_socket_type_is_tcp(sock))
        {
            co_tcp_log_debug_hex_dump(
                &sock->local.net_addr,
                "<--",
                &sock->remote.net_addr,
                buffer, data_size,
                "tcp receive %d bytes", data_size);
        }
        else
        {
            co_udp_log_debug_hex_dump(
                &sock->local.net_addr,
                "<--",
                &sock->remote.net_addr,
                buffer, data_size,
                "udp receive %d bytes", data_size);
        }

        co_queue_push_array(
            tls->receive_data_queue, buffer, data_size);
    }

#endif

    co_tls_receive_handshake(thread, sock);
}

static void
co_tls_on_handshake_timer(
    co_thread_t* thread,
    co_timer_t* timer
)
{
    co_socket_t* sock =
        (co_socket_t*)co_timer_get_user_data(timer);

    co_tls_log_error(
        &sock->local.net_addr, "<--", &sock->remote.net_addr,
        "tls handshake timeout");

    co_tls_finished_handshake(
        thread, sock, CO_TLS_ERROR_HANDSHAKE_FAILED);
}

bool
co_tls_encrypt_data(
    co_socket_t* sock,
    const void* plain_data,
    size_t plain_data_size,
    co_byte_array_t* enc_data
)
{
    co_tls_client_t* tls = (co_tls_client_t*)sock->tls;

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

ssize_t
co_tls_decrypt_data(
    co_socket_t* sock,
    co_queue_t* enc_data,
    uint8_t* plain_buffer,
    size_t plain_buffer_size
)
{
    co_tls_client_t* tls =
        (co_tls_client_t*)sock->tls;
    
    while (co_queue_get_count(enc_data) > 0)
    {
        char temp_buffer[8192];

        size_t data_size = co_queue_peek_array(
            enc_data, temp_buffer, sizeof(temp_buffer));

        int bio_result =
            BIO_write(tls->network_bio, temp_buffer, (int)data_size);

        if (bio_result > 0)
        {
            co_queue_remove(enc_data, (size_t)bio_result);
        }
        else
        {
            break;
        }
    }

    int ssl_result =
        SSL_read(tls->ssl, plain_buffer, (int)plain_buffer_size);

    return ssl_result;
}

bool
co_tls_start_handshake(
    co_socket_t* sock,
    uint32_t timeout_msec
)
{
    co_tls_client_t* tls =
        (co_tls_client_t*)sock->tls;

    if (tls == NULL)
    {
        return false;
    }

    co_tls_log_info(
        &sock->local.net_addr,
        "-->",
        &sock->remote.net_addr,
        "tls handshake start");

    tls->handshake_timer =
        co_timer_create(timeout_msec,
            co_tls_on_handshake_timer, false, sock);
    co_timer_start(tls->handshake_timer);

    int ssl_result = SSL_do_handshake(tls->ssl);
    int ssl_error = SSL_get_error(tls->ssl, ssl_result);

    if (ssl_error == SSL_ERROR_WANT_READ)
    {
        if (SSL_is_server(tls->ssl) == 0)
        {
            return co_tls_send_handshake(sock);
        }

        return true;
    }

    return false;
}

#endif // CO_USE_OPENSSL_COMPATIBLE

co_tls_callbacks_st*
co_tls_get_callbacks(
    co_socket_t* sock
)
{
    co_tls_client_t* tls =
        (co_tls_client_t*)sock->tls;

    if (tls != NULL)
    {
        return &tls->callbacks;
    }

    return NULL;
}
