cmake_minimum_required(VERSION 2.8...3.5)

project(udp_conn_client C)

add_executable(${PROJECT_NAME} main.c)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)

target_link_libraries(${PROJECT_NAME} -pthread -lm)
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_net.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_core.a
)

