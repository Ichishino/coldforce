#ifndef CO_NET_SELECTOR_WIN_H_INCLUDED
#define CO_NET_SELECTOR_WIN_H_INCLUDED

#include <coldforce/core/co_array.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_socket.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net selector (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

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

    bool valid;
    co_win_net_io_id_t id;
    co_socket_t* sock;

} co_win_net_io_ctx_t;

typedef struct
{
    void* iocp;
    co_array_t* items;

} co_net_selector_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void co_win_free_net_io_ctx(co_win_net_io_ctx_t* io_ctx);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_SELECTOR_WIN_H_INCLUDED
