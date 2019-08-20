cmake_minimum_required(VERSION 2.8)

project(co_core)

add_library(co_core STATIC 

    co_app.c
    co_event.c
    co_map.c
    co_mutex.c
    co_sem.c
    co_std.c
    co_thread.c
    co_timer.c
)

target_include_directories(co_core PUBLIC ../inc)

#add_executable(test main.c)
#target_link_libraries(test co_core -pthread)
