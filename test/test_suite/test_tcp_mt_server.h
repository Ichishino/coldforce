#pragma once

#include "test_std.h"
#include "test_tcp_server_comm.h"

typedef struct
{
    test_thread_st thread;

    co_tcp_server_t* tcp_server;
    bool close;

    size_t thread_count;
    size_t thread_client_count;
    co_list_iterator_t* thread_it;

    size_t stopped_thread_count;

} test_tcp_mt_server_thread_st;

void
test_tcp_mt_server_thread_start(
    test_tcp_mt_server_thread_st* thread
);

void
test_tcp_mt_server_thread_stop(
    test_tcp_mt_server_thread_st* thread
);
