#pragma once

#include "test_std.h"
#include "test_udp_server.h"

typedef struct
{
    test_thread_st thread;
    co_udp_server_t* udp_server;
    bool close;

} test_udp2_server_thread_st;

void
test_udp2_server_thread_start(
    test_udp2_server_thread_st* test_udp2_server_thread
);

void
test_udp2_server_thread_stop(
    test_udp2_server_thread_st* test_udp2_server_thread
);
