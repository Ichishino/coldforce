#ifndef CO_TLS_TCP_CLIENT_H_INCLUDED
#define CO_TLS_TCP_CLIENT_H_INCLUDED

#include <coldforce/core/co_queue.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls tcp
//---------------------------------------------------------------------------//

typedef void(*co_tls_tcp_handshake_fn)(
    void* self, co_tcp_client_t* client, int error_code);

typedef struct
{
    SSL* ssl;
    co_tls_ctx_st ctx;

    BIO* network_bio;

    co_tcp_connect_fn on_connect;
    co_tcp_receive_fn on_receive_ready;
    co_tls_tcp_handshake_fn on_handshake_complete;

    co_queue_t* receive_data_queue;

} co_tls_tcp_client_t;

#define co_tcp_client_get_tls(client) \
    ((co_tls_tcp_client_t*)client->sock.tls)

void co_tls_tcp_client_setup(co_tls_tcp_client_t* tls, co_tls_ctx_st* tls_ctx);
void co_tls_tcp_client_cleanup(co_tls_tcp_client_t* tls);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_TLS_API co_tcp_client_t* co_tls_tcp_client_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx);

CO_TLS_API void co_tls_tcp_client_destroy(co_tcp_client_t* client);
CO_TLS_API void co_tls_tcp_client_close(co_tcp_client_t* client);

CO_TLS_API bool co_tls_tcp_client_install(
    co_tcp_client_t* client, co_tls_ctx_st* tls_ctx);

CO_TLS_API void co_tls_tcp_client_set_host_name(
    co_tcp_client_t* client, const char* host_name);
CO_TLS_API void co_tls_tcp_client_set_alpn_protocols(
    co_tcp_client_t* client, const char* protocols[], size_t count);
CO_TLS_API bool co_tls_tcp_client_get_alpn_selected_protocol(
    co_tcp_client_t* client, char* buffer, size_t buffer_size);

CO_TLS_API bool co_tls_tcp_connect(co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr);
CO_TLS_API bool co_tls_tcp_connect_async(co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr, co_tcp_connect_fn handler);

CO_TLS_API bool co_tls_tcp_start_handshake(
    co_tcp_client_t* client, co_tls_tcp_handshake_fn handler);

CO_TLS_API bool co_tls_tcp_send(
    co_tcp_client_t* client, const void* data, size_t data_size);
CO_TLS_API bool co_tls_tcp_send_async(
    co_tcp_client_t* client, const void* data, size_t data_size);

CO_TLS_API ssize_t co_tls_tcp_receive(
    co_tcp_client_t* client, void* buffer, size_t buffer_size);
CO_TLS_API ssize_t co_tls_tcp_receive_all(
    co_tcp_client_t* client, co_byte_array_t* byte_array);

CO_TLS_API bool co_tls_tcp_is_open(const co_tcp_client_t* client);

CO_TLS_API void co_tls_tcp_set_send_complete_handler(
    co_tcp_client_t* client, co_tcp_send_fn handler);

CO_TLS_API void co_tls_tcp_set_receive_handler(
    co_tcp_client_t* client, co_tcp_receive_fn handler);

CO_TLS_API void co_tls_tcp_set_close_handler(
    co_tcp_client_t* client, co_tcp_close_fn handler);

CO_TLS_API const co_net_addr_t*
    co_tls_tcp_get_remote_net_addr(const co_tcp_client_t* client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_TCP_CLIENT_H_INCLUDED
