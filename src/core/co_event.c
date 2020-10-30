#include <coldforce/core/co_std.h>
#include <coldforce/core/co_event.h>
#include <coldforce/core/co_thread.h>

//---------------------------------------------------------------------------//
// event
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_event_set_handler(
    co_event_id_t event_id,
    co_event_fn handler
)
{
    co_thread_t* thread = co_thread_get_current();
    co_assert(thread != NULL);

    co_map_set(
        thread->event_worker->event_handler_map,
        event_id, (uintptr_t)handler);
}

co_event_fn
co_event_get_handler(
    co_event_id_t
    event_id
)
{
    co_thread_t* thread = co_thread_get_current();
    co_assert(thread != NULL);

    co_map_data_st* data =
        co_map_get(thread->event_worker->event_handler_map, event_id);

    return ((data != NULL) ? (co_event_fn)data->value : NULL);
}

void
co_event_remove_handler(
    co_event_id_t event_id
)
{
    co_thread_t* thread = co_thread_get_current();
    co_assert(thread != NULL);

    co_map_remove(
        thread->event_worker->event_handler_map, event_id);
}

bool
co_event_send(
    void* app_or_thread,
    co_event_id_t event_id,
    uintptr_t param1,
    uintptr_t param2
)
{
    co_event_t event = { event_id, param1, param2 };

    return co_event_worker_add(
        ((co_thread_t*)app_or_thread)->event_worker, &event);
}
