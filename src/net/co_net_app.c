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

co_app_t*
co_net_app_create(
    co_ctx_st* ctx
)
{
    if (!co_net_setup())
    {
        return NULL;
    }

    ctx->event_worker =
        (co_event_worker_t*)co_net_worker_create();

    return co_app_create(ctx);
}

void
co_net_app_destroy(
    co_app_t* app
)
{
    co_net_worker_cleanup(
        (co_net_worker_t*)app->thread.event_worker);

    co_app_destroy(app);

    co_net_cleanup();
}

int
co_net_app_start(
    co_ctx_st* ctx,
    co_app_param_st* param
)
{
    co_app_t* app = co_net_app_create(ctx);

    if (app == NULL)
    {
        return -1;
    }

    int exit_code = co_app_run(app, param);

    co_net_app_destroy(app);

    return exit_code;
}
