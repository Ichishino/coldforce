#include <coldforce/core/co_std.h>

#include <coldforce/ws/co_ws_frame.h>
#include <coldforce/ws/co_ws_log.h>

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

    if (level > log->category[CO_LOG_CATEGORY_WS].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(
        level, CO_LOG_CATEGORY_WS);

    co_net_log_write_addresses(
        log, CO_LOG_CATEGORY_WS, addr1, text, addr2);

    FILE* fp =
        (FILE*)log->category[CO_LOG_CATEGORY_WS].output;

    va_list args;
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);

    fprintf(fp, "\n");

    co_log_write_header(
        level, CO_LOG_CATEGORY_WS);
    fprintf(fp,
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
        level, CO_LOG_CATEGORY_WS);

    fprintf(fp,
        "%s(0x%02x) fin(%d) payload_size(%zd)\n",
        opcode_str, opcode, fin, data_size);

    co_log_write_header(
        level, CO_LOG_CATEGORY_WS);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    fflush(fp);

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
        CO_LOG_CATEGORY_WS, level);

    co_log_add_category(
        CO_LOG_CATEGORY_WS,
        CO_LOG_CATEGORY_NAME_WS);
}
