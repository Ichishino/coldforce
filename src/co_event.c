#include <coldforce/co_event.h>

//---------------------------------------------------------------------------//
// Event
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

CO_EVENT_QUEUE_T* CO_EvQueueCreate(size_t capacity)
{
    CO_Assert(capacity > 0);

    CO_EVENT_QUEUE_T* eq =
        (CO_EVENT_QUEUE_T*)CO_MemAlloc(sizeof(CO_EVENT_QUEUE_T));
    eq->buffer =
        (CO_EVENT_T*)CO_MemAlloc(sizeof(CO_EVENT_T) * capacity);

    eq->mtx = CO_MtxCreate();

    eq->capacity = capacity;
    eq->count = 0;
    eq->head = 0;
    eq->tail = 0;
    eq->stopped = false;

    return eq;
}

void CO_EvQueueDestroy(CO_EVENT_QUEUE_T** eq)
{
    CO_Assert(eq != NULL);
    CO_Assert(*eq != NULL);

    CO_MtxDestroy(&(*eq)->mtx);

    CO_MemFree((*eq)->buffer);
    CO_MemFree(*eq);

    *eq = NULL;
}

bool CO_EvQueuePush(CO_EVENT_QUEUE_T* eq, CO_EVENT_ID_T eid, uintptr_t data)
{
    CO_Assert(eq != NULL);

    CO_MtxLock(eq->mtx);

    if (eq->stopped)
    {
        CO_MtxUnlock(eq->mtx);
        return false;
    }

    if (eq->capacity == eq->count)
    {
        CO_Assert(eq->head == eq->tail);

        size_t scale = 2;

        CO_EVENT_T* newBuffer =
            (CO_EVENT_T*)CO_MemAlloc(
                sizeof(CO_EVENT_T) * (eq->capacity * scale));

        size_t len1 = eq->capacity - eq->head;

        for (size_t index = 0; index < len1; ++index)
        {
            newBuffer[index].eid = eq->buffer[eq->head + index].eid;
            newBuffer[index].data = eq->buffer[eq->head + index].data;
        }

        size_t len2 = eq->tail;

        for (size_t index = len1; index < len2; ++index)
        {
            newBuffer[len1 + index].eid = eq->buffer[index].eid;
            newBuffer[len1 + index].data = eq->buffer[index].data;
        }

        eq->capacity *= scale;

        CO_MemFree(eq->buffer);

        eq->buffer = newBuffer;
        eq->head = 0;
        eq->tail = eq->count;
    }

    if (eid == CO_EVENT_ID_STOP)
    {
        eq->stopped = true;
    }

    eq->buffer[eq->tail].eid = eid;
    eq->buffer[eq->tail].data = data;

    ++eq->tail;
    
    if (eq->capacity == eq->tail)
    {
        eq->tail = 0;
    }

    ++eq->count;

    CO_MtxUnlock(eq->mtx);

    return true;
}

bool CO_EvQueuePop(CO_EVENT_QUEUE_T* eq, CO_EVENT_ID_T* eid, uintptr_t* data)
{
    CO_Assert(eq != NULL);

    CO_MtxLock(eq->mtx);

    if (eq->count == 0)
    {
        CO_MtxUnlock(eq->mtx);
        return false;
    }

    *eid = eq->buffer[eq->head].eid;
    *data = eq->buffer[eq->head].data;

    ++eq->head;

    if (eq->capacity == eq->head)
    {
        eq->head = 0;
    }

    --eq->count;

    CO_MtxUnlock(eq->mtx);

    return true;
}

void CO_EvLoopInit(CO_EVENT_LOOP_T* eloop, size_t eqCapacity)
{
    CO_Assert(eloop != NULL);

    eloop->eq = CO_EvQueueCreate(eqCapacity);
    eloop->handlers = CO_MapCreate(NULL);
    eloop->sem = CO_SemCreate(0);
    eloop->timers = NULL;
    eloop->stopped = false;
}

void CO_EvLoopClear(CO_EVENT_LOOP_T* eloop)
{
    CO_Assert(eloop != NULL);

    eloop->stopped = false;
    CO_TimerAllStop(&eloop->timers);
    CO_SemDestroy(&eloop->sem);
    CO_MapDestroy(&eloop->handlers);
    CO_EvQueueDestroy(&eloop->eq);
}

