#ifndef _COLDFORCE_STD_H_
#define _COLDFORCE_STD_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <crtdbg.h>
#endif

//---------------------------------------------------------------------------//
// Platform
//---------------------------------------------------------------------------//

// TODO
#if defined(_WIN32) && defined(_MSC_VER)
#   define CO_OS_WIN
#elif defined(linux) || defined(__linux__)
#   define CO_OS_LINUX
#elif defined(__APPLE__)
#   define CO_OS_MAC
#else
#   error "Unknown platform"
#endif

// TODO
#ifdef CO_OS_MAC
#   include <machine/endian.h>
#elif defined(__GNUC__)
#   include <endian.h>
#endif
#ifdef __BYTE_ORDER
#   if defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#       define CO_LITTLE_ENDIAN
#   endif
#elif defined(_BYTE_ORDER)
#   if defined(_LITTLE_ENDIAN) && (_BYTE_ORDER == _LITTLE_ENDIAN)
#       define CO_LITTLE_ENDIAN
#   endif
#elif defined(__BYTE_ORDER__)
#   if defined(__ORDER_LITTLE_ENDIAN__) && \
              (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#       define CO_LITTLE_ENDIAN
#   endif
#elif defined(__LITTLE_ENDIAN__) || \
      defined(__ARMEL__) ||  \
      defined(__THUMBEL__) ||  \
      defined(__AARCH64EL__) ||  \
      defined(_MIPSEL) ||  \
      defined(__MIPSEL) ||  \
      defined(__MIPSEL__)
#   define CO_LITTLE_ENDIAN
#elif defined(CO_OS_WIN)
#   define CO_LITTLE_ENDIAN
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#define CO_DEBUG
#endif

#ifdef _MSC_VER
    #ifdef CO_WIN_EXPORT
        #define CO_API  __declspec(dllexport)
    #else
        #define CO_API
    #endif
    #define CO_THREAD_LOCAL __declspec(thread)
#elif defined(CO_OS_LINUX)
    #define CO_API
    #define CO_THREAD_LOCAL __thread
#endif

#ifdef CO_OS_WIN
typedef void* CO_WIN_HANDLE_T;
#else
#endif

//---------------------------------------------------------------------------//
// Macro
//---------------------------------------------------------------------------//

#ifdef __cplusplus
    #define CO_EXTERN_C_BEGIN   extern "C" {
    #define CO_EXTERN_C_END     }
#else
    #define CO_EXTERN_C_BEGIN
    #define CO_EXTERN_C_END
#endif

#define CO_EVENT_ID_STOP    0x7001
#define CO_EVENT_ID_TIMER   0x7002

#define CO_INFINITE         0xFFFFFFFF

#define CO_Assert           assert

typedef enum
{
    CO_WAIT_RESULT_SUCCESS = 0,
    CO_WAIT_RESULT_TIMEOUT = 1,
    CO_WAIT_RESULT_CANCEL = 2,
    CO_WAIT_RESULT_ERROR = -1

} CO_WAIT_RESULT_EN;

//---------------------------------------------------------------------------//
// Utility
//---------------------------------------------------------------------------//

#ifdef CO_OS_WIN
    #ifdef CO_DEBUG
        #define CO_MemAlloc(len) \
           _malloc_dbg(len, _NORMAL_BLOCK, __FILE__, __LINE__)
    #else
        #define CO_MemAlloc    malloc
    #endif
#else
    #define CO_MemAlloc    malloc
#endif

#define CO_MemFree  free

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_BEGIN

void CO_MemClear(void* ptr, size_t len);
void CO_MemCopy(void* to, const void* from, size_t len);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // _COLDFORCE_STD_H_
