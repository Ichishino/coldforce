cmake_minimum_required(VERSION 2.8...3.5)

project(http2_server C)

include(../../src/tls_option.cmake)

add_executable(${PROJECT_NAME} main.c)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall)
target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)
target_link_libraries(${PROJECT_NAME} -pthread -lm)
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_http2.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_http.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_tls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_net.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_core.a
)

include(../tls_link.cmake)

