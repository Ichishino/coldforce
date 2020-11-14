#ifndef CO_NET_SELECTOR_H_INCLUDED
#define CO_NET_SELECTOR_H_INCLUDED

#include <coldforce/net/co_net.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_net_selector_win.h>
#else
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net selector
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_net_selector_t* co_net_selector_create(void);
void co_net_selector_destroy(co_net_selector_t* net_selector);

bool co_net_selector_register(
    co_net_selector_t* net_selector, co_socket_t* sock);
void co_net_selector_unregister(
    co_net_selector_t* net_selector, co_socket_t* sock);

co_wait_result_t co_net_selector_wait(
    co_net_selector_t* net_worker, uint32_t msec);

void co_net_selector_wake_up(co_net_selector_t* net_worker);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_SELECTOR_H_INCLUDED
