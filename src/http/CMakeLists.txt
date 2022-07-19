cmake_minimum_required(VERSION 2.8...3.5)

project(co_http C)

add_library(${PROJECT_NAME} STATIC

    co_base64.c
    co_http.c
    co_http_client.c
    co_http_config.c
    co_http_connection.c
    co_http_content_receiver.c
    co_http_cookie.c
    co_http_header.c
    co_http_log.c
    co_http_message.c
    co_http_request.c
    co_http_response.c
    co_http_server.c
    co_http_string_list.c
    co_http_sync.c
    co_http_url.c
    co_md5.c
    co_sha1.c
)

include(../common.cmake)

