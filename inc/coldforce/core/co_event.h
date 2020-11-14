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

typedef uint16_t co_event_id_t;

typedef struct
{
    co_event_id_t event_id;

    uintptr_t param1;
    uintptr_t param2;

} co_event_t;

typedef void(*co_event_fn)(void* self, const co_event_t* event);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API void co_event_set_handler(
    struct co_thread_t* thread, co_event_id_t event_id,
    co_event_fn handler);

CO_API co_event_fn co_event_get_handler(
    struct co_thread_t* thread, co_event_id_t event_id);

CO_API void co_event_remove_handler(
    struct co_thread_t* thread, co_event_id_t event_id);

CO_API bool co_event_send(
    struct co_thread_t* thread, co_event_id_t event_id,
    uintptr_t param1, uintptr_t param2);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_EVENT_H_INCLUDED
