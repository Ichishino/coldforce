#ifndef CO_DEBUG_H_INCLUDED
#define CO_DEBUG_H_INCLUDED

//---------------------------------------------------------------------------//
// debug tools
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#if defined(CO_OS_WIN) && defined(CO_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define co_win_debug_crt_set_flags() \
    _CrtSetDbgFlag( \
        _CRTDBG_ALLOC_MEM_DF | \
        _CRTDBG_CHECK_ALWAYS_DF | \
        _CRTDBG_LEAK_CHECK_DF)
#else
#define co_win_debug_crt_set_flags() ((void)0)
#endif // CO_OS_WIN && CO_DEBUG

#ifdef CO_DEBUG

#include <stdlib.h>

inline void*
co_mem_alloc_dbg(
    size_t size,
    const char* file,
    int line
)
{
#ifdef CO_OS_WIN
    return _malloc_dbg(size, _NORMAL_BLOCK, file, line);
#else
    (void)file;
    (void)line;
    return malloc(size);
#endif // CO_OS_WIN
}

inline void*
co_mem_realloc_dbg(
    void* ptr,
    size_t size,
    const char* file,
    int line
)
{
#ifdef CO_OS_WIN
    return _realloc_dbg(ptr, size, _NORMAL_BLOCK, file, line);
#else
    (void)file;
    (void)line;
    return realloc(ptr, size);
#endif // CO_OS_WIN
}

inline void
co_mem_free_dbg(
    void* ptr,
    const char* file,
    int line
)
{
    (void)file;
    (void)line;

#ifdef CO_OS_WIN
    _free_dbg(ptr, _NORMAL_BLOCK);
#else
    free(ptr);
#endif // CO_OS_WIN
}

#endif // CO_DEBUG

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#endif // CO_DEBUG_H_INCLUDED
