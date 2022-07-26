Coldforce
========
[![Build](https://github.com/Ichishino/coldforce/actions/workflows/build.yml/badge.svg)](https://github.com/Ichishino/coldforce/actions/workflows/build.yml)

Coldforce is a library written in C that supports various network protocols.  
With the asynchronous API of this framework
You can easily create event-driven network applications.  
The currently supported protocols are as follows.
All of these support clients and servers (multi-client, C10K).
* TCP/UDP (IPv4/IPv6)
* TLS
* HTTP/1.1 (http/https, pipelining, Basic/Digest authentication)
* HTTP/2
* WebSocket (ws/wss)

### Platforms
* Windows
* Linux
* macOS

### Requirements
* C99 or later
* OpenSSL (only when using TLS, https, wss)
* Use `-pthread` `-lm`

### Modules
* Coldforce core : `co_core.dll` `libco_core.a`
* Network core (TCP,UDP) : `co_net.dll` `libco_net.a`
* TLS : `co_tls.dll` `libco_tls.a`
* HTTP/1.1 : `co_http.dll` `libco_http.a`
* HTTP/2 : `co_http2.dll` `libco_http2.a`
* WebSocket : `co_ws.dll` `libco_ws.a`

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

### Code Examples
* WebSocket echo server -> ws://127.0.0.1:8080
```C++
#include <coldforce.h>

typedef struct {
    co_app_t base_app;
    co_tcp_server_t* server;
    co_list_t* clients;
} my_app;

void on_my_ws_receive_frame(
    my_app* self, co_ws_client_t* client,
    const co_ws_frame_t* frame, int error_code)
{
    if (error_code == 0)
    {
        bool fin = co_ws_frame_get_fin(frame);
        uint8_t opcode = co_ws_frame_get_opcode(frame);
        size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
        const uint8_t* data = co_ws_frame_get_payload_data(frame);

        switch (opcode)
        {
        case CO_WS_OPCODE_TEXT:
        case CO_WS_OPCODE_BINARY:
        case CO_WS_OPCODE_CONTINUATION:
        {
            co_ws_send(client, fin, opcode, data, data_size);

            break;
        }
        default:
        {
            co_ws_default_handler(client, frame);

            break;
        }
        }
    }
    else
    {
        co_list_remove(self->clients, client);
    }
}

void on_my_ws_close(
    my_app* self, co_ws_client_t* client)
{
    co_list_remove(self->clients, client);
}

void on_my_ws_handshake(
    my_app* self, co_ws_client_t* client,
    const co_http_request_t* request, int unused)
{
    if (co_http_request_validate_ws_upgrade(request))
    {
        co_http_response_t* response =
            co_http_response_create_ws_upgrade(
                request, NULL, NULL);
        co_http_connection_send_response(
            (co_http_connection_t*)client, response);
    }
    else
    {
        co_list_remove(self->clients, client);
    }
}

void on_my_tcp_accept(
    my_app* self, co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client)
{
    co_tcp_accept((co_thread_t*)self, tcp_client);

    co_ws_client_t* ws_client = co_ws_client_create_with(tcp_client);

    co_ws_callbacks_st* callbacks = co_ws_get_callbacks(ws_client);
    callbacks->on_handshake = (co_ws_handshake_fn)on_my_ws_handshake;
    callbacks->on_receive_frame = (co_ws_receive_frame_fn)on_my_ws_receive_frame;
    callbacks->on_close = (co_ws_close_fn)on_my_ws_close;

    co_list_add_tail(self->clients, ws_client);
}

bool on_my_app_create(my_app* self)
{
    uint16_t port = 8080;

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_ws_client_destroy;
    self->clients = co_list_create(&list_ctx);

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_tcp_server_create(&local_net_addr);

    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_my_tcp_accept;

    return co_tcp_server_start(self->server, SOMAXCONN);
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->clients);
    co_tcp_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);

    int exit_code = co_app_run((co_app_t*)&app);

    co_net_app_cleanup((co_app_t*)&app);

    return exit_code;
}
```

* [more examples here](https://github.com/Ichishino/coldforce/tree/master/examples)

