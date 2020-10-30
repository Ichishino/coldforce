#ifndef CO_TIMER_MANAGER_H_INCLUDED
#define CO_TIMER_MANAGER_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_timer.h>
#include <coldforce/core/co_list.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// timer manager
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct co_timer_item_t
{
    co_timer_t* timer;
    uint64_t end;

} co_timer_item_t;

typedef struct
{
    co_list_t* timers;

} co_timer_manager_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_timer_manager_t* co_timer_manager_create(void);

void co_timer_manager_destroy(co_timer_manager_t* timer_manager);

void co_timer_manager_clear(co_timer_manager_t* timer_manager);

uint32_t co_timer_manager_get_next_timeout(co_timer_manager_t* timer_manager);

co_timer_t* co_timer_manager_remove_head_timer(co_timer_manager_t* timer_manager);

bool co_timer_manager_register(
    co_timer_manager_t* timer_manager, co_timer_t* timer);

bool co_timer_manager_unregister(
    co_timer_manager_t* timer_manager, co_timer_t* timer);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TIMER_MANAGER_H_INCLUDED
