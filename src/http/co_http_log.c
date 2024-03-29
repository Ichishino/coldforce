#include <coldforce/core/co_std.h>

#include <coldforce/http/co_http_log.h>

//---------------------------------------------------------------------------//
// http log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static void
co_http_log_write_header(
    co_log_t* log,
    int level,
    const co_http_header_t* header
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_next(header->field_list, &it);
        const co_http_header_field_t* field =
            (const co_http_header_field_t*)data->value;

        co_log_write_header(
            level, CO_LOG_CATEGORY_HTTP);

        fprintf((FILE*)log->category[CO_LOG_CATEGORY_HTTP].output,
            "%s: %s\n", field->name, field->value);
    }
}

void
co_http_log_write_request_header(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const co_http_request_t* request,
    const char* format,
    ...
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->category[CO_LOG_CATEGORY_HTTP].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);

    co_net_log_write_addresses(
        log, CO_LOG_CATEGORY_HTTP, addr1, text, addr2);

    FILE* fp =
        (FILE*)log->category[CO_LOG_CATEGORY_HTTP].output;

    va_list args;
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);

    fprintf(fp, "\n");

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);
    fprintf(fp, "%s %s %s\n",
        co_http_request_get_method(request),
        co_http_request_get_url(request)->src,
        co_http_request_get_version(request));

    co_http_log_write_header(
        log, level,
        co_http_request_get_const_header(request));

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    fflush(fp);

    co_mutex_unlock(log->mutex);
}

void
co_http_log_write_response_header(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const co_http_response_t* response,
    const char* format,
    ...
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->category[CO_LOG_CATEGORY_HTTP].level)
    {
        return;
    }

    co_mutex_lock(log->mutex);

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);

    co_net_log_write_addresses(
        log, CO_LOG_CATEGORY_HTTP, addr1, text, addr2);

    FILE* fp =
        (FILE*)log->category[CO_LOG_CATEGORY_HTTP].output;

    va_list args;
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);

    fprintf(fp, "\n");

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);
    fprintf(fp, "%s %d %s\n",
        co_http_response_get_version(response),
        co_http_response_get_status_code(response),
        co_http_response_get_reason_phrase(response));

    co_http_log_write_header(
        log, level,
        co_http_response_get_const_header(response));

    co_log_write_header(
        level, CO_LOG_CATEGORY_HTTP);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    fflush(fp);

    co_mutex_unlock(log->mutex);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_http_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_LOG_CATEGORY_HTTP, level);

    co_log_add_category(
        CO_LOG_CATEGORY_HTTP,
        CO_LOG_CATEGORY_NAME_HTTP);
}
