#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls_tcp_client.h>
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

co_tcp_client_t*
co_tls_tcp_client_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
#ifdef CO_USE_TLS

    co_tcp_client_t* tcp_client =
        co_tcp_client_create(local_net_addr);

    if (tcp_client == NULL)
    {
        return NULL;
    }

    if (!co_tls_client_setup(&tcp_client->sock, tls_ctx))
    {
        co_tcp_client_destroy(tcp_client);

        return NULL;
    }

    return tcp_client;

#else

    (void)local_net_addr;
    (void)tls_ctx;

    return NULL;

#endif // CO_USE_TLS
}

void
co_tls_tcp_client_destroy(
    co_tcp_client_t* tcp_client
)
{
#ifdef CO_USE_TLS

    if (tcp_client != NULL)
    {
        co_tls_client_cleanup(&tcp_client->sock);
        co_tcp_client_destroy(tcp_client);
    }

#else

    (void)tcp_client;

#endif // CO_USE_TLS
}

bool
co_tls_tcp_handshake_start(
    co_tcp_client_t* tcp_client
)
{
#ifdef CO_USE_TLS

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    tls->on_receive_origin =
        (void*)tcp_client->callbacks.on_receive;
    tcp_client->callbacks.on_receive =
        (co_tcp_receive_fn)co_tls_on_handshake_receive;

    return co_tls_handshake_start(
        &tcp_client->sock, co_tls_get_config()->handshake_timeout);

#else

    (void)tcp_client;

    return false;

#endif // CO_USE_TLS
}

void
co_tls_tcp_set_host_name(
    co_tcp_client_t* tcp_client,
    const char* host_name
)
{
#ifdef CO_USE_OPENSSL_COMPATIBLE

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    SSL_set_tlsext_host_name(tls->ssl, host_name);

#else

    (void)tcp_client;
    (void)host_name;

#endif // CO_USE_OPENSSL_COMPATIBLE
}

