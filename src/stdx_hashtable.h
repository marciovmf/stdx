/**
 * STDX - Generic Hashtable
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * ## Overview
 *
 *  Provides a generic, type-agnostic hashtable implementation with 
 *  customizable hash and equality functions. Supports arbitrary key 
 *  and value types, optional custom allocators, and built-in iteration.
 *  Includes helpers for common cases like string keys.
 *
 * ## How to compile
 *
 * To compile the implementation define `X_IMPL_HASHTABLE`
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * `X_HASHTABLE_ALLOC` / `X_HASHTABLE_REALLOC` / `X_HASHTABLE_FREE` before including.
 *
 * ## Dependencies:
 * - stdx_arena.h
 */

#ifndef X_HASHTABLE_H
#define X_HASHTABLE_H


#define X_HASHTABLE_VERSION_MAJOR 1
#define X_HASHTABLE_VERSION_MINOR 0
#define X_HASHTABLE_VERSION_PATCH 0
#define X_HASHTABLE_VERSION (X_HASHTABLE_VERSION_MAJOR * 10000 + X_HASHTABLE_VERSION_MINOR * 100 + X_HASHTABLE_VERSION_PATCH)

#define X_HASHTABLE_INITIAL_CAPACITY 16
#define X_HASHTABLE_LOAD_FACTOR 0.75

#include <stdx_common.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef size_t  (*XHashFnHash)(const void* key, size_t);
  typedef bool    (*XHashFnCompare)(const void* a, const void* b);
  typedef void    (*XHashFnClone)(void* dst, const void* src);
  typedef void    (*XHashFnDestroy)(void* ptr);

  typedef struct
  {
    void* key;
    void* value;
    enum XEntryState
    {
      X_HASH_ENTRY_FREE,
      X_HASH_ENTRY_OCCUPIED,
      X_HASH_ENTRY_DELETED,
    } state;
  } XHashEntry;

  typedef struct
  {
    size_t key_size;
    size_t value_size;
    size_t count;
    size_t capacity;
    XHashEntry* entries;    // parallel to keys and values
    void* keys;             // array of keys (capacity * key_size)
    void* values;           // array of values (capacity * value_size)

    bool key_is_pointer;
    bool value_is_pointer;
    bool key_is_null_terminated;
    bool value_is_null_terminated;

    // callbacks
    XHashFnHash       fn_key_hash;
    XHashFnCompare    fn_key_compare;
    XHashFnClone      fn_key_copy;
    XHashFnDestroy    fn_key_free;
    XHashFnClone      fn_value_copy;
    XHashFnDestroy    fn_value_free;
  } XHashtable;


#define X_STR(s) #s
#define X_TOSTR(s) X_STR(s)
#define X_TYPE_NAME(t) X_TOSTR(t)

