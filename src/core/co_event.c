#include <coldforce/core/co_std.h>
#include <coldforce/core/co_event.h>
#include <coldforce/core/co_thread.h>

//---------------------------------------------------------------------------//
// event
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_thread_set_event_handler(
    co_thread_t* thread,
    co_event_id_t event_id,
    co_event_fn handler
)
{
    co_map_set(
        thread->event_worker->event_handler_map,
        (void*)(uintptr_t)event_id, (void*)handler);
}

co_event_fn
co_thread_get_event_handler(
    const co_thread_t* thread,
    co_event_id_t event_id
)
{
    const co_map_data_st* data =
        co_map_get(thread->event_worker->event_handler_map,
            (void*)(uintptr_t)event_id);

    return ((data != NULL) ? (co_event_fn)data->value : NULL);
}

void
co_thread_remove_event_handler(
    co_thread_t* thread,
    co_event_id_t event_id
)
{
    co_map_remove(
        thread->event_worker->event_handler_map,
        (void*)(uintptr_t)event_id);
}

bool
co_thread_send_event(
    co_thread_t* thread,
    co_event_id_t event_id,
    uintptr_t param1,
    uintptr_t param2
)
{
    co_event_st event = { event_id, param1, param2 };

    return co_event_worker_add(thread->event_worker, &event);
}

bool
co_thread_send_task_event(
    co_thread_t* thread,
    co_task_fn handler,
    uintptr_t param
)
{
    co_event_st event = {
        CO_EVENT_ID_TASK, (uintptr_t)handler, param
    };

    if (!co_event_worker_add(thread->event_worker, &event))
    {
        return false;
    }

    return true;
}
