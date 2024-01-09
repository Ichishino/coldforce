#pragma once

#include "test_std.h"
#include "test_tcp_server_comm.h"
#include "test_tcp_server.h"

typedef test_tcp_server_thread_st test_tcp_mt_client_thread_st;

test_tcp_mt_client_thread_st*
test_tcp_mt_client_thread_create(
    void
);

void
test_tcp_mt_client_thread_destroy(
    test_tcp_mt_client_thread_st* thread
);
