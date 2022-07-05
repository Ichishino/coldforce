#include <coldforce/core/co_std.h>
#include <coldforce/core/co_event_worker.h>
#include <coldforce/core/co_thread.h>

//---------------------------------------------------------------------------//
// event worker
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

co_event_worker_t*
co_event_worker_create(
    void
)
{
    co_event_worker_t* event_worker =
        (co_event_worker_t*)co_mem_alloc(sizeof(co_event_worker_t));

    if (event_worker == NULL)
    {
        return NULL;
    }

    event_worker->running = false;
    event_worker->stop_receiving = true;

    event_worker->event_queue = NULL;
    event_worker->event_queue_mutex = NULL;
    event_worker->event_handler_map = NULL;
    event_worker->timer_manager = NULL;
    event_worker->wait_semaphore = NULL;

    event_worker->wait = NULL;
    event_worker->wake_up = NULL;
    event_worker->dispatch = NULL;
    event_worker->on_idle = NULL;

    return event_worker;
}

void
co_event_worker_destroy(
    co_event_worker_t* event_worker
)
{
    co_mem_free(event_worker);
}

void
co_event_worker_setup(
    co_event_worker_t* event_worker
)
{
    event_worker->running = true;
    event_worker->stop_receiving = false;

    event_worker->event_queue =
        co_queue_create(sizeof(co_event_t), NULL);
    event_worker->event_queue_mutex = co_mutex_create();
    event_worker->event_handler_map = co_map_create(NULL);
    event_worker->wait_semaphore = co_semaphore_create(0);
    event_worker->timer_manager = co_timer_manager_create();

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_item_free_fn)co_mem_free;
    event_worker->mem_trash = co_list_create(&list_ctx);

    if (event_worker->wait == NULL)
    {
        event_worker->wait = co_event_worker_wait;
    }

    if (event_worker->wake_up == NULL)
    {
        event_worker->wake_up = co_event_worker_wake_up;
    }

    if (event_worker->dispatch == NULL)
    {
        event_worker->dispatch = co_event_worker_dispatch;
    }

    if (event_worker->on_idle == NULL)
    {
        event_worker->on_idle = co_event_worker_on_idle;
    }
}

void
co_event_worker_cleanup(
    co_event_worker_t* event_worker
)
{
    co_timer_manager_destroy(event_worker->timer_manager);
    event_worker->timer_manager = NULL;

    co_semaphore_destroy(event_worker->wait_semaphore);
    event_worker->wait_semaphore = NULL;

    co_map_destroy(event_worker->event_handler_map);
    event_worker->event_handler_map = NULL;

    co_mutex_destroy(event_worker->event_queue_mutex);
    event_worker->event_queue_mutex = NULL;

    co_queue_destroy(event_worker->event_queue);
    event_worker->event_queue = NULL;

    co_list_destroy(event_worker->mem_trash);
    event_worker->mem_trash = NULL;

    event_worker->stop_receiving = true;
    event_worker->running = false;
}

void
co_mem_free_later(
    void* mem
)
{
    if (mem == NULL)
    {
        return;
    }

    co_thread_t* thread = co_thread_get_current();

    if (thread != NULL)
    {
        co_list_add_tail(
            thread->event_worker->mem_trash, (uintptr_t)mem);
    }
    else
    {
        co_mem_free(mem);
    }
}

void
co_event_worker_run(
    co_event_worker_t* event_worker
)
{
    while (event_worker->running)
    {
        uint32_t msec =
            co_timer_manager_get_next_timeout(event_worker->timer_manager);

        co_wait_result_t result = event_worker->wait(event_worker, msec);

        if (result == CO_WAIT_RESULT_ERROR)
        {
            break;
        }

        co_event_worker_check_timer(event_worker);
        
        long event_count =
            (long)co_event_worker_get_event_count(event_worker);

        co_event_t event = { 0 };

        while ((event_count > 0) &&
            co_event_worker_pump(event_worker, &event))
        {
            event_worker->dispatch(event_worker, &event);

            --event_count;
        }

        if (event.event_id == 0)
        {
            event_worker->on_idle(event_worker);
        }
    }
}

size_t
co_event_worker_get_event_count(
    co_event_worker_t* event_worker
)
{
    co_mutex_lock(event_worker->event_queue_mutex);

    size_t count = co_queue_get_count(event_worker->event_queue);

    co_mutex_unlock(event_worker->event_queue_mutex);

    return count;
}

