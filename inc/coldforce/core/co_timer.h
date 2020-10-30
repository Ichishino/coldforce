#ifndef CO_TIMER_H_INCLUDED
#define CO_TIMER_H_INCLUDED

#include <coldforce/core/co.h>

//---------------------------------------------------------------------------//
// timer
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef void(*co_timer_fn)(void* self, void* timer);

typedef struct
{
    bool running;
    bool queued;
    bool repeat;

    uint32_t msec;
    co_timer_fn handler;
    uintptr_t param;

} co_timer_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API co_timer_t* co_timer_create(
    uint32_t msec, co_timer_fn handler, bool repeat, uintptr_t param);

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

#define co_timer_set_param(timer, param) \
    (timer->param = param)

#define co_timer_get_param(timer) \
    (timer->param)

#define co_timer_is_running(timer) \
    (timer->running)

#define co_timer_is_queued(timer) \
    (timer->queued)

#define co_timer_is_repeat(timer) \
    (timer->repeat)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#endif // CO_TIMER_H_INCLUDED
