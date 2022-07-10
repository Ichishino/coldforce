cmake_minimum_required(VERSION 2.8...3.13)

project(co_tls C)

add_library(${PROJECT_NAME} STATIC

    co_tls.c
    co_tls_client.c
    co_tls_log.c
    co_tls_server.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)

if (APPLE)
    target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/opt/openssl/include)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/lib*.*
        ${CMAKE_BINARY_DIR}/
)
