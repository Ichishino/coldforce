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

typedef struct
{
    int argc;
    char** argv;

    uintptr_t param;

} co_app_param_st;

typedef struct
{
    co_thread_t thread;

} co_app_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API co_app_t* co_app_create(co_ctx_st* ctx);

CO_API void co_app_destroy(co_app_t* app);

CO_API int co_app_run(co_app_t* app, co_app_param_st* param);

CO_API int co_app_start(co_ctx_st* ctx, co_app_param_st* param);

CO_API void co_app_stop(void);

CO_API co_app_t* co_app_get_current(void);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_APP_H_INCLUDED
