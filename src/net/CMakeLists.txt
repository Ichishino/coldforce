cmake_minimum_required(VERSION 2.8...3.5)

project(co_net C)

add_library(${PROJECT_NAME} STATIC

    co_net.c
    co_net_addr.c
    co_net_addr_resolve.c
    co_net_app.c
    co_net_log.c
    co_net_selector_linux.c
    co_net_selector_mac.c
    co_net_selector_win.c
    co_net_thread.c
    co_net_worker.c
    co_socket.c
    co_socket_handle.c
    co_socket_option.c
    co_tcp_client.c
    co_tcp_server.c
    co_udp.c
    co_udp_server.c
    co_url.c
)

include(../common.cmake)

