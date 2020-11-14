#include <coldforce/core/co_std.h>
#include <coldforce/core/co_event_worker.h>

#include <coldforce/net/co_net_thread.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// net thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_thread_t*
co_net_thread_start(
    co_ctx_st* ctx,
    uintptr_t param
)
{
    ctx->event_worker =
        (co_event_worker_t*)co_net_worker_create();

    return co_thread_start(ctx, param);
}

void
co_net_thread_destroy(
    co_thread_t* thread
)
{
    co_net_worker_cleanup(
        (co_net_worker_t*)thread->event_worker);

    co_thread_destroy(thread);
}
