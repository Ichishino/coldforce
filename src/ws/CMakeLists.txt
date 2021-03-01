cmake_minimum_required(VERSION 2.8)

project(co_ws C)

add_library(${PROJECT_NAME} STATIC

    co_random.c
    co_ws.c
    co_ws_client.c
    co_ws_frame.c
    co_ws_http_extension.c
    co_ws_server.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)
target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/lib*.*
        ${CMAKE_BINARY_DIR}/
)
