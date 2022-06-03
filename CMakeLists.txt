cmake_minimum_required(VERSION 2.8)

project(coldforce)

add_subdirectory(src/core)
add_subdirectory(src/net)
add_subdirectory(src/tls)
add_subdirectory(src/http)
add_subdirectory(src/http2)
add_subdirectory(src/ws)

add_subdirectory(examples)

