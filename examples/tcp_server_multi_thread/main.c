#include "tcp_server_app.h"

#include <signal.h>

void
on_signal(
    int sig
)
{
    (void)sig;

    // quit app safely
    co_app_stop();
}

//---------------------------------------------------------------------------//
// main
//---------------------------------------------------------------------------//

int
main(
    int argc,
    char* argv[]
)
{
    co_win_debug_crt_set_flags();

    // for debug
    signal(SIGINT, on_signal);

    // run
    return tcp_server_app_run(argc, argv);
}
