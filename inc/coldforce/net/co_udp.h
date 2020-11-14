#ifndef CO_UDP_H_INCLUDED
#define CO_UDP_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_udp_win.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// udp
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_udp_t;

typedef void (*co_udp_receive_fn)(
    void* self, struct co_udp_t* udp);

typedef struct co_udp_t
{
    co_socket_t sock;

    co_udp_receive_fn on_receive;

    bool bound_local_net_addr;

#ifdef CO_OS_WIN
    co_win_udp_extension_t win;
#endif

} co_udp_t;

void co_udp_on_send(co_udp_t* udp, size_t data_length);
void co_udp_on_receive(co_udp_t* udp, size_t data_length);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API co_udp_t* co_udp_create(const co_net_addr_t* local_net_addr);

CO_NET_API void co_udp_destroy(co_udp_t* udp);
CO_NET_API void co_udp_close(co_udp_t* udp);

CO_NET_API void co_udp_send(co_udp_t* udp,
    const co_net_addr_t* remote_net_addr, const void* data, size_t data_length);
CO_NET_API void co_udp_send_string(co_udp_t* udp,
    const co_net_addr_t* remote_net_addr, const char* data);

CO_NET_API bool co_udp_receive_start(co_udp_t* udp, co_udp_receive_fn handler);

CO_NET_API ssize_t co_udp_receive(co_udp_t* udp,
    co_net_addr_t* remote_net_addr, void* buffer, size_t buffer_length);

CO_NET_API bool co_udp_bind_local_net_addr(co_udp_t* udp);

#ifdef CO_OS_WIN
CO_NET_API size_t co_win_udp_get_received_data_length(const co_udp_t* udp);
CO_NET_API void co_win_udp_set_receive_buffer_length(co_udp_t* udp, size_t new_length);
CO_NET_API size_t co_win_udp_get_receive_buffer_length(const co_udp_t* udp);
CO_NET_API void* co_win_udp_get_receive_buffer(co_udp_t* udp);
CO_NET_API void co_win_udp_clear_receive_buffer(co_udp_t* udp);
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_UDP_H_INCLUDED