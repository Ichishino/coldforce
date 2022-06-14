cmake_minimum_required(VERSION 2.8)

project(co_net C)

add_library(${PROJECT_NAME} STATIC

    co_net.c
    co_net_addr.c
    co_net_addr_resolve.c
    co_net_app.c
    co_net_log.c
    co_net_selector_linux.c
    co_net_thread.c
    co_net_worker.c
    co_socket.c
    co_socket_handle.c
    co_socket_option.c
    co_tcp_client.c
    co_tcp_server.c
    co_udp.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/lib*.*
        ${CMAKE_BINARY_DIR}/
)
