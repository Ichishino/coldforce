#include <coldforce/core/co_std.h>
#include <coldforce/core/co_time.h>

#ifdef CO_OS_WIN
#   include <windows.h>
#   pragma comment(lib, "winmm.lib")
#else
#   include <math.h>
#   include <time.h>
#endif

//---------------------------------------------------------------------------//
// time
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

uint64_t
co_get_current_time_in_msecs(
    void
)
{
#ifdef CO_OS_WIN
    return timeGetTime();
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t ms = round(ts.tv_nsec / 1000000);

    return (ts.tv_sec * 1000) + ((ms >= 1000) ? 1000 : ms);
#endif
}