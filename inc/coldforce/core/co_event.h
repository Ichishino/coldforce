#ifndef CO_EVENT_H_INCLUDED
#define CO_EVENT_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// event
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
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

CO_API void co_event_set_handler(co_event_id_t event_id, co_event_fn handler);

CO_API co_event_fn co_event_get_handler(co_event_id_t event_id);

CO_API void co_event_remove_handler(co_event_id_t event_id);

CO_API bool co_event_send(void* app_or_thread,
    co_event_id_t event_id, uintptr_t param1, uintptr_t param2);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_EVENT_H_INCLUDED
