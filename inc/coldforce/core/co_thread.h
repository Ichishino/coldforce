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

typedef struct
{
    uintptr_t unused;

} co_thread_handle_t;

typedef struct co_thread_t
{
    co_create_fn on_create;
    co_destroy_fn on_destroy;

    struct co_thread_t* parent;
    co_event_worker_t* event_worker;
    co_thread_handle_t* handle;

    int exit_code;

} co_thread_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void co_thread_setup(co_thread_t* thread, co_ctx_st* ctx);
void co_thread_cleanup(co_thread_t* thread);

CO_API co_thread_t* co_thread_start(co_ctx_st* ctx, uintptr_t param);

CO_API co_thread_t* co_thread_get_current(void);
CO_API co_thread_t* co_thread_get_parent(void);

CO_API void co_thread_destroy(co_thread_t* thread);
CO_API void co_thread_stop(co_thread_t* thread);
CO_API void co_thread_wait(co_thread_t* thread);

CO_API void co_thread_set_exit_code(int exit_code);
CO_API int co_thread_get_exit_code(co_thread_t* thread);

CO_API co_thread_handle_t* co_thread_get_handle(co_thread_t* thread);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_THREAD_H_INCLUDED
