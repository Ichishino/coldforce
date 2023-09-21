#pragma once

#include <coldforce.h>
    
#include "test_tcp.h"
#include "test_udp.h"

#define TEST_EVENT_ID_TEST_FINISHED    100

#define TEST_LOG_CATEGORY   1
#define test_info(format, ...) \
    co_log_write(CO_LOG_LEVEL_INFO, TEST_LOG_CATEGORY, format, ##__VA_ARGS__)
#define test_error(format, ...) \
    co_log_write(CO_LOG_LEVEL_ERROR, TEST_LOG_CATEGORY, format, ##__VA_ARGS__)

struct test_app_st;

typedef struct
{
    co_thread_t* thread;
    void (*func)(struct test_app_st*);
    uint32_t time_limit_sec;

} test_item_st;

typedef struct test_app_st
{
    co_app_t base;

    test_tcp_thread_st test_tcp_1_thread;
    test_tcp_thread_st test_tcp_2_thread;
    test_udp_thread_st test_udp_1_thread;
    test_udp_thread_st test_udp_2_thread;

    test_item_st item[32];
    size_t item_index;
    co_timer_t* timer;

    uint64_t start_time;
    uint64_t total_time;

} test_app_st;

int test_app_run(int argc, char** argv);
