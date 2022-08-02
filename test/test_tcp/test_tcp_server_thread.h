#pragma once

#include "test.h"

typedef struct
{
    co_thread_t thread;

    uint16_t port;
    co_tcp_server_t* server;
    co_list_t* clients;

    co_timer_t* report_timer;

} tcp_server_thread;

bool
tcp_server_thread_start(
    tcp_server_thread* thread
);
