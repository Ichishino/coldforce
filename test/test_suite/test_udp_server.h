#pragma once

#include "test_std.h"

typedef struct
{
    uint32_t seq;
    uint32_t size;
    uint32_t index;
    uint32_t close;
    uint32_t port;

} test_udp_packet_header_st;

#define TEST_UDP_PACKET_HEADER_SIZE    sizeof(test_udp_packet_header_st)

typedef struct
{
    test_udp_packet_header_st* data;
    size_t size;

} test_udp_data_block_st;

typedef struct
{
    co_thread_t base;

    uint16_t port;
    co_net_addr_family_t family;
    co_udp_t* udp_server;

} test_udp_server_thread_st;

void test_udp_server_thread_start(test_udp_server_thread_st* test_udp_server_thread);
void test_udp_server_thread_stop(test_udp_server_thread_st* test_udp_server_thread);
