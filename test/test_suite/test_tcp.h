#pragma once

#include <coldforce.h>

#include "test_tcp_server_thread.h"

#define TEST_EVENT_TCP_SERVER_REQ_CLOSE  1
#define TEST_EVENT_TCP_SERVER_RES_CLOSE  2

typedef struct
{
    uint32_t id;
    uint32_t size;

} test_tcp_packet_header_st;

#define TEST_TCP_PACKET_HEADER_SIZE    sizeof(test_tcp_packet_header_st)

typedef struct
{
    co_tcp_client_t* tcp_client;
    co_byte_array_t* send_data;
    co_byte_array_t* receive_data;
    co_timer_t* send_timer;
    size_t send_index;
    size_t send_count;
    size_t send_async_count;
    size_t send_async_comp_count;

} test_tcp_client_st;

typedef struct
{
    co_thread_t base;

    co_net_addr_family_t family;
    const char* server_address;
    uint16_t server_port;
    size_t data_size;
    size_t client_count;

    co_list_t* test_tcp_clients;
    test_tcp_server_thread_st test_tcp_server_thread;

} test_tcp_thread_st;

void test_tcp_run(test_tcp_thread_st* thread);
