/*
 * STDX - Generic Hashtable
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a generic, type-agnostic hashtable implementation with 
 * customizable hash and equality functions. Supports arbitrary key 
 * and value types, optional custom allocators, and built-in iteration.
 * 
 * Includes helpers for common cases like string keys.
 *
 * Header-only and modular. Designed for performance and flexibility.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_HASHTABLE
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Dependencies: stdx_arena.h stdx_allocator.h
 * Usage: #include "stdx_hashtable.h"
 */

#ifndef STDX_HASHTABLE_H
#define STDX_HASHTABLE_H

#ifdef __cplusplus
extern "C"
{
#endif

#define STDX_HASHTABLE_VERSION_MAJOR 1
#define STDX_HASHTABLE_VERSION_MINOR 0
#define STDX_HASHTABLE_VERSION_PATCH 0

#define STDX_HASHTABLE_VERSION (STDX_HASHTABLE_VERSION_MAJOR * 10000 + STDX_HASHTABLE_VERSION_MINOR * 100 + STDX_HASHTABLE_VERSION_PATCH)


#ifdef STDX_IMPLEMENTATION_HASHTABLE
#ifndef STDX_IMPLEMENTATION_ARENA
#define STDX_INTERNAL_ARENA_IMPLEMENTATION
#define STDX_IMPLEMENTATION_ARENA
#endif
#endif
#include <stdx_arena.h>

#include <stddef.h>
#include <stdbool.h>

  /**
   * VALUE_PTR(type, val)
   *
   * Wraps a value of a primitive type (e.g., int, float, etc.) in a temporary
   * compound literal and returns a pointer to it, cast as (const void*).
   *
   * This macro is primarily intended to facilitate the use of value types with
   * generic hashtable interfaces such as:
   *
   *     bool x_hashtable_set(XHashtable* table, const void* key, const void* value);
   *
   * Example usage:
   *
   *     x_hashtable_set(table, "health", VALUE_PTR(int8_t, 100));
   *     x_hashtable_set(table, "pi", VALUE_PTR(float, 3.14f));
   *
   * The compound literal ensures the value persists for the duration of the
   * full expression, making the resulting pointer safe to use within the
   * x_hashtable_set call.
   *
   * NOTE:
   * - Requires C99 or later (for compound literals).
   * - The pointer must not be stored or used after the function call.
   */
#define VALUE_PTR(type, val) ((const void*)&(type){ (val) })

  typedef size_t (*HashFn)(const void* key);
  typedef bool   (*EqualsFn)(const void* a, const void* b);

  typedef struct
  {
    void* key;
    void* value;
    bool occupied;
  } XHashEntry;

  typedef struct
  {
    size_t key_size;
    size_t value_size;
    size_t count;
    size_t capacity;
    XHashEntry* entries;
    HashFn hash_fn;
    EqualsFn eq_fn;
    XAllocator* allocator;
  } XHashtable;

#define x_hashtable_create(ks, vs, hf, eqf) x_hashtable_create_ex(ks, vs, hf, eqf, NULL)

  XHashtable* x_hashtable_create_ex(size_t key_size, size_t value_size, HashFn hash_fn, EqualsFn eq_fn, XAllocator* allocator);
  void x_hashtable_destroy(XHashtable* table);
  bool x_hashtable_set(XHashtable* table, const void* key, const void* value);
  bool x_hashtable_get(XHashtable* table, const void* key, void* out_value);
  bool x_hashtable_has(XHashtable* table, const void* key);
  bool x_hashtable_remove(XHashtable* table, const void* key);
  size_t x_hashtable_count(const XHashtable* table);

  //---------------------------------------------------------------------------------
  // Iterator
  //---------------------------------------------------------------------------------
  typedef struct {
    const XHashtable* table;
    size_t index;
  } XHashIter;

  void x_hashtable_iter_init(XHashIter* iter, const XHashtable* table);
  bool x_hashtable_iter_next(XHashIter* iter, void** out_key, void** out_value);


  //---------------------------------------------------------------------------------
  // Helpers
  //---------------------------------------------------------------------------------
  size_t stdx_hash_str(const void* str);
  bool stdx_str_eq(const void* a, const void* b);

#ifdef STDX_IMPLEMENTATION_HASHTABLE

#include <stdlib.h>
#include <string.h>

#define STDX_HASHTABLE_INITIAL_CAPACITY 16
#define STDX_HASHTABLE_LOAD_FACTOR 0.75

  static void x_hashtable_rehash(XHashtable* table);

  XHashtable* x_hashtable_create_ex(size_t key_size, size_t value_size, HashFn hash_fn, EqualsFn eq_fn, XAllocator* allocator)
  {
    XHashtable* t = (XHashtable*) stdx_alloc(allocator, sizeof(XHashtable));
    if (!t) return NULL;

    t->key_size = key_size;
    t->value_size = value_size;
    t->count = 0;
    t->capacity = STDX_HASHTABLE_INITIAL_CAPACITY;
    t->hash_fn = hash_fn;
    t->eq_fn = eq_fn;
    t->allocator = allocator;

    t->entries = (XHashEntry*) stdx_alloc(allocator, STDX_HASHTABLE_INITIAL_CAPACITY * sizeof(XHashEntry));
    memset(t->entries, 0, STDX_HASHTABLE_INITIAL_CAPACITY * sizeof(XHashEntry));
    return t;
  }

  void x_hashtable_destroy(XHashtable* table)
  {
    if (!table) return;
    XAllocator* a = table->allocator;
    for (size_t i = 0; i < table->capacity; ++i)
    {
      if (table->entries[i].occupied)
      {
        stdx_free(a, table->entries[i].key);
        stdx_free(a, table->entries[i].value);
      }
    }
    stdx_free(a, table->entries);
    stdx_free(a, table);
  }

  static size_t probe_index(XHashtable* table, const void* key, bool* found)
  {
    size_t idx = table->hash_fn(key) % table->capacity;
    size_t start = idx;

    while (table->entries[idx].occupied)
    {
      if (table->eq_fn(key, table->entries[idx].key))
      {
        *found = true;
        return idx;
      }
      idx = (idx + 1) % table->capacity;
      if (idx == start) break;
    }
    *found = false;
    return idx;
  }

  bool x_hashtable_set(XHashtable* table, const void* key, const void* value)
  {
    if (!table) return false;

    if ((double)table->count / table->capacity >= STDX_HASHTABLE_LOAD_FACTOR)
      x_hashtable_rehash(table);

    bool found;
    size_t idx = probe_index(table, key, &found);
    XAllocator* a = table->allocator;

    if (!found)
    {
      table->entries[idx].key = stdx_alloc(a, table->key_size);
      memcpy(table->entries[idx].key, key, table->key_size);
      table->entries[idx].value = stdx_alloc(a, table->value_size);
      table->entries[idx].occupied = true;
      table->count++;
    }

    memcpy(table->entries[idx].value, value, table->value_size);
    return true;
  }

  bool x_hashtable_get(XHashtable* table, const void* key, void* out_value)
  {
    if (!table) return false;
    bool found;
    size_t idx = probe_index(table, key, &found);
    if (!found) return false;
    memcpy(out_value, table->entries[idx].value, table->value_size);
    return true;
  }

  bool x_hashtable_has(XHashtable* table, const void* key)
  {
    bool found;
    probe_index(table, key, &found);
    return found;
  }

  bool x_hashtable_remove(XHashtable* table, const void* key)
  {
    bool found;
    size_t idx = probe_index(table, key, &found);
    if (!found) return false;

    XAllocator* a = table->allocator;
    stdx_free(a, table->entries[idx].key);
    stdx_free(a, table->entries[idx].value);
    table->entries[idx] = (XHashEntry){0};
    table->count--;
    return true;
  }

  static void x_hashtable_rehash(XHashtable* table)
  {
    size_t old_cap = table->capacity;
    XHashEntry* old_entries = table->entries;
    XAllocator* a = table->allocator;

    table->capacity *= 2;
    table->entries = (XHashEntry*) stdx_alloc(a, table->capacity * sizeof(XHashEntry));
    memset(table->entries, 0, table->capacity * sizeof(XHashEntry));
    table->count = 0;

    for (size_t i = 0; i < old_cap; ++i)
    {
      if (!old_entries[i].occupied) continue;
      x_hashtable_set(table, old_entries[i].key, old_entries[i].value);
      stdx_free(a, old_entries[i].key);
      stdx_free(a, old_entries[i].value);
    }
    stdx_free(a, old_entries);
  }

  size_t x_hashtable_count(const XHashtable* table)
  {
    return table ? table->count : 0;
  }

  // Helper: string hash & compare
  size_t stdx_hash_str(const void* ptr)
  {
    const int8_t* str = (const int8_t*) ptr;
    size_t hash = 5381;
    while (*str) hash = ((hash << 5) + hash) + (uint8_t)(*str++);
    return hash;
  }

  bool stdx_str_eq(const void* a, const void* b)
  {
    return strcmp((const int8_t*)a, (const int8_t*)b) == 0;
  }

  void x_hashtable_iter_init(XHashIter* iter, const XHashtable* table)
  {
    iter->table = table;
    iter->index = 0;
  }

  bool x_hashtable_iter_next(XHashIter* iter, void** out_key, void** out_value)
  {
    while (iter->index < iter->table->capacity) {
      XHashEntry* entry = &iter->table->entries[iter->index++];
      if (entry->occupied) {
        if (out_key)   *out_key   = entry->key;
        if (out_value) *out_value = entry->value;
        return true;
      }
    }
    return false;
  }

#endif // STDX_IMPLEMENTATION_HASHTABLE

#ifdef STDX_INTERNAL_ARENA_IMPLEMENTATION
#undef STDX_IMPLEMENTATION_ARENA
#undef STDX_INTERNAL_ARENA_IMPLEMENTATION
#endif

#ifdef __cplusplus
}
#endif

#endif // STDX_HASHTABLE_H
