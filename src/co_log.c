#include <coldforce/core/co_std.h>
#include <coldforce/core/co_log.h>
#include <coldforce/core/co_mutex.h>

#ifdef CO_OS_WIN
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

//---------------------------------------------------------------------------//
// log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static co_log_t g_log = { 0 };
static const char* g_level_name[] = { "", "ERR", "WRN", "INF", "DBG" };

co_log_t*
co_log_get_default(
    void
)
{
    return &g_log;
}

static void
co_log_cleanup(
    void
)
{
    if (g_log.mutex != NULL)
    {
        co_mutex_destroy(g_log.mutex);
        g_log.mutex = NULL;
    }
}

static void
co_log_setup(
    void
)
{
    if (g_log.mutex != NULL)
    {
        return;
    }

    g_log.level = CO_LOG_LEVEL_DEBUG;
    g_log.mutex = co_mutex_create();
    g_log.output = stdout;

    for (size_t index = 0; index <= CO_LOG_CATEGORY_MAX; ++index)
    {
        g_log.category[index].enable = false;
        g_log.category[index].name = "";
    }

    g_log.category[
        CO_LOG_CATEGORY_USER_DEFAULT].name =
        CO_LOG_CATEGORY_NAME_USER_DEFAULT;

    atexit(co_log_cleanup);
}

void
co_log_write_header(
    int level,
    int category
)
{
#ifdef CO_OS_WIN
    SYSTEMTIME st;
    GetLocalTime(&st);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* lt = localtime(&tv.tv_sec);
#endif

    fprintf((FILE*)g_log.output,
        "<<coldforce>> %d-%02d-%02d %02d:%02d:%02d:%03d [%s] <%s> ",
#ifdef CO_OS_WIN
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
#else
        lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
        lt->tm_hour, lt->tm_min, lt->tm_sec, (int)(tv.tv_usec / 1000),
#endif
        g_level_name[level], g_log.category[category].name);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_log_set_level(
    int level
)
{
    co_log_setup();

    g_log.level = level;
}

int
co_log_get_level(
    void
)
{
    return g_log.level;
}


void
co_log_set_enable(
    int category,
    bool enable
)
{
    co_assert(category <= CO_LOG_CATEGORY_MAX);

    g_log.category[category].enable = enable;
}

bool
co_log_get_enable(
    int category
)
{
    co_assert(category <= CO_LOG_CATEGORY_MAX);

    return g_log.category[category].enable;
}

void
co_log_add_category(
    int category,
    const char* name
)
{
    co_assert(category <= CO_LOG_CATEGORY_MAX);

    g_log.category[category].name = name;
}

void
co_log_write(
    int level,
    int category,
    const char* format,
    ...
)
{
    co_assert(level < CO_LOG_LEVEL_MAX);
    co_assert(category <= CO_LOG_CATEGORY_MAX);

    if (level > g_log.level ||
        !g_log.category[category].enable)
    {
        return;
    }

    co_mutex_lock(g_log.mutex);

    co_log_write_header(level, category);

    va_list args;
    va_start(args, format);
    vfprintf((FILE*)g_log.output, format, args);
    va_end(args);

    fprintf((FILE*)g_log.output, "\n");
    fflush((FILE*)g_log.output);

    co_mutex_unlock(g_log.mutex);
}
