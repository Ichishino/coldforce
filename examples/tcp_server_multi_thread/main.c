#include "my_server_app.h"

#include <signal.h>

void ctrl_c_handler(int sig)
{
    (void)sig;

    // quit app safely
    co_net_app_stop();
}

int main(int argc, char* argv[])
{
    // for debug
    signal(SIGINT, ctrl_c_handler);

    my_server_app server_app;

    init_my_server_app(&server_app);

    // app start
    return co_net_app_start((co_app_t*)&server_app, argc, argv);
}
