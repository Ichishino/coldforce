#include "test_app.h"

#include <signal.h>

#ifdef _WIN32
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif

void on_signal(int sig)
{
    (void)sig;

    co_app_stop();
}

int main(int argc, char** argv)
{
    co_win_crt_set_dbg();
    signal(SIGINT, on_signal);

    co_core_log_set_level(CO_LOG_LEVEL_MAX);
    co_http_log_set_level(CO_LOG_LEVEL_MAX);
    co_http2_log_set_level(CO_LOG_LEVEL_MAX);
    co_ws_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_INFO);

    return test_app_run(argc, argv);
}
