#ifndef CO_NET_SELECTOR_LINUX_H_INCLUDED
#define CO_NET_SELECTOR_LINUX_H_INCLUDED

#include <coldforce/net/co_net.h>

#ifdef CO_OS_LINUX

#include <sys/epoll.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net selector (linux)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_SOCKET_EVENT_RECEIVE     EPOLLIN
#define CO_SOCKET_EVENT_SEND        EPOLLOUT
#define CO_SOCKET_EVENT_ACCEPT      EPOLLIN
#define CO_SOCKET_EVENT_CONNECT     EPOLLOUT
#define CO_SOCKET_EVENT_CLOSE       EPOLLRDHUP

typedef struct
{
    int e_fd;
    int cancel_e_fd;

    size_t sock_count;

} co_net_selector_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_LINUX

#endif // CO_NET_SELECTOR_LINUX_H_INCLUDED