#define x_hashtable_create(tk, tv) x_hashtable_create_(  \
    sizeof(tk), strcmp( X_TYPE_NAME(tk), "char*") == 0, strchr(X_TYPE_NAME(tk), '*') != 0, \
    sizeof(tv), strcmp( X_TYPE_NAME(tv), "char*") == 0, strchr(X_TYPE_NAME(tv), '*') != 0)

  static XHashtable* x_hashtable_create_(size_t key_size, bool key_null_terminated, bool key_is_pointer,
      size_t value_size, bool value_null_terminated, bool value_is_pointer);

  XHashtable* x_hashtable_create_full(
      size_t          key_size,
      size_t          value_size,
      XHashFnHash     fn_key_hash,
      XHashFnCompare  fn_key_compare,
      XHashFnClone    fn_key_copy,
      XHashFnDestroy  fn_key_free,
      XHashFnClone    fn_value_copy,
      XHashFnDestroy  fn_value_free);

  bool x_hashtable_set(XHashtable* table, const void* key, const void* value);
  bool x_hashtable_get(XHashtable* table, const void* key, void* out_value);
  void x_hashtable_destroy(XHashtable* table);
  size_t x_hashtable_count(const XHashtable* table);
  bool stdx_str_eq(const void* a, const void* b);
  bool x_hashtable_has(XHashtable* table, const void* key);
  bool x_hashtable_remove(XHashtable* table, const void* key);

  // public utilities
  size_t x_hashtable_hash_bytes(const void* ptr, size_t size);
  size_t x_hashtable_hash_cstr(const void* ptr, size_t size);

  void x_hashtable_clone_cstr(void* dest, const void* src);
  bool x_hashtable_compare_cstr(const void* a, const void* b);
  void x_hashtable_free_cstr(void* a);

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_HASHTABLE

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef X_HASHTABLE_ALLOC
#define X_HASHTABLE_ALLOC(sz)        malloc(sz)
#define X_HASHTABLE_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  // Helpers for pointer arithmetic on keys/values arrays
  static inline void* key_at(XHashtable* t, size_t i)
  {
    return (char*)t->keys + i * t->key_size;
  }

  static inline void* value_at(XHashtable* t, size_t i)
  {
    return (char*)t->values + i * t->value_size;
  }

  size_t x_hashtable_hash_bytes(const void* key, size_t size)
  {
    // djb2 hash
    const unsigned char* data = (const unsigned char*)key;
    size_t hash = 5381;
    for (size_t i = 0; i < size; i++)
      hash = ((hash << 5) + hash) + data[i];  // hash * 33 + data[i]
    return hash;
  }

  size_t x_hashtable_hash_cstr(const void* key, size_t _)
  {
    X_UNUSED(_);
    const char* str = (const char*)key;
    size_t len = strlen(str);
    return x_hashtable_hash_bytes(str, len);
  }

  bool x_hashtable_compare_cstr(const void* a, const void* b)
  {
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcmp(str_a, str_b) == 0;
  }

  void x_hashtable_free_cstr(void* a)
  {
    X_HASHTABLE_FREE(*(char**)a);
  }

  void x_hashtable_clone_cstr(void* dest, const void* src)
  {
    char* mem =  
#if defined(_MSC_VER)
      _strdup((const char*) src);
#else
    strdup((const char*) src);
#endif
    *(char**)dest = mem;
  }

  // Forward declarations
  static bool x_hashtable_resize(XHashtable* table, size_t new_capacity);

  static XHashtable* x_hashtable_create_(size_t key_size, bool key_null_terminated, bool key_is_pointer,
      size_t value_size, bool value_null_terminated, bool value_is_pointer)
  {
    XHashtable* ht = x_hashtable_create_full(
        key_size,
        value_size,
        key_null_terminated   ? x_hashtable_hash_cstr     : x_hashtable_hash_bytes,
        key_null_terminated   ? x_hashtable_compare_cstr  : NULL,
        key_null_terminated   ? x_hashtable_clone_cstr    : NULL,
        key_null_terminated   ? x_hashtable_free_cstr     : NULL,
        value_null_terminated ? x_hashtable_clone_cstr    : NULL,
        value_null_terminated ? x_hashtable_free_cstr     : NULL);

    ht->key_is_pointer = key_is_pointer;
    ht->key_is_null_terminated = key_null_terminated;
    ht->value_is_pointer = value_is_pointer;
    ht->value_is_null_terminated = value_null_terminated;
    return ht;
  }

  XHashtable* x_hashtable_create_full(
      size_t          key_size,
      size_t          value_size,
      XHashFnHash     fn_key_hash,
      XHashFnCompare  fn_key_compare,
      XHashFnClone    fn_key_copy,
      XHashFnDestroy  fn_key_free,
      XHashFnClone    fn_value_copy,
      XHashFnDestroy  fn_value_free
      )
  {
    XHashtable* table = (XHashtable*)X_HASHTABLE_ALLOC(sizeof(XHashtable));
    if (!table) return NULL;

    size_t capacity = X_HASHTABLE_INITIAL_CAPACITY;
    table->entries = (XHashEntry*)calloc(capacity, sizeof(XHashEntry));
    table->keys = X_HASHTABLE_ALLOC(key_size * capacity);
    table->values = X_HASHTABLE_ALLOC(value_size * capacity);

    if (!table->entries || !table->keys || !table->values)
    {
      X_HASHTABLE_FREE(table->entries);
      X_HASHTABLE_FREE(table->keys);
      X_HASHTABLE_FREE(table->values);
      X_HASHTABLE_FREE(table);
      return NULL;
    }

    table->key_size = key_size;
    table->value_size = value_size;
    table->count = 0;
    table->capacity = capacity;

    table->fn_key_hash = fn_key_hash;
    table->fn_key_compare = fn_key_compare;
    table->fn_key_copy = fn_key_copy;
    table->fn_key_free = fn_key_free;
    table->fn_value_copy = fn_value_copy;
    table->fn_value_free = fn_value_free;

    // all entries are free by calloc
    return table;
  }

  static size_t probe_index(XHashtable* table, const void* key, size_t *out_index_found, bool *found)
  {
    size_t capacity = table->capacity;
    size_t hash = table->fn_key_hash(key, table->key_size);
    size_t index = hash % capacity;

    size_t first_deleted = (size_t)-1;

    for (size_t i = 0; i < capacity; i++)
    {
      size_t idx = (index + i) % capacity;
      XHashEntry* entry = &table->entries[idx];

      if (entry->state == X_HASH_ENTRY_FREE)
      {
        if (first_deleted != (size_t)-1)
          *out_index_found = first_deleted;
        else
          *out_index_found = idx;

        *found = false;
        return idx;
      }
      else if (entry->state == X_HASH_ENTRY_DELETED)
      {
        if (first_deleted == (size_t)-1)
          first_deleted = idx;
      }
      else if (entry->state == X_HASH_ENTRY_OCCUPIED)
      {
        void* key_at_idx = key_at(table, idx);
        bool keys_match = table->fn_key_compare ?
          table->fn_key_compare(key_at_idx, &key) :
          memcmp(key_at_idx, key, table->key_size) == 0;

        if (keys_match)
        {
          *out_index_found = idx;
          *found = true;
          return idx;
        }
      }
    }

    // table full but should never happen if resized properly
    *out_index_found = (size_t)-1;
    *found = false;
    return (size_t)-1;
  }

  bool x_hashtable_set(XHashtable* table, const void* key, const void* value)
  {
    if (!table || !key) return false;

    // resize if load factor exceeded
    if ((double)(table->count + 1) / table->capacity > X_HASHTABLE_LOAD_FACTOR)
    {
      if (!x_hashtable_resize(table, table->capacity * 2))
        return false;
    }

    size_t idx;
    bool found;

    probe_index(table,
        (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
        &idx, &found);
    if (idx == (size_t)-1)
      return false; // table full

    XHashEntry* entry = &table->entries[idx];

    if (found)
    {
      // Replace existing value
      void* value_ptr = value_at(table, idx);
      if (table->fn_value_free)
        table->fn_value_free(value_ptr);
      if (table->fn_value_copy)
        table->fn_value_copy(value_ptr, value);
      else
        memcpy(value_ptr,
            (table->value_is_pointer && !table->value_is_null_terminated) ? &value : value,
            table->value_size);
    }
    else
    {
      // Insert new entry
      if (table->fn_key_copy)
        table->fn_key_copy(key_at(table, idx), key);
      else
        memcpy(key_at(table, idx), 
            (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
            table->key_size);

      if (table->fn_value_copy)
        table->fn_value_copy(value_at(table, idx), value);
      else
        memcpy(value_at(table, idx),
            (table->value_is_pointer && !table->value_is_null_terminated) ? &value : value,
            table->value_size);

      entry->state = X_HASH_ENTRY_OCCUPIED;
      table->count++;
    }

    return true;
  }

  bool x_hashtable_get(XHashtable* table, const void* key, void* out_value)
  {
    if (!table || !key || !out_value) return false;

    size_t idx;
    bool found;
    probe_index(table,
        (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
        &idx, &found);

    if (found)
    {
      void* value_ptr = value_at(table, idx);
      //memcpy(out_value, value_ptr, table->value_size);
      memcpy(out_value,
          (table->value_is_pointer && !table->value_is_null_terminated) ? &value_ptr : value_ptr,
          table->value_size);
      return true;
    }
    return false;
  }

  bool x_hashtable_has(XHashtable* table, const void* key)
  {
    if (!table || !key) return false;

    size_t idx;
    bool found;
    probe_index(table, key, &idx, &found);
    return found;
  }

  bool x_hashtable_remove(XHashtable* table, const void* key)
  {
    if (!table || !key) return false;

    size_t idx;
    bool found;
    probe_index(table, key, &idx, &found);
    if (!found) return false;

    XHashEntry* entry = &table->entries[idx];
    void* key_ptr = key_at(table, idx);
    void* value_ptr = value_at(table, idx);

    if (table->fn_key_free)
      table->fn_key_free(key_ptr);
    if (table->fn_value_free)
      table->fn_value_free(value_ptr);

    entry->state = X_HASH_ENTRY_DELETED;
    table->count--;
    return true;
  }

  void x_hashtable_destroy(XHashtable* table)
  {
    if (!table) return;

    // free keys and values if needed
    for (size_t i = 0; i < table->capacity; i++)
    {
      if (table->entries[i].state == X_HASH_ENTRY_OCCUPIED)
      {
        void* key_ptr = key_at(table, i);
        void* value_ptr = value_at(table, i);

        if (table->fn_key_free)
          table->fn_key_free(key_ptr);
        if (table->fn_value_free)
          table->fn_value_free(value_ptr);
      }
    }

    X_HASHTABLE_FREE(table->entries);
    X_HASHTABLE_FREE(table->keys);
    X_HASHTABLE_FREE(table->values);
    X_HASHTABLE_FREE(table);
  }

  size_t x_hashtable_count(const XHashtable* table)
  {
    return table ? table->count : 0;
  }

  static bool x_hashtable_resize(XHashtable* table, size_t new_capacity)
  {
    if (!table || new_capacity <= table->capacity) return false;

    XHashEntry* new_entries = (XHashEntry*)calloc(new_capacity, sizeof(XHashEntry));
    void* new_keys = X_HASHTABLE_ALLOC(table->key_size * new_capacity);
    void* new_values = X_HASHTABLE_ALLOC(table->value_size * new_capacity);

    if (!new_entries || !new_keys || !new_values)
    {
      X_HASHTABLE_FREE(new_entries);
      X_HASHTABLE_FREE(new_keys);
      X_HASHTABLE_FREE(new_values);
      return false;
    }

    // Save old arrays
    XHashEntry* old_entries = table->entries;
    void* old_keys = table->keys;
    void* old_values = table->values;
    size_t old_capacity = table->capacity;

    table->entries = new_entries;
    table->keys = new_keys;
    table->values = new_values;
    table->capacity = new_capacity;
    table->count = 0;

    // Rehash all old entries
    for (size_t i = 0; i < old_capacity; i++)
    {
      if (old_entries[i].state == X_HASH_ENTRY_OCCUPIED)
      {
        void* key_ptr = (char*)old_keys + i * table->key_size;
        void* value_ptr = (char*)old_values + i * table->value_size;

        x_hashtable_set(table,
            table->key_is_null_terminated ? *(void**)key_ptr : key_ptr, 
            table->value_is_null_terminated ? *(void**)value_ptr : value_ptr);

        // Free old keys and values if user callbacks exist
        if (table->fn_key_free)
          table->fn_key_free(key_ptr);
        if (table->fn_value_free)
          table->fn_value_free(value_ptr);
      }
    }

    X_HASHTABLE_FREE(old_entries);
    X_HASHTABLE_FREE(old_keys);
    X_HASHTABLE_FREE(old_values);

    return true;
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_HASHTABLE
#endif // X_HASHTABLE_H
