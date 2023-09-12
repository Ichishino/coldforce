#pragma once

#include <coldforce.h>
    
#include "test_tcp.h"
#include "test_udp.h"

#define TEST_LOG_CATEGORY   1
#define test_info(format, ...) \
    co_log_write(CO_LOG_LEVEL_INFO, TEST_LOG_CATEGORY, format, ##__VA_ARGS__)
#define test_error(format, ...) \
    co_log_write(CO_LOG_LEVEL_ERROR, TEST_LOG_CATEGORY, format, ##__VA_ARGS__)

typedef struct
{
    co_app_t base;

    test_tcp_thread_t test_tcp_thread;
    test_udp_thread_t test_udp_thread;

} test_app_t;

int test_app_run(int argc, char** argv);
