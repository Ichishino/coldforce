#pragma once

#include <coldforce.h>

// my client thread object
typedef struct
{
    co_thread_t base_thread;

    // data
    co_list_t* client_list;
    size_t reserve;
    co_mutex_t* mutex;

} my_client_thread;

void init_my_client_thread(my_client_thread* thead);
bool add_client(my_client_thread* thead);
