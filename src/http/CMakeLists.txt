cmake_minimum_required(VERSION 2.8)

project(co_http C)

add_library(${PROJECT_NAME} STATIC

    co_http.c
    co_http_client.c
    co_http_content_receiver.c
    co_http_cookie.c
    co_http_header.c
    co_http_message.c
    co_http_request.c
    co_http_response.c
    co_http_url.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)
target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/lib*.*
        ${CMAKE_BINARY_DIR}/
)
