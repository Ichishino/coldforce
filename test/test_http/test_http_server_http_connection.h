#pragma once

#include "test.h"
#include "test_http_server_thread.h"

void
add_http_server_connection(
    http_server_thread* self,
    co_tcp_client_t* tcp_client
);
