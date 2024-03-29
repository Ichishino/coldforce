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

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

bool
co_net_setup(
    void
)
{
    static bool once = false;

    if (!once)
    {
        once = true;
    }
    else
    {
        return true;
    }

#ifdef CO_OS_WIN
    if (!co_win_net_setup())
    {
        return false;
    }
#else
    signal(SIGPIPE, SIG_IGN);
    srandom((unsigned int)time(NULL));
#endif

    atexit(co_net_cleanup);

    return true;
}

void
co_net_cleanup(
    void
)
{
    static bool once = false;

    if (!once)
    {
        once = true;
    }
    else
    {
        return;
    }

#ifdef CO_OS_WIN
    co_win_net_cleanup();
#endif
}
