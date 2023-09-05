#ifndef CO_H_INCLUDED
#define CO_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

//---------------------------------------------------------------------------//
// platform
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
#   define CO_DEBUG
#endif

#ifdef _MSC_VER
#   ifdef CO_CORE_EXPORTS
#       define CO_CORE_API  __declspec(dllexport)
#   else
#       define CO_CORE_API
#   endif
#   define CO_THREAD_LOCAL __declspec(thread)
#else
#   define CO_CORE_API
#   define CO_THREAD_LOCAL __thread
#endif

#ifdef __cplusplus
#   define CO_EXTERN_C_BEGIN   extern "C" {
#   define CO_EXTERN_C_END     }
#else
#   define CO_EXTERN_C_BEGIN
#   define CO_EXTERN_C_END
#endif

#ifndef CO_OS_WIN
#include <unistd.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// constants, types
//---------------------------------------------------------------------------//

#ifdef CO_OS_WIN
#ifndef HAVE_SSIZE_T
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#define HAVE_SSIZE_T
#endif // !HAVE_SSIZE_T
#endif // CO_OS_WIN

#define CO_INFINITE     0xFFFFFFFF

typedef enum
{
    CO_WAIT_RESULT_SUCCESS = 0,
    CO_WAIT_RESULT_TIMEOUT = 1,
    CO_WAIT_RESULT_CANCEL = 2,
    CO_WAIT_RESULT_ERROR = -1

} co_wait_result_t;

typedef size_t(*co_item_hash_fn)(const void* data);
typedef void(*co_item_destroy_fn)(void* data);
typedef void*(*co_item_duplicate_fn)(const void* src);
typedef int(*co_item_compare_fn)(const void* data1, const void* data2);

typedef struct
{
    void* ptr;
    size_t size;

} co_buffer_st;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_CORE_API
void
co_mem_free_later(
    void* mem
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

#define co_mem_alloc        malloc
#define co_mem_realloc      realloc
#define co_mem_free         free
#define co_assert           assert
#define co_max(l, r)        (((l) > (r)) ? (l) : (r))
#define co_min(l, r)        (((l) < (r)) ? (l) : (r))

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_H_INCLUDED
