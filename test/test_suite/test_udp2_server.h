#pragma once

#include "test_std.h"
#include "test_udp_server.h"

#ifndef CO_OS_WIN

typedef struct
{
    co_thread_t base;

    bool close;
    const char* address;
    uint16_t port;
    co_net_addr_family_t family;
    co_udp_t* udp_server;
    co_list_t* udp_clients;

} test_udp2_server_thread_st;

void test_udp2_server_thread_start(test_udp2_server_thread_st* test_udp2_server_thread);
void test_udp2_server_thread_stop(test_udp2_server_thread_st* test_udp2_server_thread);

#endif // !CO_OS_WIN
