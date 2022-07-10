cmake_minimum_required(VERSION 2.8...3.13)

project(co_core C)

add_library(${PROJECT_NAME} STATIC

    co.c
    co_app.c
    co_array.c    
    co_config.c
    co_event.c
    co_event_worker.c
    co_list.c
    co_log.c
    co_map.c
    co_mutex.c
    co_queue.c
    co_semaphore.c
    co_string.c
    co_thread.c
    co_time.c
    co_timer.c
    co_timer_manager.c
)

target_include_directories(${PROJECT_NAME} PUBLIC ../../inc)
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/lib*.*
        ${CMAKE_BINARY_DIR}/
)
