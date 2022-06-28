#include <coldforce/core/co_std.h>
#include <coldforce/core/co_mutex.h>

#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_net_log.h>

#ifdef CO_OS_WIN
#else
#include <stdarg.h>
#include <ctype.h>
#endif

//---------------------------------------------------------------------------//
// net log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_net_log_write_addresses(
    co_log_t* log,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2
)
{
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
}

void
co_net_log_write_data(
    co_log_t* log,
    int level,
    int category,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const char* format,
    ...
)
{
    co_log_write_header(level, category);
    co_net_log_write_addresses(log, addr1, text, addr2);

    va_list args;
    va_start(args, format);
    vfprintf((FILE*)log->output, format, args);
    va_end(args);

    fprintf((FILE*)log->output, "\n");
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

    if (level > log->category[category].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(level, category);
    co_net_log_write_addresses(log, addr1, text, addr2);

    va_list args;
    va_start(args, format);
    vfprintf((FILE*)log->output, format, args);
    va_end(args);

    fprintf((FILE*)log->output, "\n");
    fflush((FILE*)log->output);

    co_mutex_unlock(log->mutex);
}

void
co_net_log_write_hex_dump(
    int level,
    int category,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const void* data,
    size_t size,
    const char* format,
    ...
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->category[category].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(level, category);
    co_net_log_write_addresses(log, addr1, text, addr2);

    va_list args;
    va_start(args, format);
    vfprintf((FILE*)log->output, format, args);
    va_end(args);

    fprintf((FILE*)log->output, "\n");

    co_log_write_header(level, category);

    fprintf((FILE*)log->output,
        "--------------------------------------------------------------\n");

    const uint8_t* bytes = (const uint8_t*)data;

    for (size_t index1 = 0; index1 < size; index1 += 16)
    {
        char hex[64];
        char txt[32];

        char* phex = hex;
        char* ptxt = txt;

        for (size_t index2 = 0;
            (index2 < 16) && ((index1 + index2) < size);
            ++index2)
        {
            uint8_t byte = bytes[index1 + index2];

            if (((index2 % 4) == 0) && (index2 > 0))
            {
                *phex = ' ';
                phex++;
            }

            sprintf(phex, "%02x", byte);
            phex += 2;

            if (isprint(byte))
            {
                *ptxt = byte;
            }
            else
            {
                *ptxt = '.';
            }

            ++ptxt;
        }

        *phex = '\0';
        *ptxt = '\0';

        co_log_write_header(level, category);

        fprintf((FILE*)log->output,
            "%08zx: %-35s %s\n", index1, hex, txt);
    }

    co_log_write_header(level, category);

    fprintf((FILE*)log->output,
        "--------------------------------------------------------------\n");

    fflush((FILE*)log->output);

    co_mutex_unlock(log->mutex);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_tcp_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_LOG_CATEGORY_TCP, level);

    co_log_add_category(
        CO_LOG_CATEGORY_TCP,
        CO_LOG_CATEGORY_NAME_TCP);
}

void
co_udp_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_LOG_CATEGORY_UDP, level);

    co_log_add_category(
        CO_LOG_CATEGORY_UDP,
        CO_LOG_CATEGORY_NAME_UDP);
}
