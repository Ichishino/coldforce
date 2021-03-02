cmake_minimum_required(VERSION 2.8)

project(https_server C)

include_directories(../../inc)

add_executable(${PROJECT_NAME} main.c)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_http.a)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_tls.a)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_net.a)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_core.a)

target_link_libraries(${PROJECT_NAME} -lssl)
target_link_libraries(${PROJECT_NAME} -lcrypto)
target_link_libraries(${PROJECT_NAME} -lpthread)
