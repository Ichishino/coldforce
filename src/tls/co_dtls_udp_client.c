#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_dtls_udp_client.h>
#include <coldforce/tls/co_dtls_udp_server.h>
#include <coldforce/tls/co_tls_config.h>
#include <coldforce/tls/co_tls_log.h>

//---------------------------------------------------------------------------//
// tls tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_udp_t*
co_dtls_udp_client_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
#ifdef CO_USE_TLS

    co_udp_t* udp =
        co_udp_create(local_net_addr);

    if (udp == NULL)
    {
        return NULL;
    }

    if (!co_tls_client_setup(&udp->sock, tls_ctx))
    {
        co_udp_destroy(udp);

        return NULL;
    }

    return udp;

#else

    (void)local_net_addr;
    (void)tls_ctx;

    return NULL;

#endif // CO_USE_TLS
}

void
co_dtls_udp_client_destroy(
    co_udp_t* udp
)
{
#ifdef CO_USE_TLS

    if (udp != NULL)
    {
        co_tls_client_t* tls =
            (co_tls_client_t*)udp->sock.tls;

        int state =
            (tls != NULL) ? SSL_is_server(tls->ssl) : 0;

        co_tls_client_cleanup(&udp->sock);

        if (state == 1)
        {
            co_udp_destroy_connection(udp);
        }
        else
        {
            co_udp_destroy(udp);
        }
    }

#else

    (void)udp;

#endif // CO_USE_TLS
}

bool
co_dtls_udp_handshake_start(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr
)
{
#ifdef CO_USE_TLS

    const uint8_t* data = NULL;
    size_t data_size = 0;

    if (remote_net_addr != NULL)
    {
        if (!co_udp_connect(udp, remote_net_addr))
        {
            return false;
        }

        if (!co_udp_receive_start(udp))
        {
            return false;
        }
    }
    else
    {
        data_size =
            co_udp_get_accept_data(udp, &data);
    }

    co_tls_client_t* tls =
        (co_tls_client_t*)udp->sock.tls;

    tls->on_receive_origin =
        (void*)udp->callbacks.on_receive;
    udp->callbacks.on_receive =
        (co_udp_receive_fn)co_tls_on_handshake_receive;

    if (!co_tls_handshake_start(
        &udp->sock, co_tls_get_config()->handshake_timeout))
    {
        return false;
    }

    if (data != NULL && data_size > 0)
    {
        co_queue_push_array(
            tls->receive_data_queue, data, data_size);

        if (co_tls_handshake_receive(NULL, &udp->sock))
        {
            return false;
        }
    }

    return true;

#else

    (void)udp;
    (void)remote_net_addr;

    return false;

#endif // CO_USE_TLS
}

bool
co_dtls_udp_send(
    co_udp_t* udp,
    const void* data,
    size_t data_size
)
{
#ifdef CO_USE_TLS

    co_tls_log_debug_hex_dump(
        &udp->sock.local.net_addr,
        "-->",
        &udp->sock.remote.net_addr,
        data, data_size,
        "tls send %zd bytes", data_size);

    co_tls_client_t* tls =
        (co_tls_client_t*)udp->sock.tls;

    co_byte_array_clear(tls->send_data);

    bool result = false;

    if (co_tls_encrypt_data(
        &udp->sock, data, data_size, tls->send_data))
    {
        result = co_udp_send(udp,
            co_byte_array_get_ptr(tls->send_data, 0),
            co_byte_array_get_count(tls->send_data));
    }

    return result;

#else

    (void)udp;
    (void)data;
    (void)data_size;

    return false;

#endif // CO_USE_TLS
}

