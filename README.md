Coldforce
========
[![Build](https://github.com/Ichishino/coldforce/actions/workflows/build.yml/badge.svg)](https://github.com/Ichishino/coldforce/actions/workflows/build.yml)

Coldforce is a framework written in C that supports various network protocols.  
With the asynchronous API of this framework
You can easily create event-driven network applications.  
The currently supported protocols are as follows.
All of these support clients and servers (multi-client, C10K).
* TCP/UDP
* TLS
* HTTP/1.1
* HTTP/2
* WebSocket

### Platforms
* Windows
* Linux
* macOS

### Requirements
* C99 or later
* OpenSSL (only when using TLS, https, wss)
* -pthread -lm

### Modules
* co_core - Application core
* co_net - TCP,UDP
* co_tls - TLS
* co_http - HTTP/1.1
* co_http2 - HTTP/2
* co_ws - WebSocket

### Builds
* Windows  
Visual Studio ([prj/vs19/coldforce.sln](https://github.com/Ichishino/coldforce/tree/master/prj/vs19/coldforce))
* Linux  
cmake
```shellsession
$ cd build
$ cmake ..
$ make
```
* macOS  
cmake (same way as Linux)
XCode (coming soon)

### Code Examples
* simple http server http://127.0.0.1:8080
```C++
#include <coldforce.h>

// app object
typedef struct {
    co_app_t base_app;
    co_tcp_server_t* server;
} my_app;

// receive request
void on_my_http_request(my_app* self, co_http_client_t* client,
    const co_http_request_t* request, const co_http_response_t* unused, int error_code)
{
    if (error_code == 0)
    {
        co_http_response_t* response = co_http_response_create_with(200, "OK");
        co_http_header_t* response_header = co_http_response_get_header(response);

        co_http_header_add_field(response_header, "Content-Type", "text/html");

        const char* response_content =
            "<!DOCTYPE html><html><body>Hello</body></html>";
        uint32_t response_content_size = (uint32_t)strlen(response_content);

        co_http_header_set_content_length(response_header, response_content_size);

        // response
        co_http_send_response(client, response);
        co_http_send_data(client, response_content, response_content_size);
    }

    co_http_client_destroy(client);
}

// close
void on_my_http_close(my_app* self, co_http_client_t* client)
{
    co_http_client_destroy(client);
}

// accept
void on_my_tcp_accept(my_app* self, co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    co_tcp_accept((co_thread_t*)self, tcp_client);

    co_http_client_t* client = co_http_client_create_with(tcp_client);
    co_http_callbacks_st* callbacks = co_http_get_callbacks(client);
    callbacks->on_receive_finish = (co_http_receive_finish_fn)on_my_http_request;
    callbacks->on_close = (co_http_close_fn)on_my_http_close;
}

// create app
bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    uint16_t port = 8080;

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_tcp_server_create(&local_net_addr);

    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_my_tcp_accept;

    // listen
    return co_tcp_server_start(self->server, SOMAXCONN);
}

// destroy app
void on_my_app_destroy(my_app* self)
{
    co_tcp_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    return co_net_app_start((co_app_t*)&app, argc, argv);
}
```

* [more examples here](https://github.com/Ichishino/coldforce/tree/master/examples)
