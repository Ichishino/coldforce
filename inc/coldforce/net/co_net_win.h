#ifndef CO_NET_WIN_H_INCLUDED
#define CO_NET_WIN_H_INCLUDED

#include <coldforce/core/co_list.h>

#include <coldforce/net/co_net.h>

#ifdef CO_OS_WIN

#include <coldforce/net/co_net_selector_win.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// network (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    co_list_t* io_ctxs;

    struct co_win_net_receive_ctx_t
    {
        co_win_net_io_ctx_t* io_ctx;

        WSABUF buffer;
        size_t size;
        size_t index;

        size_t new_size;
        co_net_addr_t* remote_net_addr;

    } receive;

} co_win_net_extension_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

bool
co_win_net_setup(
    void
);

void
co_win_net_cleanup(
    void
);

bool
co_win_net_extension_setup(
    co_socket_t* sock,
    size_t receive_buffer_size,
    co_win_net_extension_t* win
);

void
co_win_net_extension_cleanup(
    co_win_net_extension_t* win
);

bool
co_win_net_send(
    co_socket_t* sock,
    co_win_net_extension_t* win,
    const void* data,
    size_t data_size
);

ssize_t
co_win_net_send_async(
    co_socket_t* sock,
    co_win_net_extension_t* win,
    const void* data,
    size_t data_size
);

bool
co_win_net_receive_start(
    co_socket_t* sock,
    co_win_net_extension_t* win
);

ssize_t
co_win_net_receive(
    co_socket_t* sock,
    co_win_net_extension_t* win,
    void* buffer,
    size_t buffer_size
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_WIN

#endif // CO_NET_H_INCLUDED
