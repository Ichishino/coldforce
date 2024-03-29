#ifndef CO_EVENT_H_INCLUDED
#define CO_EVENT_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

struct co_thread_t;

//---------------------------------------------------------------------------//
// event
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_EVENT_ID_STOP    0x7001
#define CO_EVENT_ID_TIMER   0x7002
#define CO_EVENT_ID_TASK    0x7003

typedef uint16_t co_event_id_t;

typedef struct
{
    co_event_id_t id;

    uintptr_t param1;
    uintptr_t param2;

} co_event_st;

typedef void(*co_event_fn)(struct co_thread_t* self, const co_event_st* event);
typedef void(*co_task_fn)(uintptr_t param);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
void
co_thread_set_event_handler(
    struct co_thread_t* thread,
    co_event_id_t event_id,
    co_event_fn handler
);

CO_CORE_API
co_event_fn
co_thread_get_event_handler(
    const struct co_thread_t* thread,
    co_event_id_t event_id
);

CO_CORE_API
void
co_thread_remove_event_handler(
    struct co_thread_t* thread,
    co_event_id_t event_id
);

CO_CORE_API
bool
co_thread_send_event(
    struct co_thread_t* thread,
    co_event_id_t event_id,
    uintptr_t param1,
    uintptr_t param2
);

CO_CORE_API
bool
co_thread_send_task_event(
    struct co_thread_t* thread,
    co_task_fn task,
    uintptr_t param
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_EVENT_H_INCLUDED
