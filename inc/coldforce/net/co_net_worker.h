#ifndef CO_NET_WORKER_H_INCLUDED
#define CO_NET_WORKER_H_INCLUDED

#include <coldforce/core/co_list.h>
#include <coldforce/core/co_event_worker.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_tcp_server.h>
#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_udp.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net worker
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_net_selector_t;

typedef struct co_net_worker_t
{
    co_event_worker_t event_worker;

    co_net_selector_t* net_selector;

    co_list_t* tcp_servers;
    co_list_t* tcp_clients;
    co_list_t* udps;

    co_tcp_handover_fn on_tcp_handover;

#ifdef CO_DEBUG
    uint32_t sock_count;
#endif

} co_net_worker_t;

#ifdef CO_DEBUG
#define CO_DEBUG_SOCKET_COUNTER_INC() \
    (((co_net_worker_t*)co_thread_get_current()->event_worker)->sock_count++)
#define CO_DEBUG_SOCKET_COUNTER_DEC() \
    (((co_net_worker_t*)co_thread_get_current()->event_worker)->sock_count--)
#else
#define CO_DEBUG_SOCKET_COUNTER_INC()   ((void)0)
#define CO_DEBUG_SOCKET_COUNTER_DEC()   ((void)0)
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_net_worker_t* co_net_worker_create(void);
void co_net_worker_cleanup(co_net_worker_t* net_worker);

co_wait_result_t co_net_worker_wait(
    co_net_worker_t* net_worker, uint32_t msec);
void co_net_worker_wake_up(co_net_worker_t* net_worker);
bool co_net_worker_dispatch(co_net_worker_t* net_worker, co_event_t* event);
void co_net_worker_on_idle(co_net_worker_t* net_worker);

bool co_net_worker_register_tcp_server(
    co_net_worker_t* net_worker, co_tcp_server_t* server);
void co_net_worker_unregister_tcp_server(
    co_net_worker_t* net_worker, co_tcp_server_t* server);

bool co_net_worker_register_tcp_connector(
    co_net_worker_t* net_worker, co_tcp_client_t* client);
void co_net_worker_unregister_tcp_connector(
    co_net_worker_t* net_worker, co_tcp_client_t* client);

bool co_net_worker_register_tcp_connection(
    co_net_worker_t* net_worker, co_tcp_client_t* client);
void co_net_worker_unregister_tcp_connection(
    co_net_worker_t* net_worker, co_tcp_client_t* client);

bool co_net_worker_set_tcp_send(
    co_net_worker_t* net_worker, co_tcp_client_t* client, bool enable);

void co_net_worker_close_tcp_client_local(
    co_net_worker_t* net_worker, co_tcp_client_t* client);
bool co_net_worker_close_tcp_client_remote(
    co_net_worker_t* net_worker, co_tcp_client_t* client);

bool co_net_worker_register_udp(
    co_net_worker_t* net_worker, co_udp_t* udp);
void co_net_worker_unregister_udp(
    co_net_worker_t* net_worker, co_udp_t* udp);
bool co_net_worker_update_udp(
    co_net_worker_t* net_worker, co_udp_t* udp);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_WORKER_H_INCLUDED
