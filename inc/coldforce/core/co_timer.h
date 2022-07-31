#ifndef CO_TIMER_H_INCLUDED
#define CO_TIMER_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

struct co_thread_t;

//---------------------------------------------------------------------------//
// timer
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_timer_t;

typedef void(*co_timer_fn)(struct co_thread_t* self, struct co_timer_t* timer);

typedef struct co_timer_t
{
    bool running;
    bool queued;
    bool repeat;

    uint32_t msec;
    co_timer_fn handler;
    void* user_data;

} co_timer_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
co_timer_t*
co_timer_create(
    uint32_t msec,
    co_timer_fn handler,
    bool repeat,
    void* user_data
);

CO_CORE_API
void
co_timer_destroy(
    co_timer_t* timer
);

CO_CORE_API
bool
co_timer_start(
    co_timer_t* timer
);

CO_CORE_API
void
co_timer_stop(
    co_timer_t* timer
);

CO_CORE_API
void
co_timer_set_time(
    co_timer_t* timer,
    uint32_t msec
);

CO_CORE_API
uint32_t
co_timer_get_time(
    const co_timer_t* timer
);

CO_CORE_API
void
co_timer_set_handler(
    co_timer_t* timer,
    co_timer_fn handler
);

CO_CORE_API
co_timer_fn
co_timer_get_handler(
    const co_timer_t* timer
);

CO_CORE_API
void
co_timer_set_user_data(
    co_timer_t* timer,
    void* user_data
);

CO_CORE_API
void*
co_timer_get_user_data(
    const co_timer_t* timer
);

CO_CORE_API
void
co_timer_set_repeat(
    co_timer_t* timer,
    bool repeat
);

CO_CORE_API
bool
co_timer_get_repeat(
    const co_timer_t* timer
);

CO_CORE_API
bool
co_timer_is_running(
    const co_timer_t* timer
);

CO_CORE_API
bool
co_timer_is_queued(
    const co_timer_t* timer
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TIMER_H_INCLUDED
