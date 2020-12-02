#include <coldforce/core/co_std.h>
#include <coldforce/core/co_app.h>
#include <coldforce/core/co_event_worker.h>

#include <coldforce/net/co_net_app.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// net app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_net_app_init(
    co_app_t* app,
    co_create_fn create_handler,
    co_destroy_fn destroy_handler
)
{
    if (!co_net_setup())
    {
        return false;
    }

    co_net_worker_t* net_worker = co_net_worker_create();

    net_worker->on_destroy = destroy_handler;

    co_app_setup(app,
        create_handler, co_net_worker_on_destroy,
        (co_event_worker_t*)net_worker);

    return true;
}

void
co_net_app_cleanup(
    co_app_t* app
)
{
    if (app != NULL)
    {
        co_net_worker_cleanup(
            (co_net_worker_t*)app->main_thread.event_worker);

        co_app_cleanup(app);
        co_net_cleanup();
    }
}

int
co_net_app_start(
    co_app_t* app,
    int argc,
    char** argv
)
{
    co_arg_st arg = { 0 };
    arg.argc = argc;
    arg.argv = argv;

    int exit_code = co_app_run(app, &arg);

    co_net_app_cleanup(app);

    return exit_code;
}

void
co_net_app_stop(
    void
)
{
    co_app_stop();
}
