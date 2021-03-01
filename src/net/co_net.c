#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_net_win.h>
#else
#include <signal.h>
#include <time.h>
#endif

//---------------------------------------------------------------------------//
// network
//---------------------------------------------------------------------------//

bool
co_net_setup(
    void
)
{
#ifdef CO_OS_WIN
    if (!co_win_net_setup())
    {
        return false;
    }
#else
    signal(SIGPIPE, SIG_IGN);
    srandom(time(NULL));
#endif

    return true;
}

void
co_net_cleanup(
    void
)
{
#ifdef CO_OS_WIN
    co_win_net_cleanup();
#endif
}
