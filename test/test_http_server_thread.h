#include "test.h"

typedef struct
{
    co_thread_t thread;

    uint16_t port;
    const char* certificate_file;
    const char* private_key_file;

    co_tcp_server_t* server;
    
    co_list_t* http_clients;
    co_list_t* http2_clients;
    co_list_t* ws_clients;

    co_timer_t* report_timer;

} http_server_thread;

bool
http_server_thread_start(
    http_server_thread* thread
);
