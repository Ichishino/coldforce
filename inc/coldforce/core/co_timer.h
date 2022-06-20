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
    uintptr_t user_data;

} co_timer_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API co_timer_t* co_timer_create(
    uint32_t msec, co_timer_fn handler, bool repeat, uintptr_t user_data);

CO_API void co_timer_destroy(co_timer_t* timer);

CO_API bool co_timer_start(co_timer_t* timer);
CO_API void co_timer_stop(co_timer_t* timer);

#define co_timer_set_time(timer, value) \
    (timer->msec = value)

#define co_timer_get_time(timer) \
    (timer->msec)

#define co_timer_set_handler(timer, handler) \
    (timer->handler = handler)

#define co_timer_get_handler(timer) \
    (timer->handler)

#define co_timer_set_user_data(timer, user_data) \
    (timer->user_data = user_data)

#define co_timer_get_user_data(timer) \
    (timer->user_data)

#define co_timer_is_running(timer) \
    (timer->running)

#define co_timer_is_queued(timer) \
    (timer->queued)

#define co_timer_is_repeat(timer) \
    (timer->repeat)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TIMER_H_INCLUDED
