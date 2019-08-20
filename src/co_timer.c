#include <coldforce/co_timer.h>
#include <coldforce/co_event.h>
#include <coldforce/co_thread.h>

#ifdef CO_OS_WIN
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#elif defined(CO_OS_LINUX)
#include <math.h>
#include <time.h>
#elif defined(CO_OS_MAC)
#endif

//---------------------------------------------------------------------------//
// Timer
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

uint64_t CO_GetCurrentTimeInMsecs()
{
#ifdef CO_OS_WIN

    return timeGetTime();

#elif defined(CO_OS_LINUX)

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t ms = round(ts.tv_nsec / 1000000);

    return (ts.tv_sec * 1000) + ((ms >= 1000) ? 1000 : ms);

#elif defined(CO_OS_MAC)
#endif
}

uint32_t CO_TimerGetNextTimeout(CO_TIMER_ITEM_T* item)
{
    if (item == NULL)
    {
        return CO_INFINITE;
    }
    else
    {
        uint64_t now = CO_GetCurrentTimeInMsecs();
        
        if (now >= item->end)
        {
            return 0;
        }
        else
        {
            return (uint32_t)(item->end - now);
        }
    }
}

void CO_TimerAllStop(CO_TIMER_ITEM_T** items)
{
    CO_Assert(items != NULL);

    CO_TIMER_ITEM_T* item = *items;

    while (item != NULL)
    {
        CO_TIMER_ITEM_T* temp = item;
        item = item->next;

        CO_MemFree(temp);
    }

    *items = NULL;
}

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_TIMER_T* CO_TimerCreate(uint32_t msec, CO_TIMER_FN task, uintptr_t param)
{
    CO_Assert(task != NULL);

    CO_TIMER_T* timer =
        (CO_TIMER_T*)CO_MemAlloc(sizeof(CO_TIMER_T));

    timer->msec = msec;
    timer->task = task;
    timer->param = param;
    timer->running = false;

    return timer;
}

void CO_TimerDestroy(CO_TIMER_T** timer)
{
    CO_Assert(timer != NULL);
    CO_Assert(*timer != NULL);

    CO_MemFree(*timer);
    *timer = NULL;
}

void CO_TimerStart(CO_TIMER_T* timer)
{
    CO_Assert(timer != NULL);
    CO_Assert(!timer->running);

    CO_EVENT_LOOP_T* eloop =
        (CO_EVENT_LOOP_T*)CO_ThreadGetCurrent();
    CO_Assert(eloop != NULL);

    CO_TIMER_ITEM_T* newItem =
        (CO_TIMER_ITEM_T*)CO_MemAlloc(sizeof(CO_TIMER_ITEM_T));

    newItem->end = CO_GetCurrentTimeInMsecs() + timer->msec;
    newItem->timer = timer;
    newItem->next = NULL;

    timer->running = true;

    if (eloop->timers == NULL)
    {
        eloop->timers = newItem;
    }
    else
    {
        CO_TIMER_ITEM_T* item = eloop->timers;
        CO_TIMER_ITEM_T* prev = NULL;

        while (item != NULL)
        {
            if (item->end > newItem->end)
            {
                if (prev != NULL)
                {
                    prev->next = newItem;
                }
                else
                {
                    eloop->timers = newItem;
                }

                newItem->next = item;

                return;
            }

            prev = item;
            item = item->next;
        }

        prev->next = newItem;
    }
}

void CO_TimerStop(CO_TIMER_T* timer)
{
    CO_Assert(timer != NULL);

    if (!timer->running)
    {
        return;
    }

    CO_EVENT_LOOP_T* eloop =
        (CO_EVENT_LOOP_T*)CO_ThreadGetCurrent();
    CO_Assert(eloop != NULL);

    CO_TIMER_ITEM_T* item = eloop->timers;
    CO_TIMER_ITEM_T* prev = NULL;

    while (item != NULL)
    {
        if (item->timer == timer)
        {
            timer->running = false;

            if (prev != NULL)
            {
                prev->next = item->next;
            }
            else
            {
                eloop->timers = item->next;
            }

            CO_MemFree(item);

            break;
        }

        prev = item;
        item = item->next;
    }
}
