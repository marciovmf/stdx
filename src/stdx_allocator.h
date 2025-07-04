/*
 * STDX - Allocator
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a custom memory allocator interface.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_ALLOCATOR
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "stdx_allocator.h"
 */

#ifndef STDX_ALLOCATOR_H
#define STDX_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define STDX_ALLOCATOR_VERSION_MAJOR 1
#define STDX_ALLOCATOR_VERSION_MINOR 0
#define STDX_ALLOCATOR_VERSION_PATCH 0


#define STDX_ALLOCATOR_VERSION (STDX_ALLOCATOR_VERSION_MAJOR * 10000 + STDX_ALLOCATOR_VERSION_MINOR * 100 + STDX_ALLOCATOR_VERSION_PATCH)

#include <stddef.h>

typedef struct XAllocator_t
{
  void* (*alloc)(struct XAllocator_t* self, size_t size);
  void  (*free)(struct XAllocator_t* self, void* ptr);
  void* userdata;
} XAllocator;

// Default allocator using malloc/free
extern XAllocator stdx_default_allocator;

// Helpers
void* stdx_alloc(XAllocator* a, size_t size);
void  stdx_free(XAllocator* a, void* ptr);

#ifdef __cplusplus
}
#endif

#ifdef STDX_IMPLEMENTATION_ALLOCATOR

#include <stdlib.h>

static void* stdx_malloc_wrapper(XAllocator* self, size_t size)
{
  (void)self; return malloc(size);
}

static void stdx_free_wrapper(XAllocator* self, void* ptr)
{
  (void)self; free(ptr);
}

XAllocator stdx_default_allocator =
{
  .alloc = stdx_malloc_wrapper,
  .free = stdx_free_wrapper,
  .userdata = NULL
};

void* stdx_alloc(XAllocator* a, size_t size) {
  return (a ? a->alloc : stdx_default_allocator.alloc)(a, size);
}

void stdx_free(XAllocator* a, void* ptr) {
  (a ? a->free : stdx_default_allocator.free)(a, ptr);
}

#endif // STDX_IMPLEMENTATION_ALLOCATOR
#endif // STDX_ALLOCATOR_H
