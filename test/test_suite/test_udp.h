#pragma once

#include "test_std.h"
#include "test_udp_server.h"

typedef struct
{
    co_udp_t* udp_client;
    test_data_st data;

} test_udp_client_st;

typedef struct
{
    test_thread_st thread;

    co_net_addr_t remote_net_addr;
    test_udp_server_thread_st server;

} test_udp_thread_st;

void
test_udp_run(
    co_thread_t* thread
);
