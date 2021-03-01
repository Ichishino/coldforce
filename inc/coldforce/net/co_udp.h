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

typedef void(*co_udp_send_fn)(
    void* self, struct co_udp_t* udp, bool result);

typedef void (*co_udp_receive_fn)(
    void* self, struct co_udp_t* udp);

typedef struct
{
    co_net_addr_t remote_net_addr;

    co_buffer_st buffer;

} co_udp_send_data_t;

typedef struct co_udp_t
{
    co_socket_t sock;

    bool bound_local_net_addr;
    uint32_t sock_event_flags;

    co_udp_send_fn on_send_complete;
    co_udp_receive_fn on_receive_ready;

#ifdef CO_OS_WIN
    co_win_udp_extension_t win;
#else
    co_queue_t* send_queue;
#endif

} co_udp_t;

void co_udp_on_send_ready(co_udp_t* udp);
void co_udp_on_send_complete(co_udp_t* udp, size_t data_size);
void co_udp_on_receive_ready(co_udp_t* udp, size_t data_size);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API co_udp_t* co_udp_create(const co_net_addr_t* local_net_addr);

CO_NET_API void co_udp_destroy(co_udp_t* udp);
CO_NET_API void co_udp_close(co_udp_t* udp);

CO_NET_API bool co_udp_send(co_udp_t* udp,
    const co_net_addr_t* remote_net_addr, const void* data, size_t data_size);
CO_NET_API bool co_udp_send_async(co_udp_t* udp,
    const co_net_addr_t* remote_net_addr, const void* data, size_t data_size);

CO_NET_API bool co_udp_receive_start(co_udp_t* udp, co_udp_receive_fn handler);

CO_NET_API ssize_t co_udp_receive(co_udp_t* udp,
    co_net_addr_t* remote_net_addr, void* buffer, size_t buffer_size);

CO_NET_API bool co_udp_bind_local_net_addr(co_udp_t* udp);

CO_NET_API void co_udp_set_send_complete_handler(
    co_udp_t* udp, co_udp_send_fn handler);

CO_NET_API co_socket_t* co_udp_get_socket(co_udp_t* udp);

CO_NET_API void co_udp_set_data(co_udp_t* udp, uintptr_t data);
CO_NET_API uintptr_t co_udp_get_data(const co_udp_t* udp);

#ifdef CO_OS_WIN
CO_NET_API size_t co_win_udp_get_receive_data_size(const co_udp_t* udp);
CO_NET_API void co_win_udp_set_receive_buffer_size(co_udp_t* udp, size_t new_size);
CO_NET_API size_t co_win_udp_get_receive_buffer_size(const co_udp_t* udp);
CO_NET_API void* co_win_udp_get_receive_buffer(co_udp_t* udp);
CO_NET_API void co_win_udp_clear_receive_buffer(co_udp_t* udp);
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_UDP_H_INCLUDED
