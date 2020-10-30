#ifndef CO_EVENT_WORKER_H_INCLUDED
#define CO_EVENT_WORKER_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_map.h>
#include <coldforce/core/co_mutex.h>
#include <coldforce/core/co_semaphore.h>
#include <coldforce/core/co_event.h>
#include <coldforce/core/co_queue.h>
#include <coldforce/core/co_timer_manager.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// event worker
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef bool(*co_create_fn)(void* self, uintptr_t param);
typedef void(*co_destroy_fn)(void* self);

typedef struct co_event_worker_t
{
    bool running;
    bool stop_receiving;

    co_queue_t* event_queue;
    co_mutex_t* event_queue_mutex;

    co_map_t* event_handler_map;
    co_timer_manager_t* timer_manager;
    co_semaphore_t* wait_semaphore;

    void(*run)(struct co_event_worker_t*);
    void(*wake_up)(struct co_event_worker_t*);

} co_event_worker_t;

typedef struct
{
    size_t object_size;
    co_create_fn on_create;
    co_destroy_fn on_destroy;

    co_event_worker_t* event_worker;

} co_ctx_st;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_event_worker_t* co_event_worker_create(void);
void co_event_worker_destroy(co_event_worker_t* event_worker);

void co_event_worker_setup(co_event_worker_t* event_worker);
void co_event_worker_cleanup(co_event_worker_t* event_worker);
void co_event_worker_run(co_event_worker_t* event_worker);

void co_event_worker_wake_up(
    co_event_worker_t* event_worker);
bool co_event_worker_dispatch(
    co_event_worker_t* event_worker, co_event_t* event);

bool co_event_worker_add(
    co_event_worker_t* event_worker, const co_event_t* event);
bool co_event_worker_pump(
    co_event_worker_t* event_worker, co_event_t* event);

bool co_event_worker_register_timer(
    co_event_worker_t* event_worker, co_timer_t* timer);
void co_event_worker_unregister_timer(
    co_event_worker_t* event_worker, co_timer_t* timer);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_EVENT_WORKER_H_INCLUDED
