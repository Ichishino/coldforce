#ifndef CO_STRING_H_INCLUDED
#define CO_STRING_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// string
//---------------------------------------------------------------------------//

CO_API size_t co_string_trim_left(char* str, size_t length);
CO_API size_t co_string_trim_right(char* str, size_t length);
CO_API size_t co_string_trim(char* str, size_t length);

CO_API size_t co_string_hash(const char* str);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_H_INCLUDED
