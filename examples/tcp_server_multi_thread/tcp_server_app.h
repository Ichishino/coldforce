#pragma once

#include <coldforce.h>

#include "tcp_client_thread.h"

#define THREAD_COUNT 10
#define MAX_CLIENTS_PER_THREAD 200

//---------------------------------------------------------------------------//
// tcp server object
//---------------------------------------------------------------------------//

typedef struct
{
    co_app_t base_app;

    // app data
    co_tcp_server_t* tcp_server;
    tcp_client_thread_st tcp_client_thread[THREAD_COUNT];
    size_t thread_index;

} tcp_server_app_st;

int tcp_server_app_run(int argc, char** argv);
