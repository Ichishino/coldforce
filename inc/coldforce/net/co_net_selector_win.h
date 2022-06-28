#ifndef CO_NET_SELECTOR_WIN_H_INCLUDED
#define CO_NET_SELECTOR_WIN_H_INCLUDED

#include <coldforce/core/co_list.h>
#include <coldforce/core/co_array.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net selector (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_SOCKET_EVENT_RECEIVE     0x01
#define CO_SOCKET_EVENT_SEND        0x02
#define CO_SOCKET_EVENT_ACCEPT      0x04
#define CO_SOCKET_EVENT_CONNECT     0x08
#define CO_SOCKET_EVENT_CLOSE       0x10

typedef enum
{
    CO_WIN_NET_IO_ID_TCP_ACCEPT = 1,
    CO_WIN_NET_IO_ID_TCP_CONNECT,
    CO_WIN_NET_IO_ID_TCP_SEND,
    CO_WIN_NET_IO_ID_TCP_RECEIVE,
    CO_WIN_NET_IO_ID_TCP_CLOSE,

    CO_WIN_NET_IO_ID_UDP_SEND,
    CO_WIN_NET_IO_ID_UDP_RECEIVE

} co_win_net_io_id_t;

typedef struct
{
    WSAOVERLAPPED ol;

    co_win_net_io_id_t id;
    co_socket_t* sock;

} co_win_net_io_ctx_t;

typedef struct co_net_selector_t
{
    void* iocp;

    size_t sock_count;
    co_array_t* ol_entries;

    co_list_t* io_ctx_trash;

} co_net_selector_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_win_free_io_ctx(
    co_win_net_io_ctx_t* io_ctx
);

void
co_win_try_clear_io_ctx_trash(
    co_net_selector_t* net_selector
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_WIN

#endif // CO_NET_SELECTOR_WIN_H_INCLUDED