CO_WAIT_RESULT_EN CO_EventWait(CO_EVENT_LOOP_T* eloop, uint32_t msec)
{
    CO_WAIT_RESULT_EN result = CO_SemWait(eloop->sem, msec);

    return result;
}

uint32_t CO_EventTimerPump(CO_EVENT_LOOP_T* eloop, CO_EVENT_T* e)
{
    uint32_t msec = CO_TimerGetNextTimeout(eloop->timers);

    if (msec > 0)
    {
        return msec;
    }
    else
    {
        CO_TIMER_ITEM_T* item = eloop->timers;
        eloop->timers = item->next;

        e->eid = CO_EVENT_ID_TIMER;
        e->data = (uintptr_t)item->timer;

        CO_MemFree(item);

        return 0;
    }
}

bool CO_EventPump(CO_EVENT_LOOP_T* eloop, CO_EVENT_T* e)
{
    return CO_EvQueuePop(eloop->eq, &e->eid, &e->data);
}

bool CO_EventDispatch(CO_EVENT_LOOP_T* eloop, CO_EVENT_T* e)
{
    CO_Assert(eloop != NULL);
    CO_Assert(e != NULL);

    if (e->eid == CO_EVENT_ID_TIMER)
    {
        CO_TIMER_T* timer = (CO_TIMER_T*)e->data;

        timer->running = false;

        timer->task(eloop, timer);
    }
    else if (e->eid == CO_EVENT_ID_STOP)
    {
        eloop->stopped = true;

        return false;
    }
    else
    {
        CO_EVENT_FN handler;

        if (CO_MapGet(eloop->handlers, e->eid, (uintptr_t*)&handler))
        {
            handler(eloop, e->eid, e->data);
        }
    }

    return true;
}

void CO_EvLoopRun(CO_EVENT_LOOP_T* eloop)
{
    while (!eloop->stopped)
    {
        CO_EVENT_T e = { 0 };

        uint32_t msec = CO_EventTimerPump(eloop, &e);

        if (msec == 0)
        {
            CO_EventDispatch(eloop, &e);
        }
        else
        {
            CO_WAIT_RESULT_EN waitResult = CO_EventWait(eloop, msec);

            if (waitResult == CO_WAIT_RESULT_SUCCESS)
            {
                if (CO_EventPump(eloop, &e))
                {
                    CO_EventDispatch(eloop, &e);
                }
            }
            else if (waitResult == CO_WAIT_RESULT_ERROR)
            {
                // error
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

void CO_EventSetHandler(
    void* appOrThread, CO_EVENT_ID_T id, CO_EVENT_FN handler)
{
    CO_Assert(appOrThread != NULL);
    CO_Assert(handler != NULL);

    CO_EVENT_LOOP_T* eloop = (CO_EVENT_LOOP_T*)appOrThread;

    CO_MapSet(eloop->handlers, id, (uintptr_t)handler);
}

bool CO_EventGetHandler(
    void* appOrThread, CO_EVENT_ID_T id, CO_EVENT_FN* handler)
{
    CO_Assert(appOrThread != NULL);
    CO_Assert(handler != NULL);

    CO_EVENT_LOOP_T* eloop = (CO_EVENT_LOOP_T*)appOrThread;

    return CO_MapGet(eloop->handlers, id, (uintptr_t*)handler);
}

void CO_EventRemoveHandler(
    void* appOrThread, CO_EVENT_ID_T id)
{
    CO_Assert(appOrThread != NULL);

    CO_EVENT_LOOP_T* eloop = (CO_EVENT_LOOP_T*)appOrThread;

    CO_MapRemove(eloop->handlers, id);
}

bool CO_EventPost(void* appOrThread, CO_EVENT_ID_T eid, uintptr_t data)
{
    CO_Assert(appOrThread != NULL);

    CO_EVENT_LOOP_T* eloop = (CO_EVENT_LOOP_T*)appOrThread;

    if (!CO_EvQueuePush(eloop->eq, eid, data))
    {
        return false;
    }

    CO_SemPost(eloop->sem);

    return true;
}
