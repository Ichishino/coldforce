#ifndef _COLDFORCE_EVENT_H_
#define _COLDFORCE_EVENT_H_

#include <coldforce/co_std.h>
#include <coldforce/co_map.h>
#include <coldforce/co_sem.h>
#include <coldforce/co_mutex.h>
#include <coldforce/co_timer.h>

//---------------------------------------------------------------------------//
// Event
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef uint16_t CO_EVENT_ID_T;
typedef void(*CO_EVENT_FN)(void*, CO_EVENT_ID_T, uintptr_t);

typedef struct
{
    CO_EVENT_ID_T eid;
    uintptr_t data;

} CO_EVENT_T;

typedef struct
{
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;

    CO_EVENT_T* buffer;
    CO_MTX_T* mtx;

    bool stopped;

} CO_EVENT_QUEUE_T;

struct CO_EVENT_LOOP_ST
{
    CO_EVENT_QUEUE_T* eq;
    CO_MAP_T* handlers;
    CO_TIMER_ITEM_T* timers;

    CO_SEM_T* sem;

    bool stopped;
};
typedef struct CO_EVENT_LOOP_ST CO_EVENT_LOOP_T;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

CO_WAIT_RESULT_EN CO_EventWait(CO_EVENT_LOOP_T* eloop, uint32_t msec);

CO_EVENT_QUEUE_T* CO_EvQueueCreate(size_t capacity);
void CO_EvQueueDestroy(CO_EVENT_QUEUE_T** eq);
bool CO_EvQueuePush(CO_EVENT_QUEUE_T* eq, CO_EVENT_ID_T eid, uintptr_t data);
bool CO_EvQueuePop(CO_EVENT_QUEUE_T* eq, CO_EVENT_ID_T* eid, uintptr_t* data);

void CO_EvLoopInit(CO_EVENT_LOOP_T*, size_t eqCapacity);
void CO_EvLoopClear(CO_EVENT_LOOP_T* eloop);
void CO_EvLoopRun(CO_EVENT_LOOP_T* eloop);

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_API void CO_EventSetHandler(void* appOrThread, CO_EVENT_ID_T eid, CO_EVENT_FN handler);
CO_API bool CO_EventGetHandler(void* appOrThread, CO_EVENT_ID_T eid, CO_EVENT_FN* handler);
CO_API void CO_EventRemoveHandler(void* appOrThread, CO_EVENT_ID_T eid);

CO_API bool CO_EventPost(void* appOrThread, CO_EVENT_ID_T eid, uintptr_t data);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // _COLDFORCE_EVENT_H_