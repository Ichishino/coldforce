#ifndef CO_APP_H_INCLUDED
#define CO_APP_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_thread.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef co_thread_create_fn co_app_create_fn;
typedef co_thread_destroy_fn co_app_destroy_fn;

typedef unsigned long co_app_id_t;

typedef struct
{
    int count;
    char** values;

} co_args_st;

typedef struct
{
    co_thread_t main_thread;
    co_app_id_t id;
    co_args_st args;

} co_app_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_CORE_API
void
co_app_setup_internal(
    co_app_t* app,
    const char* name,
    co_app_create_fn create_handler,
    co_app_destroy_fn destroy_handler,
    co_event_worker_t* event_worker,
    int argc,
    char** argv
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
void
co_app_setup(
    co_app_t* app,
    const char* name,
    co_app_create_fn create_handler,
    co_app_destroy_fn destroy_handler,
    int argc,
    char** argv
);

CO_CORE_API
void
co_app_cleanup(
    co_app_t* app
);

CO_CORE_API
int
co_app_run(
    co_app_t* app
);

CO_CORE_API
void
co_app_stop(
    void
);

CO_CORE_API
const co_args_st*
co_app_get_args(
    const co_app_t* app
);

CO_CORE_API
co_app_t*
co_app_get_current(
    void
);

CO_CORE_API
co_app_id_t
co_app_get_id(
    void
);

CO_CORE_API
const char*
co_app_get_name(
    void
);

CO_CORE_API
void
co_app_set_exit_code(
    int exit_code
);

CO_CORE_API
int
co_app_get_exit_code(
    void
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_APP_H_INCLUDED
