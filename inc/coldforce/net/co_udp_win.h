#ifndef CO_UDP_WIN_H_INCLUDED
#define CO_UDP_WIN_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_socket_handle.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN

#include <coldforce/net/co_net_win.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// udp (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WIN_UDP_DEFAULT_RECEIVE_BUFFER_SIZE    65535

struct co_udp_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

co_socket_handle_t
co_win_udp_socket_create(
    co_net_addr_family_t family
);

bool
co_win_udp_setup(
    struct co_udp_t* udp,
    size_t receive_buffer_size
);

bool
co_win_udp_send_to(
    struct co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
);

bool
co_win_udp_send_to_async(
    struct co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
);

bool
co_win_udp_receive_from_start(
    struct co_udp_t* udp
);

bool
co_win_udp_receive_from(
    struct co_udp_t* udp,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_size,
    size_t* data_size
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_WIN

#endif // CO_UDP_WIN_H_INCLUDED