co_wait_result_t
co_event_worker_wait(
    co_event_worker_t* event_worker,
    uint32_t msec
)
{
    return co_semaphore_wait(
        event_worker->wait_semaphore, msec);
}

void
co_event_worker_wake_up(
    co_event_worker_t* event_worker
)
{
    co_semaphore_post(event_worker->wait_semaphore);
}

bool
co_event_worker_dispatch(
    co_event_worker_t* event_worker,
    co_event_t* event
)
{
    switch (event->event_id)
    {
    case CO_EVENT_ID_TIMER:
    {
        if (event->param2)
        {
            co_timer_t* timer = (co_timer_t*)event->param1;

            timer->running = false;
            timer->queued = false;

            if (co_timer_get_repeat(timer))
            {
                co_timer_start(timer);
            }

            co_timer_fn handler = co_timer_get_handler(timer);

            handler(co_thread_get_current(), timer);
        }

        break;
    }
    case CO_EVENT_ID_STOP:
    {
        event_worker->running = false;

        break;
    }
    case CO_EVENT_ID_TASK:
    {
        co_task_t* task = (co_task_t*)event->param1;

        task->handler(task->param1, task->param2);

        co_mem_free(task);

        break;
    }
    default:
    {
        const co_map_data_st* data = co_map_get(
            event_worker->event_handler_map, event->event_id);

        if (data == NULL)
        {
            return false;
        }

        co_event_fn handler = (co_event_fn)data->value;

        handler(co_thread_get_current(), event);

        break;
    }
    }

    return true;
}

void
co_event_worker_on_idle(
    co_event_worker_t* event_worker
)
{
    if (co_list_get_count(event_worker->mem_trash) > 0)
    {
        co_list_clear(event_worker->mem_trash);
    }
}

bool
co_event_worker_add(
    co_event_worker_t* event_worker,
    const co_event_t* event
)
{
    bool result = false;

    co_mutex_lock(event_worker->event_queue_mutex);

    if (!event_worker->stop_receiving)
    {
        if (event->event_id == CO_EVENT_ID_STOP)
        {
            event_worker->stop_receiving = true;
        }

        co_queue_push(event_worker->event_queue, event);

        event_worker->wake_up(event_worker);

        result = true;
    }

    co_mutex_unlock(event_worker->event_queue_mutex);

    return result;
}

bool
co_event_worker_pump(
    co_event_worker_t* event_worker,
    co_event_t* event
)
{
    co_event_worker_check_timer(event_worker);

    co_mutex_lock(event_worker->event_queue_mutex);

    bool result =
        co_queue_pop(event_worker->event_queue, event);

    co_mutex_unlock(event_worker->event_queue_mutex);

    return result;
}

bool
co_event_worker_register_timer(
    co_event_worker_t* event_worker,
    co_timer_t* timer
)
{
    return co_timer_manager_register(
        event_worker->timer_manager, timer);
}

intptr_t
co_compare_event(
    const co_event_t* event1,
    const co_event_t* event2
)
{
    return
        ((event1->event_id == event2->event_id) &&
         (event1->param1 == event2->param1) &&
         (event1->param2 == event2->param2)) ? 0 : 1;
}

void
co_event_worker_unregister_timer(
    co_event_worker_t* event_worker,
    co_timer_t* timer
)
{
    if (timer->queued)
    {
        co_event_t timer_event = {
            CO_EVENT_ID_TIMER, (uintptr_t)timer, true };

        co_event_t* queued_event = (co_event_t*)
            co_queue_find(event_worker->event_queue,
                &timer_event, (co_item_compare_fn)co_compare_event);
        co_assert(queued_event != NULL);

        queued_event->param2 = false;

        timer->queued = false;
    }
    else
    {
        co_timer_manager_unregister(
            event_worker->timer_manager, timer);
    }
}

void
co_event_worker_check_timer(
    co_event_worker_t* event_worker
)
{
    uint32_t msec =
        co_timer_manager_get_next_timeout(event_worker->timer_manager);

    while (msec == 0)
    {
        co_timer_t* timer =
            co_timer_manager_remove_head_timer(event_worker->timer_manager);

        co_event_t timer_event = {
            CO_EVENT_ID_TIMER, (uintptr_t)timer, true };

        if (co_event_worker_add(event_worker, &timer_event))
        {
            timer->queued = true;
        }

        msec = co_timer_manager_get_next_timeout(event_worker->timer_manager);
    }
}
