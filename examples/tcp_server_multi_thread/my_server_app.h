#pragma once

#include <coldforce.h>

#include "my_client_thread.h"

#define THREAD_COUNT 10
#define MAX_CLIENTS_PER_THREAD 200

// my server app object
typedef struct
{
    co_app_t base_app;

    // data
    co_tcp_server_t* server;
    my_client_thread client_thread[THREAD_COUNT];
    size_t thread_index;

} my_server_app;

int my_server_run(int argc, char** argv);
