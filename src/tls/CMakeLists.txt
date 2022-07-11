cmake_minimum_required(VERSION 2.8...3.5)

project(co_tls C)

add_library(${PROJECT_NAME} STATIC

    co_tls.c
    co_tls_client.c
    co_tls_log.c
    co_tls_server.c
)

if (APPLE)
    target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/opt/openssl/include)
endif()

include(../common.cmake)

