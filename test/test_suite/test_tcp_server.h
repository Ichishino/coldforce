#pragma once

#include "test_std.h"

typedef struct test_tcp_server_thread_st
{
    test_thread_st thread;

    co_tcp_server_t* tcp_server;
    bool close;

} test_tcp_server_thread_st;

void
test_tcp_server_thread_start(
    test_tcp_server_thread_st* test_tcp_server_thread
);

void
test_tcp_server_thread_stop(
    test_tcp_server_thread_st* test_tcp_server_thread
);
