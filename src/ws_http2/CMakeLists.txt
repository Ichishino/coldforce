cmake_minimum_required(VERSION 2.8...3.5)

project(co_ws_http2 C)

add_library(${PROJECT_NAME} STATIC

    co_ws_http2.c
    co_ws_http2_extension.c
)

include(../common.cmake)
