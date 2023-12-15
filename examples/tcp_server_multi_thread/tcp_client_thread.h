#pragma once

#include <coldforce.h>

#define app_get_remote_address(protocol, net_unit, buffer) \
    co_net_addr_to_string( \
        co_socket_get_remote_net_addr( \
            co_##protocol##_get_socket(net_unit)), \
        buffer, sizeof(buffer));

//---------------------------------------------------------------------------//
// tcp client object
//---------------------------------------------------------------------------//

typedef struct
{
    co_thread_t base_thread;

    // data
    co_list_t* tcp_clients;
    size_t reserve;
    co_mutex_t* mutex;

} tcp_client_thread_st;

void tcp_client_thread_start(tcp_client_thread_st* thead);
bool tcp_client_thread_add(tcp_client_thread_st* thead);
