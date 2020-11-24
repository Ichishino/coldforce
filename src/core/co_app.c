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
co_app_init(
    co_app_t* app,
    co_create_fn create_handler,
    co_destroy_fn destroy_handler
)
{
    co_app_setup(
        app, create_handler, destroy_handler, NULL);
}

void
co_app_setup(
    co_app_t* app,
    co_create_fn create_handler,
    co_destroy_fn destroy_handler,
    co_event_worker_t* event_worker
)
{
    co_assert(current_app == NULL);
    co_assert(current_thread == NULL);

    co_thread_setup(&app->main_thread,
        create_handler, destroy_handler, event_worker);

#ifdef CO_OS_WIN
    app->main_thread.handle =
        (co_thread_handle_t*)GetCurrentThread();
#else
    pthread_t* pthread = (pthread_t*)co_mem_alloc(sizeof(pthread_t));
    *pthread = pthread_self();
    app->main_thread.handle = (co_thread_handle_t*)pthread;
#endif

    current_app = app;
    current_thread = (co_thread_t*)app;
}

void
co_app_cleanup(
    co_app_t* app
)
{
    if (app != NULL)
    {
        co_thread_cleanup(&app->main_thread);
    }
}

int
co_app_run(
    co_app_t* app,
    co_arg_st* arg
)
{
    bool create_result = true;

    if (app->main_thread.on_create != NULL)
    {
        create_result =
            app->main_thread.on_create(app, (uintptr_t)arg);
    }

    if (create_result)
    {
        co_thread_run(&app->main_thread);
    }

    if (app->main_thread.on_destroy != NULL)
    {
        app->main_thread.on_destroy(app);
    }

    return app->main_thread.exit_code;
}

int
co_app_start(
    co_app_t* app,
    int argc,
    char** argv
)
{
    co_arg_st arg = { 0 };
    arg.argc = argc;
    arg.argv = argv;

    int exit_code = co_app_run(app, &arg);

    co_app_cleanup(app);

    return exit_code;
}

void
co_app_stop(
    void
)
{
    co_app_t* app = co_app_get_current();

    if (app != NULL)
    {
        co_thread_stop(&app->main_thread);
    }
}

co_app_t*
co_app_get_current(
    void
)
{
    return current_app;
}
