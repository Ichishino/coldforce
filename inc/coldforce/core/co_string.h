#ifndef CO_STRING_H_INCLUDED
#define CO_STRING_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// string
//---------------------------------------------------------------------------//

CO_API size_t co_string_hash(const char* str);

CO_API size_t co_string_trim_left(char* str, size_t length);
CO_API size_t co_string_trim_right(char* str, size_t length);
CO_API size_t co_string_trim(char* str, size_t length);

CO_API char* co_string_duplicate(const char* str);
CO_API char* co_string_duplicate_n(const char* str, size_t length);

CO_API char* co_string_find_n(const char* str1, const char* str2, size_t length);

#define co_string_destroy   co_mem_free

#if (SIZE_MAX == UINT64_MAX)
#   define co_string_to_size_t(str, ep, rad) strtoull(str, ep, rad)
#elif (SIZE_MAX == UINT32_MAX)
#   define co_string_to_size_t(str, ep, rad) strtoul(str, ep, rad)
#endif

#ifdef CO_OS_WIN
#   define co_string_case_compare   stricmp
#   define co_string_case_compare_n strnicmp
#else
#   define co_string_case_compare   strcasecmp
#   define co_string_case_compare_n strncasecmp
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_H_INCLUDED
