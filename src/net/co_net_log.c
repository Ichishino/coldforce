#include <coldforce/core/co_std.h>
#include <coldforce/core/co_mutex.h>

#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_net_log.h>

//---------------------------------------------------------------------------//
// net log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_tcp_log_enable(
    bool enable
)
{
    co_log_add_category(
        CO_TCP_LOG_CATEGORY,
        CO_TCP_LOG_CATEGORY_NAME);

    co_log_set_enable(
        CO_TCP_LOG_CATEGORY, enable);
}

void
co_udp_log_enable(
    bool enable
)
{
    co_log_add_category(
        CO_UDP_LOG_CATEGORY,
        CO_UDP_LOG_CATEGORY_NAME);

    co_log_set_enable(
        CO_UDP_LOG_CATEGORY, enable);
}

void
co_net_log_write(
    int level,
    int category,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const char* format,
    ...
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->level ||
        !log->category[category].enable)
    {
        return;
    }

    char addr1_str[64];
    addr1_str[0] = '\0';

    if (addr1 != NULL)
    {
        co_net_addr_to_string(
            addr1, addr1_str, sizeof(addr1_str));
    }

    char addr2_str[64];
    addr2_str[0] = '\0';

    if (addr2 != NULL)
    {
        co_net_addr_to_string(
            addr2, addr2_str, sizeof(addr2_str));
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(level, category);

    if (addr1 != NULL)
    {
        fprintf((FILE*)log->output, "(%s) ", addr1_str);
    }

    if (text != NULL)
    {
        fprintf((FILE*)log->output, "%s ", text);
    }

    if (addr2 != NULL)
    {
        fprintf((FILE*)log->output, "(%s) ", addr2_str);
    }

    va_list args;
    va_start(args, format);
    vfprintf((FILE*)log->output, format, args);
    va_end(args);

    fprintf((FILE*)log->output, "\n");
    fflush((FILE*)log->output);

    co_mutex_unlock(log->mutex);
}
