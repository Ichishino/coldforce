#include <coldforce/core/co_std.h>

#include <coldforce/http/co_http_config.h>

//---------------------------------------------------------------------------//
// http config
//---------------------------------------------------------------------------//

static co_http_config_t http_config =
{
    CO_HTTP_CONFIG_DEFAULT_MAX_RECEIVE_HEADER_LINE_SIZE,
    CO_HTTP_CONFIG_DEFAULT_MAX_RECEIVE_HEADER_FIELD_COUNT,
    CO_HTTP_CONFIG_DEFAULT_MAX_RECEIVE_CONTENT_SIZE
};

void
co_http_config_set_max_receive_header_line_size(
    size_t max_receive_header_line_size
)
{
    http_config.max_receive_header_line_size = max_receive_header_line_size;
}

size_t
co_http_config_get_max_receive_header_line_size(
    void
)
{
    return http_config.max_receive_header_line_size;
}

void
co_http_config_set_max_receive_header_field_count(
    size_t max_receive_header_field_count
)
{
    http_config.max_receive_header_field_count = max_receive_header_field_count;
}

size_t
co_http_config_get_max_receive_header_field_count(
    void
)
{
    return http_config.max_receive_header_field_count;
}

void
co_http_config_set_max_receive_content_size(
    size_t max_receive_content_size
)
{
    http_config.max_receive_content_size = max_receive_content_size;
}

size_t
co_http_config_get_max_receive_content_size(
    void
)
{
    return http_config.max_receive_content_size;
}
