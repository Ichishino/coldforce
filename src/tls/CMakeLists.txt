cmake_minimum_required(VERSION 2.8...3.5)

project(co_tls C)

include(../tls_option.cmake)

message(STATUS TLS_LIB=${use_tls_lib})

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

