/* Capstone Disassembly Engine */
/* By Satoshi Tanda <tanda.sat@gmail.com>, 2016 */
/* Modified for comprehensive Windows Kernel Mode support */

#ifndef CS_WINDOWS_WINKERNEL_MM_H
#define CS_WINDOWS_WINKERNEL_MM_H

#if defined(_KERNEL_MODE) || defined(CAPSTONE_WINKERNEL)

#include <ntddk.h>
#include <wdm.h>
#include <stdarg.h>

// ============================================
// Integer types (replace stdint.h/inttypes.h)
// ============================================
#ifndef _STDINT_TYPES_DEFINED
#define _STDINT_TYPES_DEFINED
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef unsigned __int64  uint64_t;
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef signed __int64    int64_t;
typedef ULONG_PTR         uintptr_t;
typedef LONG_PTR          intptr_t;
#endif
#ifndef INT8_MIN
#define INT8_MIN         (-127i8 - 1)
#define INT16_MIN        (-32767i16 - 1)
#define INT32_MIN        (-2147483647i32 - 1)
#define INT64_MIN        (-9223372036854775807i64 - 1)
#define INT8_MAX         127i8
#define INT16_MAX        32767i16
#define INT32_MAX        2147483647i32
#define INT64_MAX        9223372036854775807i64
#define UINT8_MAX        0xffui8
#define UINT16_MAX       0xffffui16
#define UINT32_MAX       0xffffffffui32
#define UINT64_MAX       0xffffffffffffffffui64
#endif
#define CS_WINKERNEL_TYPES_DEFINED 1

#ifndef _PTRDIFF_T_DEFINED
typedef LONG_PTR ptrdiff_t;
#define _PTRDIFF_T_DEFINED
#endif

// ============================================
// Boolean type (replace stdbool.h)
// ============================================
#ifndef __cplusplus
#ifndef _BOOL_DEFINED
#define _BOOL_DEFINED
typedef unsigned char bool;
#define false 0
#define true 1
#endif
#endif

// ============================================
// CAPSTONE_API definition (needed before including capstone.h)
// ============================================
#ifndef CAPSTONE_API
#ifdef _MSC_VER
#define CAPSTONE_API __cdecl
#else
#define CAPSTONE_API
#endif
#endif

// ============================================
// Memory allocation functions (replace stdlib.h)
// ============================================

#ifdef __cplusplus
extern "C" {
#endif

void CAPSTONE_API cs_winkernel_free(void *ptr);
void * CAPSTONE_API cs_winkernel_malloc(size_t size);
void * CAPSTONE_API cs_winkernel_calloc(size_t n, size_t size);
void * CAPSTONE_API cs_winkernel_realloc(void *ptr, size_t size);
int CAPSTONE_API cs_winkernel_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);
typedef int (__cdecl *cs_winkernel_qsort_compare_t)(const void *a, const void *b);
void CAPSTONE_API cs_winkernel_qsort(void *base, size_t num, size_t size, cs_winkernel_qsort_compare_t cmp);

#ifdef __cplusplus
}
#endif

// Macros to replace standard functions (for direct usage in code)
#ifndef CS_WINKERNEL_NO_MACROS
#define malloc(size)        cs_winkernel_malloc(size)
#define calloc(n, size)     cs_winkernel_calloc(n, size)
#define realloc(ptr, size)  cs_winkernel_realloc(ptr, size)
#define free(ptr)           cs_winkernel_free(ptr)
#define vsnprintf           cs_winkernel_vsnprintf
#define qsort               cs_winkernel_qsort
#endif

// kern_os_* functions (used by Capstone for OSX, need to define for Windows)
#define kern_os_malloc(size)        cs_winkernel_malloc(size)
#define kern_os_calloc(n, size)     cs_winkernel_calloc(n, size)
#define kern_os_realloc(ptr, size)  cs_winkernel_realloc(ptr, size)
#define kern_os_free(ptr)           cs_winkernel_free(ptr)

// ============================================
// String functions (replace string.h)
// ============================================
// Note: This is a simplified memcmp for kernel mode
// It returns 0 if equal, non-zero otherwise (unlike standard memcmp which returns -1, 0, or 1)
// This is sufficient for Capstone's usage which typically only checks for equality
static __inline int cs_memcmp(const void *p1, const void *p2, size_t size) {
    return RtlEqualMemory(p1, p2, size) ? 0 : 1;
}

#ifndef CS_WINKERNEL_NO_MACROS
#define memcpy(dst, src, size)      RtlCopyMemory(dst, src, size)
#define memmove(dst, src, size)     RtlMoveMemory(dst, src, size)
#define memset(dst, val, size)      RtlFillMemory(dst, size, val)
#define memcmp(p1, p2, size)        cs_memcmp(p1, p2, size)
#endif

// strlen for kernel (assuming ANSI strings, not wide strings)
static __inline size_t cs_strlen(const char *str) {
    const char *s = str;
    while (*s) s++;
    return (size_t)(s - str);
}

#ifndef CS_WINKERNEL_NO_MACROS
#define strlen(str)                 cs_strlen(str)
#endif

// ============================================
// I/O functions (replace stdio.h)
// ============================================
// WARNING: DbgPrint has different behavior than printf:
// - It's only available in checked builds or with debugger attached
// - Output goes to debugger, not console
// - Format specifiers may differ from standard printf
// These mappings are for basic debugging only
#ifndef CS_WINKERNEL_NO_MACROS
#define printf                      DbgPrint
#define snprintf                    _snprintf
#endif

// ============================================
// Other replacements
// ============================================
#ifndef CS_WINKERNEL_NO_MACROS
#define assert(x)                   ((void)0)
#endif

// Limits (replace limits.h)
#ifndef INT_MIN
#define INT_MIN (-2147483647 - 1)
#endif

#ifndef LONG_MIN
#define LONG_MIN (-2147483647L - 1)
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#ifndef UINT_MAX
#define UINT_MAX 0xffffffffU
#endif

// offsetof (replace stddef.h)
#ifndef offsetof
#define offsetof(type, member) ((size_t)(&(((type *)0)->member)))
#endif

#endif // defined(_KERNEL_MODE) || defined(CAPSTONE_WINKERNEL)

#endif  // CS_WINDOWS_WINKERNEL_MM_H
