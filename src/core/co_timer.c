#include <coldforce/core/co_std.h>
#include <coldforce/core/co_timer.h>
#include <coldforce/core/co_thread.h>

//---------------------------------------------------------------------------//
// timer
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_timer_t*
co_timer_create(
    uint32_t msec,
    co_timer_fn handler,
    bool repeat,
    uintptr_t param
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
    timer->param = param;

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
