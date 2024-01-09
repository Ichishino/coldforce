cmake_minimum_required(VERSION 2.8...3.5)

project(test_suite C)

include(../../src/tls_option.cmake)

add_executable(${PROJECT_NAME}

    main.c
    test_app.c
    test_tcp.c
    test_tcp_mt_client.c
    test_tcp_mt_server.c
    test_tcp_server.c
    test_tcp_server_comm.c
    test_udp.c
    test_udp_server.c
    test_udp2.c
    test_udp2_server.c
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
