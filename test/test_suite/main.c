#include "test_app.h"

#include <signal.h>

#ifdef _WIN32
#   ifdef CO_USE_WOLFSSL
#       pragma comment(lib, "wolfssl.lib")
#   elif defined(CO_USE_OPENSSL)
#       pragma comment(lib, "libssl.lib")
#       pragma comment(lib, "libcrypto.lib")
#   endif
#endif

void ctrl_c_handler(int sig)
{
    (void)sig;

    // quit app safely
    co_app_stop();
}

//---------------------------------------------------------------------------//
// main
//---------------------------------------------------------------------------//

int main(int argc, char** argv)
{
    signal(SIGINT, ctrl_c_handler);

    return test_app_run(argc, argv);
}
