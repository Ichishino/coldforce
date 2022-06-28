#ifndef CO_NET_SELECTOR_H_INCLUDED
#define CO_NET_SELECTOR_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_net_selector_win.h>
#elif defined(CO_OS_LINUX)
#include <coldforce/net/co_net_selector_linux.h>
#elif defined(CO_OS_MAC)
#include <coldforce/net/co_net_selector_mac.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net selector
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

co_net_selector_t*
co_net_selector_create(
    void
);

void
co_net_selector_destroy(
    co_net_selector_t* net_selector
);

bool
co_net_selector_register(
    co_net_selector_t* net_selector,
    co_socket_t* sock,
    uint32_t flags
);

void
co_net_selector_unregister(
    co_net_selector_t* net_selector,
    co_socket_t* sock
);

bool
co_net_selector_update(
    co_net_selector_t* net_selector,
    co_socket_t* sock,
    uint32_t flags
);

co_wait_result_t
co_net_selector_wait(
    co_net_selector_t* net_worker,
    uint32_t msec
);

void
co_net_selector_wake_up(
    co_net_selector_t* net_worker
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_SELECTOR_H_INCLUDED
