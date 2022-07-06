#include <coldforce/core/co_std.h>

#include <coldforce/http2/co_http2_log.h>

//---------------------------------------------------------------------------//
// http2 log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http2_log_write_header(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const co_http2_header_t* header,
    const char* format,
    ...
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->category[CO_LOG_CATEGORY_HTTP2].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP2);

    co_net_log_write_addresses(
        log, CO_LOG_CATEGORY_HTTP2, addr1, text, addr2);

    FILE* fp =
        (FILE*)log->category[CO_LOG_CATEGORY_HTTP2].output;

    va_list args;
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);

    fprintf(fp, "\n");

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP2);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    if (header->pseudo.authority != NULL)
    {
        co_log_write_header(
            level, CO_LOG_CATEGORY_HTTP2);

        fprintf(fp,
            ":authority: %s\n", header->pseudo.authority);
    }

    if (header->pseudo.method != NULL)
    {
        co_log_write_header(
            level, CO_LOG_CATEGORY_HTTP2);

        fprintf(fp,
            ":method: %s\n", header->pseudo.method);
    }

    if (header->pseudo.url != NULL)
    {
        co_log_write_header(
            level, CO_LOG_CATEGORY_HTTP2);

        fprintf(fp,
            ":path: %s\n", header->pseudo.url->src);
    }

    if (header->pseudo.scheme != NULL)
    {
        co_log_write_header(
            level, CO_LOG_CATEGORY_HTTP2);

        fprintf(fp,
            ":scheme: %s\n", header->pseudo.scheme);
    }

    if (header->pseudo.status_code != 0)
    {
        co_log_write_header(
            level, CO_LOG_CATEGORY_HTTP2);

        fprintf(fp,
            ":status: %u\n", header->pseudo.status_code);
    }

    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_const_next(header->field_list, &it);
        const co_http2_header_field_t* field =
            (const co_http2_header_field_t*)data->value;

        co_log_write_header(
            level, CO_LOG_CATEGORY_HTTP2);

        fprintf(fp,
            "%s: %s\n", field->name, field->value);
    }

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP2);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    fflush(fp);

    co_mutex_unlock(log->mutex);
}

void
co_http2_log_write_frame(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const co_http2_frame_t* frame,
    const char* format,
    ...
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->category[CO_LOG_CATEGORY_HTTP2].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP2);

    co_net_log_write_addresses(
        log, CO_LOG_CATEGORY_HTTP2, addr1, text, addr2);

    FILE* fp =
        (FILE*)log->category[CO_LOG_CATEGORY_HTTP2].output;

    va_list args;
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);

    fprintf(fp, "\n");

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP2);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    char info[256] = { 0 };
    char type[32] = { 0 };

    switch (frame->header.type)
    {
    case CO_HTTP2_FRAME_TYPE_DATA:
    {
        sprintf(type, "DATA(%u)", frame->header.type);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_HEADERS:
    {
        sprintf(type, "HEADERS(%u)", frame->header.type);

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PRIORITY)
        {
            sprintf(info, "dep_id:%u, weight:%u",
                frame->payload.headers.stream_dependency,
                frame->payload.headers.weight);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PRIORITY:
    {
        sprintf(type, "PRIORITY(%u)", frame->header.type);

        sprintf(info, "dep_id:%u, weight:%u",
            frame->payload.priority.stream_dependency,
            frame->payload.priority.weight);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_RST_STREAM:
    {
        sprintf(type, "RST(%u)", frame->header.type);

        sprintf(info, "error:%u",
            frame->payload.rst_stream.error_code);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_SETTINGS:
    {
        sprintf(type, "SETTINGS(%u)", frame->header.type);

        for (size_t index = 0; index < frame->payload.settings.param_count; ++index)
        {
            char setting[32];
            sprintf(setting, "%u:%u ",
                frame->payload.settings.params[index].id,
                frame->payload.settings.params[index].value);

            strcat(info, setting);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PUSH_PROMISE:
    {
        sprintf(type, "PUSH-PROMISE(%u)", frame->header.type);

        sprintf(info, "promised_id:%u",
            frame->payload.push_promise.promised_stream_id);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PING:
    {
        sprintf(type, "PING(%u)", frame->header.type);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_GOAWAY:
    {
        sprintf(type, "GOAWAY(%u)", frame->header.type);

        sprintf(info, "last_id:%u error:%u",
            frame->payload.goaway.last_stream_id,
            frame->payload.goaway.error_code);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE:
    {
        sprintf(type, "WINDOW-UPDATE(%u)", frame->header.type);

        sprintf(info, "size_inc:%u",
            frame->payload.window_update.window_size_increment);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_CONTINUATION:
    {
        sprintf(type, "CONTINUATION(%u)", frame->header.type);

        break;
    }
    default:
    {
        sprintf(type, "\?\?\?\?(%u)", frame->header.type);

        break;
    }
    }

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP2);

    fprintf(fp,
        "stream-%u %s flags:0x%02x length:%u %s\n",
        frame->header.stream_id, type,
        frame->header.flags, frame->header.length, info);

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP2);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    fflush(fp);

    co_mutex_unlock(log->mutex);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_http2_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_LOG_CATEGORY_HTTP2, level);

    co_log_add_category(
        CO_LOG_CATEGORY_HTTP2,
        CO_LOG_CATEGORY_NAME_HTTP2);
}
