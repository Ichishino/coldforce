#ifndef CO_UDP_H_INCLUDED
#define CO_UDP_H_INCLUDED

#include <coldforce/core/co_queue.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket.h>

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

typedef void(*co_udp_receive_timer_fn)(
    co_thread_t* self, struct co_udp_t* udp);

typedef struct
{
    const co_net_addr_t* remote_net_addr;

    const void* data;
    size_t data_size;
    void* user_data;

} co_udp_send_async_data_t;

typedef struct
{
    co_udp_send_async_fn on_send_async;
    co_udp_receive_fn on_receive;
    co_udp_receive_timer_fn on_receive_timer;

} co_udp_callbacks_st;

typedef struct co_udp_t
{
    co_socket_t sock;

    co_udp_callbacks_st callbacks;

    bool is_bound;
    co_queue_t* send_async_queue;

#ifndef CO_OS_WIN
    uint32_t sock_event_flags;
#endif

} co_udp_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

bool
co_udp_setup(
    co_udp_t* udp,
    co_socket_type_t type
);

void
co_udp_cleanup(
    co_udp_t* udp
);

bool
co_udp_setup_new_handle(
    co_udp_t* udp,
    const co_net_addr_t* local_net_addr
);

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
co_udp_receive_start(
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
co_udp_bind(
    co_udp_t* udp
);

CO_NET_API
bool
co_udp_connect(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr
);

CO_NET_API
bool
co_udp_send(
    co_udp_t* udp_conn,
    const void* data,
    size_t data_size
);

CO_NET_API
bool
co_udp_send_async(
    co_udp_t* udp_conn,
    const void* data,
    size_t data_size,
    void* user_data
);

CO_NET_API
ssize_t
co_udp_receive(
    co_udp_t* udp_conn,
    void* buffer,
    size_t buffer_size
);

CO_NET_API
bool
co_udp_create_receive_timer(
    co_udp_t* udp,
    uint32_t msec
);

CO_NET_API
void
co_udp_destroy_receive_timer(
    co_udp_t* udp
);

CO_NET_API
bool
co_udp_start_receive_timer(
    co_udp_t* udp
);

CO_NET_API
void
co_udp_stop_receive_timer(
    co_udp_t* udp
);

CO_NET_API
bool
co_udp_restart_receive_timer(
    co_udp_t* udp
);

CO_NET_API
bool
co_udp_is_running_receive_timer(
    const co_udp_t* udp
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

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_UDP_H_INCLUDED
