#pragma once

#include "test_std.h"
#include "test_udp.h"
#include "test_udp2_server.h"

#ifndef CO_OS_WIN

#define TEST_EVENT_UDP2_SERVER_REQ_CLOSE  1
#define TEST_EVENT_UDP2_SERVER_RES_CLOSE  2

typedef struct
{
    co_thread_t base;

    co_net_addr_family_t family;
    const char* server_address;
    uint16_t server_port;

    size_t data_size;
    size_t client_count;
    co_net_addr_t remote_net_addr;

    co_list_t* test_udp_clients;
    test_udp2_server_thread_st test_udp2_server_thread;

} test_udp2_thread_st;

void test_udp2_run(test_udp2_thread_st* thread);

#endif // !CO_OS_WIN
