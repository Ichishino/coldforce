#pragma once

#include <coldforce.h>

typedef struct
{
    co_thread_t base;

    uint16_t port;
    co_net_addr_family_t family;
    co_udp_t* udp_server;

} test_udp_server_thread_st;

void test_udp_server_thread_start(test_udp_server_thread_st* test_udp_server_thread);
void test_udp_server_thread_stop(test_udp_server_thread_st* test_udp_server_thread);
