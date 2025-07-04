/*
 * STDX - Arena Allocator
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a high-performance memory arena allocator for fast,
 * linear allocation and bulk deallocation via reset. Suitable for 
 * transient or frame-based memory usage patterns.
 *
 * Integrates with the generic Allocator interface from <stdx/allocator.h>,
 * allowing seamless use in allocator-aware systems.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_ARENA
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Dependencies: stdx_allocator.h
 * Usage: #include "stdx_arena.h"
 */

#ifndef STDX_ARENA_H
#define STDX_ARENA_H

#ifdef __cplusplus
extern "C"
{
#endif

#define STDX_ARENA_VERSION_MAJOR 1
#define STDX_ARENA_VERSION_MINOR 0
#define STDX_ARENA_VERSION_PATCH 0

#define STDX_ARENA_VERSION (STDX_ARENA_VERSION_MAJOR * 10000 + STDX_ARENA_VERSION_MINOR * 100 + STDX_ARENA_VERSION_PATCH)


#ifdef STDX_IMPLEMENTATION_ARENA
#ifndef STDX_IMPLEMENTATION_ALLOCATOR
#define STDX_INTERNAL_ALLOCATOR_IMPLEMENTATION
#define STDX_IMPLEMENTATION_ALLOCATOR
#endif
#endif
#include <stdx_allocator.h>

#include <stdint.h>

  typedef struct XArenaChunk_t XArenaChunk;
  typedef struct XArena_t  XArena;

  XArena*  x_arena_create(size_t chunk_size); // Create a new arena 
  void*   x_arena_alloc(XArena* arena, size_t size); // Allocate memory from the aren
  void    x_arena_reset(XArena* arena); // Reset the arena (free allocations, keep chunks)
  void    x_arena_destroy(XArena* arena); // Free all chunks and destroy the arena

  // XArena backed allocator 
  void* x_arena_alloc_wrapper(XAllocator* self, size_t size);
  XAllocator x_arena_allocator(XArena* arena);
  XAllocator x_arena_allocator(XArena* arena);

#ifdef __cplusplus
}
#endif

#ifdef STDX_IMPLEMENTATION_ARENA

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

  struct XArenaChunk_t
  {
    struct XArenaChunk_t* next;
    size_t capacity;
    size_t used;
    uint8_t* data;
  };

  struct XArena_t
  {
    size_t chunk_size;
    XArenaChunk* chunks;
  };

  static XArenaChunk* x_arena_chunk_create(size_t size)
  {
    XArenaChunk* chunk = (XArenaChunk*) malloc(sizeof(XArenaChunk));
    if (!chunk) return NULL;
    chunk->capacity = size;
    chunk->used = 0;
    chunk->data = (uint8_t*) malloc(size);
    chunk->next = NULL;
    return chunk->data ? chunk : (free(chunk), NULL);
  }

  XArena* x_arena_create(size_t chunk_size)
  {
    XArena* arena = (XArena*) malloc(sizeof(XArena));
    if (!arena) return NULL;
    arena->chunk_size = chunk_size;
    arena->chunks = x_arena_chunk_create(chunk_size);
    if (!arena->chunks)
    { free(arena); return NULL; }
    return arena;
  }

  void* x_arena_alloc(XArena* arena, size_t size)
  {
    if (size == 0 || !arena) return NULL;

    // Try to find a chunk with enough space
    for (XArenaChunk* chunk = arena->chunks; chunk != NULL; chunk = chunk->next)
    {
      size_t remaining = chunk->capacity - chunk->used;
      if (remaining >= size)
      {
        void* ptr = chunk->data + chunk->used;
        chunk->used += size;
        return ptr;
      }
    }

    // No chunk had enough space, create a new one
    size_t chunk_size = (size > arena->chunk_size) ? size : arena->chunk_size;
    XArenaChunk* new_chunk = x_arena_chunk_create(chunk_size);
    if (!new_chunk) return NULL;

    new_chunk->used = size;
    new_chunk->next = arena->chunks;
    arena->chunks = new_chunk;
    return new_chunk->data;
  }

  void x_arena_reset(XArena* arena)
  {
    if (!arena) return;
    for (XArenaChunk* chunk = arena->chunks; chunk != NULL; chunk = chunk->next)
    {
      chunk->used = 0;
    }
  }

  void x_arena_destroy(XArena* arena)
  {
    if (!arena) return;
    XArenaChunk* chunk = arena->chunks;
    while (chunk)
    {
      XArenaChunk* next = chunk->next;
      free(chunk->data);
      free(chunk);
      chunk = next;
    }
    free(arena);
  }

  void* x_arena_alloc_wrapper(XAllocator* self, size_t size)
  {
    return x_arena_alloc((XArena*) self->userdata, size);
  }

  void x_arena_free_noop(XAllocator* self, void* ptr)
  {
    (void)self; (void)ptr;
  }

  XAllocator x_arena_allocator(XArena* arena)
  {
    return (XAllocator) {
      .alloc = x_arena_alloc_wrapper,
        .free  = x_arena_free_noop,
        .userdata = arena
    };
  }

#endif // STDX_IMPLEMENTATION_ARENA

#ifdef STDX_INTERNAL_ALLOCATOR_IMPLEMENTATION
#undef STDX_IMPLEMENTATION_ALLOCATOR
#undef STDX_INTERNAL_ALLOCATOR_IMPLEMENTATION
#endif
#endif // STDX_ARENA_H
