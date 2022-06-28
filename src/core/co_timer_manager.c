#include <coldforce/core/co_std.h>
#include <coldforce/core/co_timer_manager.h>
#include <coldforce/core/co_time.h>

//---------------------------------------------------------------------------//
// timer manager
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

co_timer_manager_t*
co_timer_manager_create(
    void
)
{
    co_timer_manager_t* timer_manager =
        (co_timer_manager_t*)co_mem_alloc(sizeof(co_timer_manager_t));

    if (timer_manager == NULL)
    {
        return NULL;
    }

    co_list_ctx_st ctx = { 0 };
    ctx.free_value = (co_item_free_fn)co_mem_free;

    timer_manager->timers = co_list_create(&ctx);

    return timer_manager;
}

void
co_timer_manager_destroy(
    co_timer_manager_t* timer_manager
)
{
    co_timer_manager_clear(timer_manager);
    co_list_destroy(timer_manager->timers);

    co_mem_free(timer_manager);
}

void
co_timer_manager_clear(
    co_timer_manager_t* timer_manager
)
{
    co_list_clear(timer_manager->timers);
}

bool
co_timer_manager_register(
    co_timer_manager_t* timer_manager,
    co_timer_t* timer
)
{
    co_timer_item_t* new_item =
        (co_timer_item_t*)co_mem_alloc(sizeof(co_timer_item_t));

    if (new_item == NULL)
    {
        return false;
    }

    new_item->end = co_get_current_time_in_msecs() + timer->msec;
    new_item->timer = timer;

    co_list_iterator_t* it =
        co_list_get_head_iterator(timer_manager->timers);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get(timer_manager->timers, it);

        if (((co_timer_item_t*)data->value)->end > new_item->end)
        {
            co_list_insert(
                timer_manager->timers, it, (uintptr_t)new_item);

            return true;
        }

        it = co_list_get_next_iterator(timer_manager->timers, it);
    }

    co_list_add_tail(timer_manager->timers, (uintptr_t)new_item);

    return true;
}

bool
co_timer_manager_unregister(
    co_timer_manager_t* timer_manager,
    co_timer_t* timer
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(timer_manager->timers);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get(timer_manager->timers, it);

        if (((co_timer_item_t*)data->value)->timer == timer)
        {
            co_list_remove_at(timer_manager->timers, it);

            return true;
        }

        it = co_list_get_next_iterator(timer_manager->timers, it);
    }

    return false;
}

uint32_t
co_timer_manager_get_next_timeout(
    co_timer_manager_t* timer_manager
)
{
    if (co_list_get_count(timer_manager->timers) == 0)
    {
        return CO_INFINITE;
    }
    else
    {
        uint64_t now = co_get_current_time_in_msecs();

        const co_list_data_st* data =
            co_list_get_head(timer_manager->timers);

        uint64_t end =
            ((const co_timer_item_t*)data->value)->end;

        if (now >= end)
        {
            return 0;
        }
        else
        {
            return (uint32_t)(end - now);
        }
    }
}

co_timer_t*
co_timer_manager_remove_head_timer(
    co_timer_manager_t* timer_manager
)
{
    co_timer_t* timer = NULL;

    const co_list_data_st* data =
        co_list_get_head(timer_manager->timers);

    if (data != NULL)
    {
        timer = ((co_timer_item_t*)data->value)->timer;

        co_list_remove_head(timer_manager->timers);
    }

    return timer;
}
