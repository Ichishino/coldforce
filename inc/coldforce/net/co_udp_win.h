#ifndef CO_UDP_WIN_H_INCLUDED
#define CO_UDP_WIN_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_socket_handle.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// udp (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WIN_UDP_DEFAULT_RECEIVE_BUFFER_LENGTH    65535

struct co_udp_t;

typedef struct
{
    co_net_addr_t local_net_addr;

    co_list_t* io_send_ctxs;

    struct co_udp_receive_ctx_t
    {
        co_win_net_io_ctx_t* io_ctx;

        WSABUF buffer;
        size_t length;
        size_t index;

        size_t new_length;

        co_net_addr_t remote_net_addr;

    } receive;

} co_win_udp_extension_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_socket_handle_t co_win_udp_socket_create(co_address_family_t family);

bool co_win_udp_setup(struct co_udp_t* udp, size_t receive_buffer_length);
void co_win_udp_cleanup(struct co_udp_t* udp);

bool co_win_udp_send(struct co_udp_t* udp,
    const co_net_addr_t* remote_net_addr, const void* data, size_t data_length);
bool co_win_udp_send_async(struct co_udp_t* udp,
    const co_net_addr_t* remote_net_addr, const void* data, size_t data_length);

bool co_win_udp_receive_start(struct co_udp_t* udp);

bool co_win_udp_receive(struct co_udp_t* udp,
    co_net_addr_t* remote_net_addr, void* buffer, size_t buffer_length,
    size_t* data_length);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_WIN

#endif // CO_UDP_WIN_H_INCLUDED
