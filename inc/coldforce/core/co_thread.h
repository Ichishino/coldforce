#ifndef CO_THREAD_H_INCLUDED
#define CO_THREAD_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_event_worker.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_thread_t;

typedef bool(*co_thread_create_fn)(struct co_thread_t* self);
typedef void(*co_thread_destroy_fn)(struct co_thread_t* self);

typedef unsigned long co_thread_id_t;

typedef struct
{
    uintptr_t unused;

} co_thread_handle_t;

typedef struct co_thread_t
{
    co_thread_create_fn on_create;
    co_thread_destroy_fn on_destroy;

    co_thread_id_t id;
    co_thread_handle_t* handle;
    co_event_worker_t* event_worker;
    struct co_thread_t* parent;
    char* name;

    int exit_code;

} co_thread_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_CORE_API
void co_thread_setup_internal(
    co_thread_t* thread,
    const char* name,
    co_thread_create_fn create_handler,
    co_thread_destroy_fn destroy_handler,
    co_event_worker_t* event_worker
);

void
co_thread_run(
    co_thread_t* thread
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
void
co_thread_setup(
    co_thread_t* thread,
    const char* name,
    co_thread_create_fn create_handler,
    co_thread_destroy_fn destroy_handler
);

CO_CORE_API
void
co_thread_cleanup(
    co_thread_t* thread
);

CO_CORE_API
bool
co_thread_start(
    co_thread_t* thread
);

CO_CORE_API
void
co_thread_stop(
    co_thread_t* thread
);

CO_CORE_API
void
co_thread_join(
    co_thread_t* thread
);

CO_CORE_API
co_thread_t*
co_thread_get_current(
    void
);

CO_CORE_API
co_thread_t*
co_thread_get_parent(
    co_thread_t* thread
);

CO_CORE_API
void
co_thread_set_exit_code(
    int exit_code
);

CO_CORE_API
int
co_thread_get_exit_code(
    const co_thread_t* thread
);

CO_CORE_API
co_thread_id_t
co_thread_get_id(
    const co_thread_t* thread
);

CO_CORE_API
co_thread_handle_t*
co_thread_get_handle(
    co_thread_t* thread
);

CO_CORE_API
const char*
co_thread_get_name(
    const co_thread_t* thread
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_THREAD_H_INCLUDED
