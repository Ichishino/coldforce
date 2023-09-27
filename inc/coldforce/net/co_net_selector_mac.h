#ifndef CO_NET_SELECTOR_MAC_H_INCLUDED
#define CO_NET_SELECTOR_MAC_H_INCLUDED

#include <coldforce/net/co_net.h>

#ifdef CO_OS_MAC

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net selector (mac)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_SOCKET_EVENT_RECEIVE     0x01
#define CO_SOCKET_EVENT_SEND        0x02
#define CO_SOCKET_EVENT_ACCEPT      0x04
#define CO_SOCKET_EVENT_CONNECT     0x08
#define CO_SOCKET_EVENT_CANCEL      0x10
#define CO_SOCKET_EVENT_CLOSE       0x00

typedef struct
{
    int kqueue_fd;
    int cancel_fds[2];

    size_t sock_count;

} co_net_selector_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_MAC

#endif // CO_NET_SELECTOR_MAC_H_INCLUDED
