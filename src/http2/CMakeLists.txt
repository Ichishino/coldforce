cmake_minimum_required(VERSION 2.8)

project(co_http2 C)

add_library(${PROJECT_NAME} STATIC

    co_http2.c
    co_http2_client.c
    co_http2_frame.c
    co_http2_header.c
    co_http2_hpack.c
    co_http2_http_extension.c
    co_http2_huffman.c
    co_http2_server.c
    co_http2_stream.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/lib*.*
        ${CMAKE_BINARY_DIR}/
)
