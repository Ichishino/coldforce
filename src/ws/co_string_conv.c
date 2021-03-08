#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/ws/co_string_conv.h>

#ifdef CO_OS_WIN
#include <windows.h>
#else
#include <iconv.h>
#endif

//---------------------------------------------------------------------------//
// string conv
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef CO_OS_WIN
static size_t
co_win_string_conv_mbs_to_mbs(
    int src_cp,
    const char* src,
    size_t src_length,
    int dest_cp,
    char* dest_buff,
    size_t dest_buff_size
)
{
    if ((dest_buff == NULL) || (dest_buff_size <= 1))
    {
        return 0;
    }

    dest_buff[0] = '\0';

    int wstr_length = MultiByteToWideChar(
        src_cp, 0, src, (int)src_length, NULL, 0);

    if (wstr_length == 0)
    {
        return 0;
    }

    wchar_t* wstr =
        (wchar_t*)co_mem_alloc(
            (size_t)wstr_length * sizeof(wchar_t));

    if (wstr == NULL)
    {
        return 0;
    }

    if (MultiByteToWideChar(src_cp, 0,
        src, (int)src_length, wstr, wstr_length) == 0)
    {
        co_mem_free(wstr);

        return 0;
    }

    int result = WideCharToMultiByte(dest_cp, 0,
        wstr, wstr_length, dest_buff, (int)(dest_buff_size - 1),
        NULL, NULL);

    co_mem_free(wstr);

    dest_buff[result] = '\0';

    return (size_t)result;
}
#else
static size_t
co_string_conv(
    const char* src_cp,
    const char* src,
    size_t src_length,
    const char* dest_cp,
    char* dest_buffer,
    size_t dest_buffer_size
)
{
    if ((dest_buffer == NULL) || (dest_buffer_size <= 1))
    {
        return 0;
    }

    char* src_dup = co_string_duplicate_n(src, src_length);

    if (src_dup == NULL)
    {
        return 0;
    }

    char* src_temp = src_dup;
    char* dest_temp = dest_buffer;
    --dest_buffer_size;

    iconv_t conv = iconv_open(dest_cp, src_cp);

    while (src_length > 0)
    {
        if (iconv(conv,
             &src_temp, &src_length,
             &dest_temp, &dest_buffer_size) == -1)
        {
            break;
        }
    }

    *dest_temp = '\0';

    iconv_close(conv);
    co_string_destroy(src_dup);

    return (size_t)(dest_temp - dest_buffer);
}
#endif

size_t
co_string_conv_utf8_to_sjis(
    const char* src,
    size_t src_length,
    char* dest_buffer,
    size_t dest_buffer_size
)
{
#ifdef CO_OS_WIN
    return co_win_string_conv_mbs_to_mbs(
        CP_UTF8, src, src_length, 932, dest_buffer, dest_buffer_size);
#else
    return co_string_conv(
        "UTF-8", src, src_length, "CP932", dest_buffer, dest_buffer_size);
#endif
}

size_t
co_string_conv_sjis_to_utf8(
    const char* src,
    size_t src_length,
    char* dest_buffer,
    size_t dest_buffer_size
)
{
#ifdef CO_OS_WIN
    return co_win_string_conv_mbs_to_mbs(
        932, src, src_length, CP_UTF8, dest_buffer, dest_buffer_size);
#else
    return co_string_conv(
        "CP932", src, src_length, "UTF-8", dest_buffer, dest_buffer_size);
#endif
}
