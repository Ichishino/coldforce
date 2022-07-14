#include "my_server_app.h"

#include <signal.h>

void ctrl_c_handler(int sig)
{
    (void)sig;

    // quit app safely
    co_app_stop();
}

int main(int argc, char* argv[])
{
    // for debug
    signal(SIGINT, ctrl_c_handler);

    // run
    return my_server_run(argc, argv);
}
