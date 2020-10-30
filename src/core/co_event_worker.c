#include <coldforce/core/co_std.h>
#include <coldforce/core/co_event_worker.h>
#include <coldforce/core/co_thread.h>

//---------------------------------------------------------------------------//
// event worker
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
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

    event_worker->run = NULL;
    event_worker->wake_up = NULL;

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

    if (event_worker->run == NULL)
    {
        event_worker->run = co_event_worker_run;
    }

    if (event_worker->wake_up == NULL)
    {
        event_worker->wake_up = co_event_worker_wake_up;
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

    event_worker->stop_receiving = true;
    event_worker->running = false;
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

        co_wait_result_t result =
            co_semaphore_wait(event_worker->wait_semaphore, msec);

        if (result == CO_WAIT_RESULT_ERROR)
        {
            break;
        }

        co_event_t event;

        if (co_event_worker_pump(event_worker, &event))
        {
            co_event_worker_dispatch(event_worker, &event);
        }
    }
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
    if (event->event_id == CO_EVENT_ID_TIMER)
    {
        if (event->param2)
        {
            co_timer_t* timer = (co_timer_t*)event->param1;

            timer->running = false;
            timer->queued = false;

            if (co_timer_is_repeat(timer))
            {
                co_timer_start(timer);
            }

            co_timer_fn handler = co_timer_get_handler(timer);

            handler(co_thread_get_current(), timer);
        }
    }
    else if (event->event_id == CO_EVENT_ID_STOP)
    {
        event_worker->running = false;
    }
    else
    {
        const co_map_data_st* data =
            co_map_get(event_worker->event_handler_map, event->event_id);

        if (data == NULL)
        {
            return false;
        }

        co_event_fn handler = (co_event_fn)data->value;

        handler(co_thread_get_current(), event);
    }

    return true;
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
    co_mutex_lock(event_worker->event_queue_mutex);

    uint32_t msec =
        co_timer_manager_get_next_timeout(event_worker->timer_manager);

    while (msec == 0)
    {
        co_timer_t* timer =
            co_timer_manager_remove_head_timer(event_worker->timer_manager);

        co_event_t timer_event =
            { CO_EVENT_ID_TIMER, (uintptr_t)timer, true };

        timer->queued = true;

        co_queue_push(
            event_worker->event_queue, &timer_event);

        event_worker->wake_up(event_worker);

        msec = co_timer_manager_get_next_timeout(event_worker->timer_manager);
    }

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
         (event1->param2 == event2->param2));
}

void
co_event_worker_unregister_timer(
    co_event_worker_t* event_worker,
    co_timer_t* timer
)
{
    if (timer->queued)
    {
        co_event_t timer_event =
            { CO_EVENT_ID_TIMER, (uintptr_t)timer, true };

        co_event_t* queued_event = (co_event_t*)
            co_queue_find(event_worker->event_queue,
                &timer_event, (co_compare_fn)co_compare_event);
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