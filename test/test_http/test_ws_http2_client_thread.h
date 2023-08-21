#pragma once

#include "test.h"

typedef struct
{
    co_thread_t thread;

    co_url_st* url;

    co_http2_client_t* http2_client;
    co_http2_stream_t* http2_stream1;
    co_http2_stream_t* http2_stream2;

} ws_http2_client_thread;

bool
ws_http2_client_thread_start(
    ws_http2_client_thread* thread
);
