#include <coldforce/core/co_std.h>

#include <coldforce/ws/co_ws_frame.h>
#include <coldforce/ws/co_ws_log.h>

#ifdef CO_OS_WIN
#else
#include <stdarg.h>
#endif

//---------------------------------------------------------------------------//
// websocket log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_ws_log_write_frame(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    bool fin,
    uint8_t opcode,
    size_t data_size,
    const char* format,
    ...
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->category[CO_WS_LOG_CATEGORY].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(
        level, CO_WS_LOG_CATEGORY);

    co_net_log_write_addresses(
        log, addr1, text, addr2);

    va_list args;
    va_start(args, format);
    vfprintf((FILE*)log->output, format, args);
    va_end(args);

    fprintf((FILE*)log->output, "\n");

    co_log_write_header(
        level, CO_WS_LOG_CATEGORY);
    fprintf((FILE*)log->output,
        "-------------------------------------------------------------\n");

    const char* opcode_str;

    switch (opcode)
    {
    case CO_WS_OPCODE_CONTINUATION:
        opcode_str = "CONTINUATION";
        break;
    case CO_WS_OPCODE_TEXT:
        opcode_str = "TEXT";
        break;
    case CO_WS_OPCODE_BINARY:
        opcode_str = "BINARY";
        break;
    case CO_WS_OPCODE_CLOSE:
        opcode_str = "CLOSE";
        break;
    case CO_WS_OPCODE_PING:
        opcode_str = "PING";
        break;
    case CO_WS_OPCODE_PONG:
        opcode_str = "PONG";
        break;
    default:
        opcode_str = "UNKNOWN";
        break;
    }

    co_log_write_header(
        level, CO_WS_LOG_CATEGORY);

    fprintf((FILE*)log->output,
        "%s(0x%02x) fin(%d) payload_size(%zd)\n",
        opcode_str, opcode, fin, data_size);

    co_log_write_header(
        level, CO_WS_LOG_CATEGORY);
    fprintf((FILE*)log->output,
        "-------------------------------------------------------------\n");

    fflush((FILE*)log->output);

    co_mutex_unlock(log->mutex);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_ws_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_WS_LOG_CATEGORY, level);

    co_log_add_category(
        CO_WS_LOG_CATEGORY,
        CO_WS_LOG_CATEGORY_NAME);
}

