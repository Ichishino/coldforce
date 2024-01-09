#pragma once

#include "test_std.h"
#include "test_udp.h"
#include "test_udp2_server.h"

#define TEST_EVENT_UDP2_SERVER_REQ_CLOSE  1
#define TEST_EVENT_UDP2_SERVER_RES_CLOSE  2

typedef struct
{
    test_thread_st thread;

    co_net_addr_t remote_net_addr;
    test_udp2_server_thread_st server;

} test_udp2_thread_st;

void
test_udp2_run(
    co_thread_t* thread
);
