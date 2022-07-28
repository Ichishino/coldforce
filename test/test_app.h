#pragma once

#include "test.h"
#include "test_tcp_server_thread.h"
#include "test_tcp_client_thread.h"
#include "test_http_server_thread.h"

typedef struct
{
    co_app_t app;

    tcp_server_thread tcp_server;
    tcp_client_thread tcp_client;

    http_server_thread http_server;

} test_app;

int test_app_run(int argc, char** argv);
