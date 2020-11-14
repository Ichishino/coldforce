#ifndef CO_TCP_CLIENT_H_INCLUDED
#define CO_TCP_CLIENT_H_INCLUDED

#include <coldforce/core/co_timer.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_tcp_win.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_tcp_client_t;

typedef void(*co_tcp_close_fn)(
    void* self, struct co_tcp_client_t* client);

typedef void(*co_tcp_send_fn)(
    void* self, struct co_tcp_client_t* client, bool result);

typedef void(*co_tcp_receive_fn)(
    void* self, struct co_tcp_client_t* client);

typedef void(*co_tcp_connect_fn)(
    void* self, struct co_tcp_client_t* client, int error_code);

typedef struct co_tcp_client_t
{
    co_socket_t sock;

    co_net_addr_t remote_net_addr;
    bool open_remote;

    co_tcp_send_fn on_send;
    co_tcp_receive_fn on_receive;
    co_tcp_close_fn on_close;
    co_tcp_connect_fn on_connect;

    co_timer_t* close_timer;

#ifdef CO_OS_WIN
    co_win_tcp_client_extension_t win;
#endif

} co_tcp_client_t;

bool co_tcp_client_setup(co_tcp_client_t* client);
void co_tcp_client_cleanup(co_tcp_client_t* client);
void co_tcp_client_on_send(co_tcp_client_t* client, size_t data_length);
void co_tcp_client_on_receive(co_tcp_client_t* client, size_t data_length);
void co_tcp_client_on_close(co_tcp_client_t* client);
void co_tcp_client_on_connect(co_tcp_client_t* client, int error_code);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API co_tcp_client_t* co_tcp_client_create(
    const co_net_addr_t* remote_net_addr, const co_net_addr_t* local_net_addr);

CO_NET_API void co_tcp_client_destroy(co_tcp_client_t* client);
CO_NET_API void co_tcp_client_close(co_tcp_client_t* client);

CO_NET_API int co_tcp_connect(co_tcp_client_t* client);
CO_NET_API bool co_tcp_connect_async(
    co_tcp_client_t* client, co_tcp_connect_fn handler);

CO_NET_API bool co_tcp_send(
    co_tcp_client_t* client, const void* data, size_t data_length);
CO_NET_API bool co_tcp_send_string(
    co_tcp_client_t* client, const char* data);
CO_NET_API bool co_tcp_send_async(
    co_tcp_client_t* client, const void* data, size_t data_length);

CO_NET_API ssize_t co_tcp_receive(
    co_tcp_client_t* client, void* buffer, size_t buffer_length);

CO_NET_API bool co_tcp_is_open(const co_tcp_client_t* client);

CO_NET_API void co_tcp_set_send_handler(
    co_tcp_client_t* client, co_tcp_send_fn handler);
CO_NET_API void co_tcp_set_receive_handler(
    co_tcp_client_t* client, co_tcp_receive_fn handler);
CO_NET_API void co_tcp_set_close_handler(
    co_tcp_client_t* client, co_tcp_close_fn handler);

CO_NET_API const co_net_addr_t*
    co_tcp_get_remote_net_addr(const co_tcp_client_t* client);

#ifdef CO_OS_WIN
CO_NET_API size_t co_win_tcp_get_received_data_length(const co_tcp_client_t* client);
CO_NET_API void co_win_tcp_set_receive_buffer_length(co_tcp_client_t* client, size_t new_length);
CO_NET_API size_t co_win_tcp_get_receive_buffer_length(const co_tcp_client_t* client);
CO_NET_API void* co_win_tcp_get_receive_buffer(co_tcp_client_t* client);
CO_NET_API void co_win_tcp_clear_receive_buffer(co_tcp_client_t* client);
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TCP_CLIENT_H_INCLUDED
