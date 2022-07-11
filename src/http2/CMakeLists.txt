cmake_minimum_required(VERSION 2.8...3.5)

project(co_http2 C)

add_library(${PROJECT_NAME} STATIC

    co_http2.c
    co_http2_client.c
    co_http2_frame.c
    co_http2_header.c
    co_http2_hpack.c
    co_http2_http_extension.c
    co_http2_huffman.c
    co_http2_log.c
    co_http2_server.c
    co_http2_stream.c
)

include(../common.cmake)