void
co_tls_tcp_set_available_protocols(
    co_tcp_client_t* tcp_client,
    const char* protocols[],
    size_t count
)
{
#ifdef CO_USE_OPENSSL_COMPATIBLE

    co_byte_array_t* buffer = co_byte_array_create();

    for (size_t index = 0; index < count; ++index)
    {
        uint8_t length = (uint8_t)strlen(protocols[index]);

        co_byte_array_add(buffer, &length, 1);
        co_byte_array_add(buffer, protocols[index], length);
    }

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    SSL_set_alpn_protos(
        tls->ssl,
        co_byte_array_get_ptr(buffer, 0),
        (unsigned int)co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

#else

    (void)tcp_client;
    (void)protocols;
    (void)count;

#endif // CO_USE_OPENSSL_COMPATIBLE
}

bool
co_tls_tcp_get_selected_protocol(
    const co_tcp_client_t* tcp_client,
    char* buffer,
    size_t buffer_size
)
{
#ifdef CO_USE_OPENSSL_COMPATIBLE

    const unsigned char* data = NULL;
    unsigned int length = 0;

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    SSL_get0_alpn_selected(
        tls->ssl, &data, &length);

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

#else

    (void)tcp_client;
    (void)buffer;
    (void)buffer_size;

    return false;

#endif // CO_USE_OPENSSL_COMPATIBLE
}

bool
co_tls_tcp_send(
    co_tcp_client_t* tcp_client,
    const void* data,
    size_t data_size
)
{
#ifdef CO_USE_TLS

    co_tls_log_debug_hex_dump(
        &tcp_client->sock.local.net_addr,
        "-->",
        &tcp_client->sock.remote.net_addr,
        data, data_size,
        "tls send %zd bytes", data_size);

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    co_byte_array_clear(tls->send_data);

    bool result = false;

    if (co_tls_encrypt_data(
        &tcp_client->sock, data, data_size, tls->send_data))
    {
        result = co_tcp_send(tcp_client,
            co_byte_array_get_ptr(tls->send_data, 0),
            co_byte_array_get_count(tls->send_data));
    }
    
    return result;

#else

    (void)tcp_client;
    (void)data;
    (void)data_size;

    return false;

#endif // CO_USE_TLS
}

bool
co_tls_tcp_send_async(
    co_tcp_client_t* tcp_client,
    const void* data,
    size_t data_size,
    void* user_data
)
{
#ifdef CO_USE_TLS

    co_tls_log_debug_hex_dump(
        &tcp_client->sock.local.net_addr,
        "-->",
        &tcp_client->sock.remote.net_addr,
        data, data_size,
        "tls send async %zd bytes", data_size);

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    co_byte_array_clear(tls->send_data);

    bool result = false;

    if (co_tls_encrypt_data(
        &tcp_client->sock, data, data_size, tls->send_data))
    {
        result = co_tcp_send_async(tcp_client,
            co_byte_array_get_ptr(tls->send_data, 0),
            co_byte_array_get_count(tls->send_data),
            user_data);
    }

    return result;

#else

    (void)tcp_client;
    (void)data;
    (void)data_size;
    (void)user_data;

    return false;

#endif // CO_USE_TLS
}

ssize_t
co_tls_tcp_receive(
    co_tcp_client_t* tcp_client,
    void* buffer,
    size_t buffer_size
)
{
#ifdef CO_USE_OPENSSL_COMPATIBLE

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    ssize_t enc_data_size =
        co_tcp_receive(tcp_client, buffer, buffer_size);

    if (enc_data_size > 0)
    {
        co_queue_push_array(
            tls->receive_data_queue, buffer, enc_data_size);
    }

    ssize_t plain_data_size = 
        co_tls_decrypt_data(&tcp_client->sock,
            tls->receive_data_queue,
            buffer, buffer_size);

    if (plain_data_size > 0)
    {
        co_tls_log_debug_hex_dump(
            &tcp_client->sock.local.net_addr,
            "<--",
            &tcp_client->sock.remote.net_addr,
            buffer, plain_data_size,
            "tls receive %d bytes", plain_data_size);
    }
    else if (enc_data_size > 0)
    {
        int ssl_error =
            SSL_get_error(tls->ssl, (int)plain_data_size);

        if (ssl_error == SSL_ERROR_WANT_READ)
        {
            return co_tls_tcp_receive(tcp_client, buffer, buffer_size);
        }
        else
        {
            co_tls_log_error(
                &tcp_client->sock.local.net_addr,
                "<--",
                &tcp_client->sock.remote.net_addr,
                "tls receive error: (%d)", ssl_error);
        }
    }

    return plain_data_size;

#else

    (void)tcp_client;
    (void)buffer;
    (void)buffer_size;

    return -1;

#endif // CO_USE_OPENSSL_COMPATIBLE
}

ssize_t
co_tls_tcp_receive_all(
    co_tcp_client_t* tcp_client,
    co_byte_array_t* byte_array
)
{
#ifdef CO_USE_TLS

    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    size_t array_size_before =
        co_byte_array_get_count(byte_array);

    for (;;)
    {
        uint8_t buffer[8192];

        ssize_t enc_data_size =
            co_tcp_receive(tcp_client, buffer, sizeof(buffer));

        if (enc_data_size > 0)
        {
            co_queue_push_array(
                tls->receive_data_queue, buffer, enc_data_size);
        }

        ssize_t plain_data_size =
            co_tls_decrypt_data(&tcp_client->sock,
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
            &tcp_client->sock.local.net_addr,
            "<--",
            &tcp_client->sock.remote.net_addr,
            co_byte_array_get_ptr(byte_array, array_size_before),
            receive_size,
            "tls receive %d bytes", receive_size);
    }

    return receive_size;

#else

    (void)tcp_client;
    (void)byte_array;

    return -1;

#endif // CO_USE_TLS
}

co_tls_tcp_callbacks_st*
co_tls_tcp_get_callbacks(
    co_tcp_client_t* tcp_client
)
{
    co_tls_client_t* tls =
        (co_tls_client_t*)tcp_client->sock.tls;

    if (tls != NULL)
    {
        return &tls->callbacks;
    }

    return NULL;
}
