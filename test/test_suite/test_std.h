#pragma once

#include <coldforce.h>
#include <coldforce/core/co_time.h>
#include <coldforce/net/co_byte_order.h>

#include <assert.h>

struct test_thread_st;

typedef void (*test_run_fn)(struct co_thread_t*);

typedef struct
{
    uint32_t time_limit_sec;
    co_net_addr_family_t family;
    const char* server_address;
    uint16_t server_port;
    size_t data_size;
    size_t client_count;

    const char* title;
    test_run_fn run;

} test_ctx_st;

typedef struct
{
    co_thread_t base;
    test_ctx_st ctx;
    co_list_t* clients;

} test_thread_st;

typedef struct
{
    co_byte_array_t* send_data;
    co_byte_array_t* receive_data;
    co_timer_t* send_timer;
    size_t send_index;
    size_t send_count;
    size_t send_async_count;
    size_t send_async_comp_count;
    size_t total_send_count;
    size_t receive_count;

} test_data_st;

static inline void
test_data_destroy(
    test_data_st* data
)
{
    co_byte_array_destroy(data->send_data);
    co_byte_array_destroy(data->receive_data);
    co_timer_destroy(data->send_timer);

    data->send_data = NULL;
    data->receive_data = NULL;
    data->send_timer = NULL;
}
