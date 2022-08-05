cmake_minimum_required(VERSION 2.8...3.5)

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
    co_random.c
    co_semaphore.c
    co_string.c
    co_string_token.c
    co_thread.c
    co_time.c
    co_timer.c
    co_timer_manager.c
)

include(../common.cmake)

