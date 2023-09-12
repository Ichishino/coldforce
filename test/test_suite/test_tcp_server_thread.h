#pragma once

#include <coldforce.h>

typedef struct
{
    co_thread_t base;

    uint16_t port;
    co_net_addr_family_t family;
    co_tcp_server_t* tcp_server;
    co_list_t* tcp_clients;

} test_tcp_server_thread_t;

void test_tcp_server_thread_start(test_tcp_server_thread_t* test_tcp_server_thread);
void test_tcp_server_thread_stop(test_tcp_server_thread_t* test_tcp_server_thread);
