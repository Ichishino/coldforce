#ifndef CO_SOCKET_H_INCLUDED
#define CO_SOCKET_H_INCLUDED

#include <coldforce/core/co_thread.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket_handle.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// socket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    co_socket_handle_t handle;
    co_thread_t* owner_thread;
    co_net_addr_t local_net_addr;
    bool open_local;

} co_socket_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API co_socket_handle_t co_socket_get_handle(const co_socket_t* sock);
CO_NET_API co_thread_t* co_socket_get_owner_thread(const co_socket_t* sock);
CO_NET_API const co_net_addr_t* co_socket_get_local_net_addr(const co_socket_t* sock);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SOCKET_H_INCLUDED
