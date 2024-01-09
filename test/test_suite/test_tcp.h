#pragma once

#include "test_std.h"
#include "test_tcp_server.h"

#define TEST_EVENT_TCP_SERVER_REQ_CLOSE  1
#define TEST_EVENT_TCP_SERVER_RES_CLOSE  2

typedef struct
{
    uint32_t id;
    uint32_t size;

} test_tcp_packet_header_st;

#define TEST_TCP_PACKET_HEADER_SIZE    \
    sizeof(test_tcp_packet_header_st)

typedef struct
{
    co_tcp_client_t* tcp_client;
    test_data_st data;

} test_tcp_client_st;

typedef struct
{
    test_thread_st thread;
    test_tcp_server_thread_st server;

} test_tcp_thread_st;

void
test_tcp_run(
    co_thread_t* thread
);
