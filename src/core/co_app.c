#include <coldforce/core/co_std.h>
#include <coldforce/core/co_app.h>

#ifdef CO_OS_WIN
#   include <windows.h>
#else
#   include <pthread.h>
#endif

//---------------------------------------------------------------------------//
// app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static co_app_t* current_app = NULL;
extern CO_THREAD_LOCAL co_thread_t* current_thread;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_app_setup(
    co_app_t* app,
    co_ctx_st* ctx
)
{
    co_assert(current_app == NULL);
    co_assert(current_thread == NULL);

    co_thread_setup(&app->thread, ctx);

#ifdef CO_OS_WIN
    app->thread.handle =
        (co_thread_handle_t*)GetCurrentThread();
#else
    pthread_t* pthread = (pthread_t*)co_mem_alloc(sizeof(pthread_t));
    *pthread = pthread_self();
    app->thread.handle = (co_thread_handle_t*)pthread;
#endif

    current_app = app;
    current_thread = (co_thread_t*)app;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_app_t*
co_app_get_current(
    void
)
{
    return current_app;
}

co_app_t*
co_app_create(
    co_ctx_st* ctx
)
{
    size_t size = sizeof(co_app_t);

    if (ctx != NULL)
    {
        size = ctx->object_size;
    }

    size = co_max(size, sizeof(co_app_t));

    co_app_t* app = (co_app_t*)co_mem_alloc(size);

    if (app == NULL)
    {
        return NULL;
    }

    memset(app, 0x00, size);

    co_app_setup(app, ctx);

    return app;
}

void
co_app_destroy(
    co_app_t* app
)
{
    if (app != NULL)
    {
        co_thread_cleanup(&app->thread);
        co_mem_free(app);
    }
    
    current_thread = NULL;
    current_app = NULL;
}

int
co_app_run(
    co_app_t* app,
    co_app_param_st* app_param
)
{
    bool create_result = true;

    if (app->thread.on_create != NULL)
    {
        create_result =
            app->thread.on_create(app, (uintptr_t)app_param);
    }

    if (create_result)
    {
        app->thread.event_worker->run(app->thread.event_worker);
    }

    if (app->thread.on_destroy != NULL)
    {
        app->thread.on_destroy(app);
    }

    return app->thread.exit_code;
}

int
co_app_start(
    co_ctx_st* ctx,
    co_app_param_st* param
)
{
    co_app_t* app = co_app_create(ctx);

    int exit_code = co_app_run(app, param);

    co_app_destroy(app);

    return exit_code;
}

void
co_app_stop(
    void
)
{
    co_app_t* app = co_app_get_current();

    co_thread_stop(&app->thread);
}
