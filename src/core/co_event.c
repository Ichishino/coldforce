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
    co_thread_t* thread,
    co_event_id_t event_id,
    co_event_fn handler
)
{
    co_map_set(
        thread->event_worker->event_handler_map,
        event_id, (uintptr_t)handler);
}

co_event_fn
co_event_get_handler(
    co_thread_t* thread,
    co_event_id_t
    event_id
)
{
    co_map_data_st* data =
        co_map_get(thread->event_worker->event_handler_map, event_id);

    return ((data != NULL) ? (co_event_fn)data->value : NULL);
}

void
co_event_remove_handler(
    co_thread_t* thread,
    co_event_id_t event_id
)
{
    co_map_remove(
        thread->event_worker->event_handler_map, event_id);
}

bool
co_event_send(
    co_thread_t* thread,
    co_event_id_t event_id,
    uintptr_t param1,
    uintptr_t param2
)
{
    co_event_t event = { event_id, param1, param2 };

    return co_event_worker_add(thread->event_worker, &event);
}

bool
co_event_send_task(
    co_thread_t* thread,
    co_task_fn handler,
    uintptr_t param1,
    uintptr_t param2
)
{
    co_task_t* task =
        (co_task_t*)co_mem_alloc(sizeof(co_task_t));

    if (task == NULL)
    {
        return false;
    }

    task->handler = handler;
    task->param1 = param1;
    task->param2 = param2;

    co_event_t event = { CO_EVENT_ID_TASK, (uintptr_t)task, 0 };

    if (!co_event_worker_add(thread->event_worker, &event))
    {
        co_mem_free(task);

        return false;
    }

    return true;
}