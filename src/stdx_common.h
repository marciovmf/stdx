/*
 * STDX - Common
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides portable macros and types for different taks such as os detection,
 * architecture detection, compiler detection, bit manipulation, dll
 * export/import macros, assertions and more.
 *
 * Header-only and modular. Designed for performance and flexibility.
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "stdx_common.h"
 */

#ifndef STDX_COMMON_H
#define STDX_COMMON_H

#ifdef __cplusplus
extern "C" {
#include <stdbool.h>
#endif


#define STDX_COMMON_VERSION_MAJOR 1
#define STDX_COMMON_VERSION_MINOR 0
#define STDX_COMMON_VERSION_PATCH 0

#define STDX_COMMON_VERSION (STDX_COMMON_VERSION_MAJOR * 10000 + STDX_COMMON_VERSION_MINOR * 100 + STDX_COMMON_VERSION_PATCH)

  // -----------------------------------------------------------------------------
  // Compiler Detection macros
  // -----------------------------------------------------------------------------

#if defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_NAME "GCC"
#define COMPILER_VERSION __VERSION__
#define COMPILER_GCC
#elif defined(_MSC_VER)
#define COMPILER_NAME "Microsoft Visual C/C++ Compiler"
#define COMPILER_VERSION _MSC_FULL_VER
#define COMPILER_MSVC
#elif defined(__clang__)
#define COMPILER_NAME "Clang"
#define COMPILER_VERSION __clang_version__
#define COMPILER_CLANG
#else
#define COMPILER_NAME "Unknown"
#define COMPILER_VERSION "Unknown"
#define COMPILER_UNKNOWN
#endif

  // -----------------------------------------------------------------------------
  //  Compiler ATTRIBUTES / INLINE / ALIGN
  // -----------------------------------------------------------------------------
  //
#if defined(COMPILER_MSVC)
#define INLINE      __inline
#define FORCEINLINE __forceinline
#define NOINLINE    __declspec(noinline)
#define NORETURN    __declspec(noreturn)
#define ALIGN(n)    __declspec(align(n))
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define INLINE      inline
#define FORCEINLINE inline __attribute__((always_inline))
#define NOINLINE    __attribute__((noinline))
#define NORETURN    __attribute__((noreturn))
#define ALIGN(n)    __attribute__((aligned(n)))
#else
#define INLINE
#define FORCEINLINE
#define NOINLINE
#define NORETURN
#define ALIGN(n)
#endif

  // ----------------------------------------------------------------------------
  // Branch prediction
  // ----------------------------------------------------------------------------
#if defined(PLAT_COMPILER_GCC) || defined(PLAT_COMPILER_CLANG)
#define PLAT_LIKELY(x)   __builtin_expect(!!(x), 1)
#define PLAT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define PLAT_LIKELY(x)   (x)
#define PLAT_UNLIKELY(x) (x)
#endif


  // ----------------------------------------------------------------------------
  // Architecture detection
  // ----------------------------------------------------------------------------
#if defined(_M_X64) || defined(__x86_64__)
#define PLAT_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define PLAT_ARCH_X86 1
#elif defined(__aarch64__)
#define PLAT_ARCH_ARM64 1
#elif defined(__arm__)
#define PLAT_ARCH_ARM32 1
#else
#define PLAT_ARCH_UNKNOWN 1
#endif


  // -----------------------------------------------------------------------------
  // OS Detection macros
  // -----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#define OS_WINDOWS 1
#elif defined(__linux__)
#define OS_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_OSX
#else
#define OS_UNKNOWN
#endif


  // ----------------------------------------------------------------------------
  // DLL import/export
  // ----------------------------------------------------------------------------
#if defined(PLAT_OS_WINDOWS)
#define PLAT_EXPORT __declspec(dllexport)
#define PLAT_IMPORT __declspec(dllimport)
#else
#define PLAT_EXPORT __attribute__((visibility("default")))
#define PLAT_IMPORT
#endif


#if defined(_DEBUG)
#ifndef DEBUG
#define DEBUG
#endif
#endif


  // ----------------------------------------------------------------------------
  // Assertion macros
  // ----------------------------------------------------------------------------

#define STATIC_ASSERT(cond, msg) \
  typedef char static_assert_##msg[(cond) ? 1 : -1]


#ifdef DEBUG
#define ASSERT(expr) \
  do { \
    if (!(expr)) { \
      fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #expr, __FILE__, __LINE__); \
      abort(); \
    } \
  } while(0)
#else
#define ASSERT(expr) ((void)0)
#endif

  // ----------------------------------------------------------------------------
  // Endianness
  // ----------------------------------------------------------------------------
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PLAT_IS_BIG_ENDIAN 1
#else
#define PLAT_IS_LITTLE_ENDIAN 1
#endif

  // ----------------------------------------------------------------------------
  // Path separator
  // ----------------------------------------------------------------------------
#if defined(PLAT_OS_WINDOWS)
#define PLAT_PATH_SEPARATOR '\\'
#else
#define PLAT_PATH_SEPARATOR '/'
#endif


  // -----------------------------------------------------------------------------
  // Bit manipulation macros
  // -----------------------------------------------------------------------------
#define STDMIN_BIT_SET(var, bit)    ((var) |=  (1U << (bit)))
#define STDMIN_BIT_CLEAR(var, bit)  ((var) &= ~(1U << (bit)))
#define STDMIN_BIT_TOGGLE(var, bit) ((var) ^=  (1U << (bit)))
#define STDMIN_BIT_CHECK(var, bit)  (((var) &  (1U << (bit))) != 0)


#define UNUSED(x) (void)(x) // mark a function argument as unused

  // -----------------------------------------------------------------------------
  // Common types
  // -----------------------------------------------------------------------------

#if defined(OS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 

#ifndef _CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <windows.h>
#endif

#include <stdint.h>

  typedef int8_t    i8;
  typedef uint8_t   u8;
  typedef uint8_t   byte;
  typedef int16_t   i16;
  typedef uint16_t  u16;
  typedef int32_t   i32;
  typedef uint32_t  u32;
  typedef int64_t   i64;
  typedef uint64_t  u64;

  /* File reading */

#ifdef __cplusplus
}
#endif

#endif //STDX_COMMON_H

