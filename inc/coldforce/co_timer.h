#ifndef _COLDFORCE_TIMER_H_
#define _COLDFORCE_TIMER_H_

#include <coldforce/co_std.h>

//---------------------------------------------------------------------------//
// Timer
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef void(*CO_TIMER_FN)(void*, void*);

typedef struct
{
    uint32_t msec;
    CO_TIMER_FN task;
    uintptr_t param;

    bool running;

} CO_TIMER_T;

struct CO_TIMER_ITEM_ST
{
    CO_TIMER_T* timer;
    uint64_t end;

    struct CO_TIMER_ITEM_ST* next;
};
typedef struct CO_TIMER_ITEM_ST CO_TIMER_ITEM_T;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

uint32_t CO_TimerGetNextTimeout(CO_TIMER_ITEM_T* item);
void CO_TimerAllStop(CO_TIMER_ITEM_T** items);

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_API CO_TIMER_T* CO_TimerCreate(uint32_t msec, CO_TIMER_FN task, uintptr_t param);
CO_API void CO_TimerDestroy(CO_TIMER_T** timer);

CO_API void CO_TimerStart(CO_TIMER_T* timer);
CO_API void CO_TimerStop(CO_TIMER_T* timer);

#define CO_TimerSetTime(timer, value) (timer->msec = value)
#define CO_TimerGetTime(timer) (timer->msec)

#define CO_TimerSetTask(timer, value) (timer->task = value)
#define CO_TimerGetTask(timer) (timer->task)

#define CO_TimerSetParam(timer, value) (timer->param = value)
#define CO_TimerGetParam(timer) (timer->param)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // _COLDFORCE_TIMER_H_
