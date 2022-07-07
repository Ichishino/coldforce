cmake_minimum_required(VERSION 2.8)

project(tcp_server_multithread C)

add_executable(${PROJECT_NAME}

    main.c
    my_server_app.c
    my_client_thread.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_net.a)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_core.a)

target_link_libraries(${PROJECT_NAME} -lpthread)