bool
co_dtls_udp_send_async(
    co_udp_t* udp,
    const void* data,
    size_t data_size,
    void* user_data
)
{
#ifdef CO_USE_TLS

    co_tls_log_debug_hex_dump(
        &udp->sock.local.net_addr,
        "-->",
        &udp->sock.remote.net_addr,
        data, data_size,
        "tls send async %zd bytes", data_size);

    co_tls_client_t* tls =
        (co_tls_client_t*)udp->sock.tls;

    co_byte_array_clear(tls->send_data);

    bool result = false;

    if (co_tls_encrypt_data(
        &udp->sock, data, data_size, tls->send_data))
    {
        result = co_udp_send_async(udp,
            co_byte_array_get_ptr(tls->send_data, 0),
            co_byte_array_get_count(tls->send_data),
            user_data);
    }

    return result;

#else

    (void)udp;
    (void)data;
    (void)data_size;
    (void)user_data;

    return false;

#endif // CO_USE_TLS
}

ssize_t
co_dtls_udp_receive(
    co_udp_t* udp,
    void* buffer,
    size_t buffer_size
)
{
#ifdef CO_USE_OPENSSL_COMPATIBLE

    co_tls_client_t* tls =
        (co_tls_client_t*)udp->sock.tls;

    ssize_t enc_data_size =
        co_udp_receive(udp, buffer, buffer_size);

    if (enc_data_size > 0)
    {
        co_queue_push_array(
            tls->receive_data_queue, buffer, enc_data_size);
    }

    ssize_t plain_data_size =
        co_tls_decrypt_data(&udp->sock,
            tls->receive_data_queue,
            buffer, buffer_size);

    if (plain_data_size > 0)
    {
        co_tls_log_debug_hex_dump(
            &udp->sock.local.net_addr,
            "<--",
            &udp->sock.remote.net_addr,
            buffer, plain_data_size,
            "tls receive %d bytes", plain_data_size);
    }
    else if (enc_data_size > 0)
    {
        int ssl_error =
            SSL_get_error(tls->ssl, (int)plain_data_size);

        if (ssl_error == SSL_ERROR_WANT_READ)
        {
            return co_dtls_udp_receive(udp, buffer, buffer_size);
        }
        else
        {
            co_tls_log_error(
                &udp->sock.local.net_addr,
                "<--",
                &udp->sock.remote.net_addr,
                "tls receive error: (%d)", ssl_error);
        }
    }

    return plain_data_size;

#else

    (void)udp;
    (void)buffer;
    (void)buffer_size;

    return -1;

#endif // CO_USE_OPENSSL_COMPATIBLE
}

ssize_t
co_dtls_udp_receive_all(
    co_udp_t* udp,
    co_byte_array_t* byte_array
)
{
#ifdef CO_USE_TLS

    co_tls_client_t* tls =
        (co_tls_client_t*)udp->sock.tls;

    size_t array_size_before =
        co_byte_array_get_count(byte_array);

    for (;;)
    {
        uint8_t buffer[8192];

        ssize_t enc_data_size =
            co_udp_receive(udp, buffer, sizeof(buffer));

        if (enc_data_size > 0)
        {
            co_queue_push_array(
                tls->receive_data_queue, buffer, enc_data_size);
        }

        ssize_t plain_data_size =
            co_tls_decrypt_data(&udp->sock,
                tls->receive_data_queue,
                buffer, sizeof(buffer));

        if (plain_data_size <= 0)
        {
            if (enc_data_size <= 0)
            {
                break;
            }
        }
        else
        {
            co_byte_array_add(
                byte_array, buffer, plain_data_size);
        }
    }

    size_t receive_size =
        co_byte_array_get_count(byte_array) - array_size_before;

    if (receive_size > 0)
    {
        co_tls_log_debug_hex_dump(
            &udp->sock.local.net_addr,
            "<--",
            &udp->sock.remote.net_addr,
            co_byte_array_get_ptr(byte_array, array_size_before),
            receive_size,
            "tls receive %d bytes", receive_size);
    }

    return receive_size;

#else

    (void)udp;
    (void)byte_array;

    return -1;

#endif // CO_USE_TLS
}

co_tls_callbacks_st*
co_dtls_udp_get_callbacks(
    co_udp_t* udp
)
{
    co_tls_client_t* tls =
        (co_tls_client_t*)udp->sock.tls;

    if (tls != NULL)
    {
        return &tls->callbacks;
    }

    return NULL;
}
