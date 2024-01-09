#pragma once

#include "test_std.h"

struct test_tcp_server_thread_st;

void
test_tcp_server_on_accept(
    struct test_tcp_server_thread_st* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
);
