#pragma once

#include "test.h"
#include "test_http_server_thread.h"
#include "test_ws_http2_client_thread.h"

typedef struct
{
    co_app_t app;

    http_server_thread http_server;
    ws_http2_client_thread ws_http2_client;

} test_app;

int test_app_run(int argc, char** argv);
