#ifndef CO_UDP_H_INCLUDED
#define CO_UDP_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_udp_win.h>
#else
#include <coldforce/core/co_queue.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// udp
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_udp_t;

typedef void(*co_udp_send_async_fn)(
    co_thread_t* self, struct co_udp_t* udp, void* user_data, bool result);

typedef void (*co_udp_receive_fn)(
    co_thread_t* self, struct co_udp_t* udp);

typedef struct
{
    co_net_addr_t remote_net_addr;

    const void* data;
    size_t data_size;
    void* user_data;

} co_udp_send_async_data_t;

typedef struct
{
    co_udp_send_async_fn on_send_async;
    co_udp_receive_fn on_receive;

} co_udp_callbacks_st;

typedef struct co_udp_t
{
    co_socket_t sock;

    co_udp_callbacks_st callbacks;

    bool bound_local_net_addr;
    uint32_t sock_event_flags;
    co_queue_t* send_async_queue;

#ifdef CO_OS_WIN
    co_win_net_extension_t win;
#endif

} co_udp_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#ifndef CO_OS_WIN
void
co_udp_on_send_async_ready(
    co_udp_t* udp
);
#endif

void
co_udp_on_send_async_complete(
    co_udp_t* udp,
    bool result
);

void
co_udp_on_receive_ready(
    co_udp_t* udp,
    size_t data_size
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
co_udp_t*
co_udp_create(
    const co_net_addr_t* local_net_addr
);

CO_NET_API
void
co_udp_destroy(
    co_udp_t* udp
);

CO_NET_API
co_udp_callbacks_st*
co_udp_get_callbacks(
    co_udp_t* udp
);

CO_NET_API
void
co_udp_close(
    co_udp_t* udp
);

CO_NET_API
bool
co_udp_send_to(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
);

CO_NET_API
bool
co_udp_send_to_async(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size,
    void* user_data
);

CO_NET_API
bool
co_udp_receive_from_start(
    co_udp_t* udp
);

CO_NET_API
ssize_t
co_udp_receive_from(
    co_udp_t* udp,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_size
);

CO_NET_API
bool
co_udp_bind_local_net_addr(
    co_udp_t* udp
);

CO_NET_API
co_socket_t*
co_udp_get_socket(
    co_udp_t* udp
);

CO_NET_API
void
co_udp_set_user_data(
    co_udp_t* udp,
    void* user_data
);

CO_NET_API
void*
co_udp_get_user_data(
    const co_udp_t* udp
);

#ifdef CO_OS_WIN

CO_NET_API
size_t
co_win_udp_get_receive_data_size(
    const co_udp_t* udp
);

CO_NET_API
void
co_win_udp_set_receive_buffer_size(
    co_udp_t* udp,
    size_t new_size
);

CO_NET_API
size_t
co_win_udp_get_receive_buffer_size(
    const co_udp_t* udp
);

CO_NET_API
void*
co_win_udp_get_receive_buffer(
    co_udp_t* udp
);

CO_NET_API
void
co_win_udp_clear_receive_buffer(
    co_udp_t* udp
);

#endif // CO_OS_WIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_UDP_H_INCLUDED
