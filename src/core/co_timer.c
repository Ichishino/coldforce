#include <coldforce/core/co_std.h>
#include <coldforce/core/co_timer.h>
#include <coldforce/core/co_thread.h>

//---------------------------------------------------------------------------//
// timer
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_timer_t*
co_timer_create(
    uint32_t msec,
    co_timer_fn handler,
    bool repeat,
    uintptr_t user_data
)
{
    co_timer_t* timer =
        (co_timer_t*)co_mem_alloc(sizeof(co_timer_t));

    if (timer == NULL)
    {
        return NULL;
    }

    timer->running = false;
    timer->queued = false;

    timer->repeat = repeat;
    timer->msec = msec;
    timer->handler = handler;
    timer->user_data = user_data;

    return timer;
}

void
co_timer_destroy(
    co_timer_t* timer
)
{
    if (timer != NULL)
    {
        co_timer_stop(timer);
        co_mem_free(timer);
    }
}

bool
co_timer_start(
    co_timer_t* timer
)
{
    if (timer->running)
    {
        return false;
    }

    co_thread_t* thread = co_thread_get_current();
    co_assert(thread != NULL);

    if (!co_event_worker_register_timer(
        thread->event_worker, timer))
    {
        return false;
    }

    timer->running = true;

    return true;
}

void
co_timer_stop(
    co_timer_t* timer
)
{
    if ((timer == NULL) || !timer->running)
    {
        return;
    }

    co_thread_t* thread = co_thread_get_current();

    if (thread == NULL)
    {
        return;
    }

    co_event_worker_unregister_timer(thread->event_worker, timer);

    timer->running = false;
}

void
co_timer_set_time(
    co_timer_t* timer,
    uint32_t msec
)
{
    timer->msec = msec;
}

uint32_t
co_timer_get_time(
    const co_timer_t* timer
)
{
    return timer->msec;
}

void
co_timer_set_handler(
    co_timer_t* timer,
    co_timer_fn handler
)
{
    timer->handler = handler;
}

co_timer_fn
co_timer_get_handler(
    const co_timer_t* timer
)
{
    return timer->handler;
}

void
co_timer_set_user_data(
    co_timer_t* timer,
    uintptr_t user_data
)
{
    timer->user_data = user_data;
}

uintptr_t
co_timer_get_user_data(
    const co_timer_t* timer
)
{
    return timer->user_data;
}

void
co_timer_set_repeat(
    co_timer_t* timer,
    bool repeat
)
{
    timer->repeat = repeat;
}

bool
co_timer_get_repeat(
    const co_timer_t* timer
)
{
    return timer->repeat;
}

bool
co_timer_is_running(
    const co_timer_t* timer
)
{
    return timer->running;
}

bool
co_timer_is_queued(
    const co_timer_t* timer
)
{
    return timer->queued;
}
