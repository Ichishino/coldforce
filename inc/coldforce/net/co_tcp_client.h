#ifndef CO_TCP_CLIENT_H_INCLUDED
#define CO_TCP_CLIENT_H_INCLUDED

#include <coldforce/core/co_timer.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_tcp_win.h>
#else
#include <coldforce/core/co_queue.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_tcp_client_t;

typedef void(*co_tcp_close_fn)(
    co_thread_t* self, struct co_tcp_client_t* client);

typedef void(*co_tcp_send_fn)(
    co_thread_t* self, struct co_tcp_client_t* client, bool result);

typedef void(*co_tcp_receive_fn)(
    co_thread_t* self, struct co_tcp_client_t* client);

typedef void(*co_tcp_connect_fn)(
    co_thread_t* self, struct co_tcp_client_t* client, int error_code);

typedef struct
{
    co_tcp_connect_fn on_connect;
    co_tcp_send_fn on_send_async;
    co_tcp_receive_fn on_receive;
    co_tcp_close_fn on_close;

} co_tcp_callbacks_st;

typedef struct co_tcp_client_t
{
    co_socket_t sock;

    co_tcp_callbacks_st callbacks;

    co_net_addr_t remote_net_addr;
    bool open_remote;
    bool destroy_later;

    co_timer_t* close_timer;

#ifdef CO_OS_WIN
    co_win_tcp_client_extension_t win;
#else
    co_queue_t* send_queue;
#endif

} co_tcp_client_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

co_tcp_client_t*
co_tcp_client_create_with(
    co_socket_handle_t handle,
    const co_net_addr_t* remote_net_addr
);

bool
co_tcp_client_setup(
    co_tcp_client_t* client
);

void
co_tcp_client_cleanup(
    co_tcp_client_t* client
);

void
co_tcp_client_on_connect_complete(
    co_tcp_client_t* client,
    int error_code
);

void
co_tcp_client_on_send_ready(
    co_tcp_client_t* client
);

void
co_tcp_client_on_send_complete(
    co_tcp_client_t* client,
    size_t data_size
);

void
co_tcp_client_on_receive_ready(
    co_tcp_client_t* client,
    size_t data_size
);

void
co_tcp_client_on_close(
    co_tcp_client_t* client
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
co_tcp_client_t*
co_tcp_client_create(
    const co_net_addr_t* local_net_addr
);

CO_NET_API
void
co_tcp_client_destroy(
    co_tcp_client_t* client
);

CO_NET_API
co_tcp_callbacks_st*
co_tcp_get_callbacks(
    co_tcp_client_t* client
);

CO_NET_API
void
co_tcp_close(
    co_tcp_client_t* client
);

CO_NET_API
bool
co_tcp_connect(
    co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr
);

CO_NET_API
bool
co_tcp_send(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
);

CO_NET_API
bool
co_tcp_send_async(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
);

CO_NET_API
ssize_t
co_tcp_receive(
    co_tcp_client_t* client,
    void* buffer,
    size_t buffer_size
);

CO_NET_API
ssize_t
co_tcp_receive_all(
    co_tcp_client_t* client,
    co_byte_array_t* byte_array
);

CO_NET_API
bool
co_tcp_is_open(
    const co_tcp_client_t* client
);

CO_NET_API
const co_net_addr_t*
co_tcp_get_remote_net_addr(
    const co_tcp_client_t* client
);

CO_NET_API
co_socket_t*
co_tcp_client_get_socket(
    co_tcp_client_t* client
);

CO_NET_API
void
co_tcp_set_user_data(
    co_tcp_client_t* client,
    void* user_data
);

CO_NET_API
void*
co_tcp_get_user_data(
    const co_tcp_client_t* client
);

#ifdef CO_OS_WIN

CO_NET_API
size_t
co_win_tcp_get_receive_data_size(
    const co_tcp_client_t* client
);

CO_NET_API
void
co_win_tcp_set_receive_buffer_size(
    co_tcp_client_t* client,
    size_t size
);

CO_NET_API
size_t
co_win_tcp_get_receive_buffer_size(
    const co_tcp_client_t* client
);

CO_NET_API
void*
co_win_tcp_get_receive_buffer(
    co_tcp_client_t* client
);

CO_NET_API
void
co_win_tcp_clear_receive_buffer(
    co_tcp_client_t* client
);

#endif // CO_OS_WIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TCP_CLIENT_H_INCLUDED
