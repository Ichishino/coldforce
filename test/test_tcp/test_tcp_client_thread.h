#pragma once

#include "test.h"

#define TCP_CLIENT_DATA_SIZE    8192

typedef struct
{
    co_tcp_client_t* client;

    co_timer_t* retry_connect_timer;
    co_timer_t* send_timer;

    size_t data_index;

    char send_data[TCP_CLIENT_DATA_SIZE];

} test_client;

typedef struct
{
    co_thread_t thread;

    const char* server_address;
    uint16_t server_port;

    uint16_t client_count;
    test_client* clients;

} tcp_client_thread;

bool
tcp_client_thread_start(
    tcp_client_thread* thread
);
