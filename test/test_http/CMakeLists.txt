cmake_minimum_required(VERSION 2.8...3.5)

project(test_http C)

include(../../src/tls_option.cmake)

add_executable(${PROJECT_NAME}

    main.c
    test_app.c
    test_http_server_http_connection.c
    test_http_server_http2_connection.c
    test_http_server_thread.c
    test_http_server_ws_connection.c
    test_ws_http2_client_thread.c
)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)

target_link_libraries(${PROJECT_NAME} -pthread -lm)
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_ws_http2.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_ws.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_http2.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_http.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_tls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_net.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libco_core.a
)

include(../../examples/tls_link.cmake)
