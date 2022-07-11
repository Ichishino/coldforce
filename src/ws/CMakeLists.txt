cmake_minimum_required(VERSION 2.8...3.5)

project(co_ws C)

add_library(${PROJECT_NAME} STATIC

    co_random.c
    co_ws.c
    co_ws_client.c
    co_ws_config.c
    co_ws_frame.c
    co_ws_http_extension.c
    co_ws_log.c
    co_ws_server.c
)

include(../common.cmake)

