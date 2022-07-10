cmake_minimum_required(VERSION 2.8...3.13)

project(tcp_echo_server C)

add_executable(${PROJECT_NAME} main.c)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../build)

target_link_libraries(${PROJECT_NAME} -pthread -lm)
target_link_libraries(${PROJECT_NAME}
    libco_core.a
    libco_net.a
)
