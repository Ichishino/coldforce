#pragma once

#include "test.h"
#include "test_http_server_thread.h"

void
add_ws_server_connection(
    http_server_thread* self,
    co_http_client_t* http_client
);
