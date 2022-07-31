#ifndef CO_STRING_H_INCLUDED
#define CO_STRING_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// string
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
size_t
co_string_hash(
    const char* str
);

CO_CORE_API
size_t
co_string_trim_left(
    char* str,
    size_t length
);

CO_CORE_API
size_t
co_string_trim_right(
    char* str,
    size_t length
);

CO_CORE_API
size_t
co_string_trim(
    char* str,
    size_t length
);

CO_CORE_API
char*
co_string_wrap_quotes(
    const char* str,
    bool double_or_single
);

CO_CORE_API
void
co_string_trim_quotes(
    char* str
);

CO_CORE_API
char*
co_string_duplicate(
    const char* str
);

CO_CORE_API
char*
co_string_duplicate_n(
    const char* str,
    size_t length
);

CO_CORE_API
char*
co_string_find_n(
    const char* str1,
    const char* str2,
    size_t length
);

CO_CORE_API
void
co_string_hex(
    const void* binary,
    size_t size,
    char* buffer,
    bool uppercase
);

#define co_string_destroy   co_mem_free

#if (SIZE_MAX == UINT64_MAX)
#   define co_string_to_size_t(str, ep, rad) strtoull(str, ep, rad)
#elif (SIZE_MAX == UINT32_MAX)
#   define co_string_to_size_t(str, ep, rad) strtoul(str, ep, rad)
#endif

#ifdef CO_OS_WIN
#   define co_string_case_compare   _stricmp
#   define co_string_case_compare_n _strnicmp
#else
#   define co_string_case_compare   strcasecmp
#   define co_string_case_compare_n strncasecmp
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_H_INCLUDED
