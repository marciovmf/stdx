/**
 * STDX - Generic Hashtable 
 * Part of the STDX General Purpose C Library by marciovmf 
 * License: MIT 
 * <https://github.com/marciovmf/stdx> 
 *
 * ## Overview
 *
 * Provides a generic, type-agnostic hashtable implementation with
 * customizable hash and equality functions. Supports arbitrary key 
 * and value types, optional custom allocators, and built-in iteration. 
 * Includes helpers for common cases like string keys. 
 * 
 * ## How to compile 
 * 
 * To compile the implementation define X_IMPL_HASHTABLE 
 * in **one** source file before including this header. 
 * 
 * To customize how this module allocates memory, define 
 * X_HASHTABLE_ALLOC / X_HASHTABLE_REALLOC / X_HASHTABLE_FREE before including.
 *
 * ## Typed usage (recommended)
 *
 * The hashtable core operates on `void*` and explicit key/value sizes.
 * For convenience, this header also provides macros that generate
 * type-safe wrappers on top of the generic API.
 *
 * These macros create:
 *
 * - a typed alias for `XHashtable`
 * - typed wrapper functions (`create`, `set`, `get`, `has`, `remove`, `count`)
 *
 * This removes the need for manual casts and makes the API easier to use.
 *
 * ### Example
 *
 * ```
 * X_HASHTABLE_TYPE(int32_t, float)
 * XHashtable_int32_t_float* ht = x_hashtable_int32_t_float_create();
 * x_hashtable_int32_t_float_set(ht, 10, 2.5f);
 * float value;
 * if (x_hashtable_int32_t_float_get(ht, 10, &value))
 * {
 *     printf("value = %f\n", value);
 * }
 * ```
 *
 * ### Named variants
 *
 * If the key or value type is not a valid identifier (for example
 * `char*`, `unsigned int`, or complex types), use the named version:
 *
 * ```
 * X_HASHTABLE_TYPE_NAMED(char*, int32_t, cstr_i32)
 * XHashtable_cstr_i32* ht = x_hashtable_cstr_i32_create();
 * ```
 *
 * ### Specialized macros
 *
 * Some key/value combinations require explicit semantics. The following
 * helpers are provided:
 *
 * - `X_HASHTABLE_TYPE_PTR_KEY_NAMED`
 * - `X_HASHTABLE_TYPE_CSTR_KEY_NAMED`
 * - `X_HASHTABLE_TYPE_PTR_VALUE_NAMED`
 * - `X_HASHTABLE_TYPE_PTR_KEY_PTR_VALUE_NAMED`
 * - `X_HASHTABLE_TYPE_CSTR_KEY_PTR_VALUE_NAMED`
 *
 * These macros declare the correct ownership and comparison behavior
 * for pointer keys, string keys, or pointer values.
 *
 * ### Example with string keys
 *
 * ```
 * X_HASHTABLE_TYPE_CSTR_KEY_NAMED(int32_t, cstr_i32)
 * XHashtable_cstr_i32* ht = x_hashtable_cstr_i32_create();
 * x_hashtable_cstr_i32_set(ht, "ONE", 1);
 * x_hashtable_cstr_i32_set(ht, "TWO", 2);
 * ```
 *
 * Internally these wrappers call the generic API, so there is no
 * additional runtime overhead.
 *
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

#ifndef X_HASHTABLE_API
#define X_HASHTABLE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef size_t  (*XHashFnHash)(const void* key, size_t);
  typedef bool    (*XHashFnCompare)(const void* a, const void* b);
  typedef void    (*XHashFnClone)(void* dst, const void* src);
  typedef void    (*XHashFnDestroy)(void* ptr);

  typedef struct
  {
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
    XHashEntry* entries;    /* parallel to keys and values */
    void* keys;             /* array of keys (capacity * key_size) */
    void* values;           /* array of values (capacity * value_size) */

    bool key_is_pointer;
    bool value_is_pointer;
    bool key_is_null_terminated;
    bool value_is_null_terminated;

    /* callbacks */
    XHashFnHash       fn_key_hash;
    XHashFnCompare    fn_key_compare;
    XHashFnClone      fn_key_copy;
    XHashFnDestroy    fn_key_free;
    XHashFnClone      fn_value_copy;
    XHashFnDestroy    fn_value_free;
  } XHashtable;

  typedef struct
  {
    XHashtable* table;
    size_t index;      /* current occupied slot, or table->capacity when finished */
  } XHashtableIter;

  /**
   * @brief Create a hashtable using raw size and explicit string/pointer traits for keys and values.
   * @param key_size Size in bytes of the key type.
   * @param key_null_terminated True if the key is a NUL-terminated string (C string).
   * @param key_is_pointer True if the key is stored as a pointer value.
   * @param value_size Size in bytes of the value type.
   * @param value_null_terminated True if the value is a NUL-terminated string (C string).
   * @param value_is_pointer True if the value is stored as a pointer value.
   * @return Pointer to a newly created hashtable, or NULL on failure.
   */
  X_HASHTABLE_API XHashtable* x_hashtable_create_ex(
      size_t key_size,
      bool key_null_terminated,
      bool key_is_pointer,
      size_t value_size,
      bool value_null_terminated,
      bool value_is_pointer);

  /**
   * @brief Create a fully-configurable hashtable with custom hashing, comparison, cloning, and destruction.
   * @param key_size Size in bytes of the key type.
   * @param value_size Size in bytes of the value type.
   * @param fn_key_hash Function used to hash keys.
   * @param fn_key_compare Function used to compare keys for equality.
   * @param fn_key_copy Function used to clone/copy keys into the table.
   * @param fn_key_free Function used to destroy/free keys owned by the table.
   * @param fn_value_copy Function used to clone/copy values into the table.
   * @param fn_value_free Function used to destroy/free values owned by the table.
   * @return Pointer to a newly created hashtable, or NULL on failure.
   */
  XHashtable* x_hashtable_create_full(
      size_t          key_size,
      size_t          value_size,
      XHashFnHash     fn_key_hash,
      XHashFnCompare  fn_key_compare,
      XHashFnClone    fn_key_copy,
      XHashFnDestroy  fn_key_free,
      XHashFnClone    fn_value_copy,
      XHashFnDestroy  fn_value_free);

  /**
   * @brief Insert or update a key/value pair in the hashtable.
   * @param table Hashtable instance.
   * @param key Pointer to the key data.
   * @param value Pointer to the value data.
   * @return True on success, false on failure (e.g., allocation failure).
   */
  X_HASHTABLE_API bool x_hashtable_set(XHashtable* table, const void* key, const void* value);

  /**
   * @brief Retrieve a value from the hashtable by key.
   * @param table Hashtable instance.
   * @param key Pointer to the key data.
   * @param out_value Output buffer to receive the value.
   * @return True if the key was found and out_value was written, false otherwise.
   */
  X_HASHTABLE_API bool x_hashtable_get(XHashtable* table, const void* key, void* out_value);

  /**
   * @brief Destroy a hashtable and free all associated resources.
   * @param table Hashtable instance to destroy.
   * @return Nothing.
   */
  X_HASHTABLE_API void x_hashtable_destroy(XHashtable* table);

  /**
   * @brief Get the number of stored entries in the hashtable.
   * @param table Hashtable instance.
   * @return Number of key/value pairs currently stored.
   */
  X_HASHTABLE_API size_t x_hashtable_count(const XHashtable* table);

  /**
   * @brief Compare two NUL-terminated strings for equality.
   * @param a Pointer to the first string (const char*).
   * @param b Pointer to the second string (const char*).
   * @return True if equal, false otherwise.
   */
  X_HASHTABLE_API bool stdx_str_eq(const void* a, const void* b);

  /**
   * @brief Check whether the hashtable contains a key.
   * @param table Hashtable instance.
   * @param key Pointer to the key data.
   * @return True if the key exists, false otherwise.
   */
  X_HASHTABLE_API bool x_hashtable_has(XHashtable* table, const void* key);

  /**
   * @brief Remove an entry from the hashtable by key.
   * @param table Hashtable instance.
   * @param key Pointer to the key data.
   * @return True if an entry was removed, false if the key was not found.
   */
  X_HASHTABLE_API bool x_hashtable_remove(XHashtable* table, const void* key);

  /**
   * @brief Hash an arbitrary byte buffer.
   * @param ptr Pointer to the bytes to hash.
   * @param size Number of bytes to hash.
   * @return Hash value for the given byte buffer.
   */
  X_HASHTABLE_API size_t x_hashtable_hash_bytes(const void* ptr, size_t size);

  /**
   * @brief Hash a NUL-terminated C string (or string-like buffer) for hashtable use.
   * @param ptr Pointer to the string data (typically const char*).
   * @param size Size parameter (typically ignored for C strings, or used as a limit depending on implementation).
   * @return Hash value for the string.
   */
  X_HASHTABLE_API size_t x_hashtable_hash_cstr(const void* ptr, size_t size);

  /**
   * @brief Clone a NUL-terminated C string into destination storage.
   * @param dest Destination storage receiving the cloned string (implementation-defined: may store char*).
   * @param src Source string pointer (const char*).
   * @return Nothing.
   */
  X_HASHTABLE_API void x_hashtable_clone_cstr(void* dest, const void* src);

  /**
   * @brief Compare two NUL-terminated C strings for hashtable key equality.
   * @param a First string pointer (const char*).
   * @param b Second string pointer (const char*).
   * @return True if equal, false otherwise.
   */
  X_HASHTABLE_API bool x_hashtable_compare_cstr(const void* a, const void* b);

  /**
   * @brief Free a cloned NUL-terminated C string previously allocated by x_hashtable_clone_cstr().
   * @param a Pointer to the stored string (typically a char* or pointer slot containing it).
   * @return Nothing.
   */
  X_HASHTABLE_API void x_hashtable_free_cstr(void* a);

  /**
   * @brief Initialize an iterator for a hashtable.
   * @param table Hashtable instance.
   * @param it Iterator to initialize.
   * @return True if the iterator was initialized (even if empty table), false on invalid args.
   */
  X_HASHTABLE_API bool x_hashtable_iter_begin(XHashtable* table, XHashtableIter* it);

  /**
   * @brief Advance iterator to the next occupied entry.
   * @param it Iterator.
   * @param out_key Receives pointer to key (user-facing: dereferenced if stored as pointer/string).
   * @param out_value Receives pointer to value (user-facing: dereferenced if stored as pointer/string).
   * @return True if an entry was produced, false if iteration finished or invalid args.
   */
  X_HASHTABLE_API bool x_hashtable_iter_next(XHashtableIter* it, void** out_key, void** out_value);

  /**
   * @brief Const-friendly variant of x_hashtable_iter_next().
   */
  X_HASHTABLE_API bool x_hashtable_iter_next_const(XHashtableIter* it, const void** out_key, const void** out_value);

  /**
   * @brief Get the raw slot index of the current entry (mainly for debugging).
   * @param it Iterator.
   * @return Current slot index, or table->capacity if finished/invalid.
   */
  X_HASHTABLE_API size_t x_hashtable_iter_slot(const XHashtableIter* it);

#define X_HASHTABLE_TYPE(tk, tv) X_HASHTABLE_TYPE_NAMED(tk, tv, tk##_##tv)

#define X_HASHTABLE_TYPE_NAMED(tk, tv, suffix) \
  typedef XHashtable XHashtable_##suffix; \
  static inline XHashtable_##suffix* x_hashtable_##suffix##_create(void) \
  { \
    return (XHashtable_##suffix*)x_hashtable_create_ex(sizeof(tk), false, false, sizeof(tv), false, false); \
  } \
  static inline bool x_hashtable_##suffix##_set(XHashtable_##suffix* table, tk key, tv value) \
  { \
    return x_hashtable_set((XHashtable*)table, &key, &value); \
  } \
  static inline bool x_hashtable_##suffix##_get(XHashtable_##suffix* table, tk key, tv* out_value) \
  { \
    return x_hashtable_get((XHashtable*)table, &key, out_value); \
  } \
  static inline bool x_hashtable_##suffix##_has(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_has((XHashtable*)table, &key); \
  } \
  static inline bool x_hashtable_##suffix##_remove(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_remove((XHashtable*)table, &key); \
  } \
  static inline void x_hashtable_##suffix##_destroy(XHashtable_##suffix* table) \
  { \
    x_hashtable_destroy((XHashtable*)table); \
  } \
  static inline size_t x_hashtable_##suffix##_count(const XHashtable_##suffix* table) \
  { \
    return x_hashtable_count((const XHashtable*)table); \
  }

#define X_HASHTABLE_TYPE_PTR_KEY_NAMED(tk, tv, suffix) \
  typedef XHashtable XHashtable_##suffix; \
  static inline XHashtable_##suffix* x_hashtable_##suffix##_create(void) \
  { \
    return (XHashtable_##suffix*)x_hashtable_create_ex(sizeof(tk), false, true, sizeof(tv), false, false); \
  } \
  static inline bool x_hashtable_##suffix##_set(XHashtable_##suffix* table, tk key, tv value) \
  { \
    return x_hashtable_set((XHashtable*)table, key, &value); \
  } \
  static inline bool x_hashtable_##suffix##_get(XHashtable_##suffix* table, tk key, tv* out_value) \
  { \
    return x_hashtable_get((XHashtable*)table, key, out_value); \
  } \
  static inline bool x_hashtable_##suffix##_has(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_has((XHashtable*)table, key); \
  } \
  static inline bool x_hashtable_##suffix##_remove(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_remove((XHashtable*)table, key); \
  } \
  static inline void x_hashtable_##suffix##_destroy(XHashtable_##suffix* table) \
  { \
    x_hashtable_destroy((XHashtable*)table); \
  } \
  static inline size_t x_hashtable_##suffix##_count(const XHashtable_##suffix* table) \
  { \
    return x_hashtable_count((const XHashtable*)table); \
  }

#define X_HASHTABLE_TYPE_CSTR_KEY_NAMED(tv, suffix) \
  typedef XHashtable XHashtable_##suffix; \
  static inline XHashtable_##suffix* x_hashtable_##suffix##_create(void) \
  { \
    return (XHashtable_##suffix*)x_hashtable_create_ex(sizeof(char*), true, true, sizeof(tv), false, false); \
  } \
  static inline bool x_hashtable_##suffix##_set(XHashtable_##suffix* table, const char* key, tv value) \
  { \
    return x_hashtable_set((XHashtable*)table, key, &value); \
  } \
  static inline bool x_hashtable_##suffix##_get(XHashtable_##suffix* table, const char* key, tv* out_value) \
  { \
    return x_hashtable_get((XHashtable*)table, key, out_value); \
  } \
  static inline bool x_hashtable_##suffix##_has(XHashtable_##suffix* table, const char* key) \
  { \
    return x_hashtable_has((XHashtable*)table, key); \
  } \
  static inline bool x_hashtable_##suffix##_remove(XHashtable_##suffix* table, const char* key) \
  { \
    return x_hashtable_remove((XHashtable*)table, key); \
  } \
  static inline void x_hashtable_##suffix##_destroy(XHashtable_##suffix* table) \
  { \
    x_hashtable_destroy((XHashtable*)table); \
  } \
  static inline size_t x_hashtable_##suffix##_count(const XHashtable_##suffix* table) \
  { \
    return x_hashtable_count((const XHashtable*)table); \
  }

#define X_HASHTABLE_TYPE_PTR_VALUE_NAMED(tk, tv, suffix) \
  typedef XHashtable XHashtable_##suffix; \
  static inline XHashtable_##suffix* x_hashtable_##suffix##_create(void) \
  { \
    return (XHashtable_##suffix*)x_hashtable_create_ex(sizeof(tk), false, false, sizeof(tv), false, true); \
  } \
  static inline bool x_hashtable_##suffix##_set(XHashtable_##suffix* table, tk key, tv value) \
  { \
    return x_hashtable_set((XHashtable*)table, &key, value); \
  } \
  static inline bool x_hashtable_##suffix##_get(XHashtable_##suffix* table, tk key, tv* out_value) \
  { \
    return x_hashtable_get((XHashtable*)table, &key, out_value); \
  } \
  static inline bool x_hashtable_##suffix##_has(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_has((XHashtable*)table, &key); \
  } \
  static inline bool x_hashtable_##suffix##_remove(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_remove((XHashtable*)table, &key); \
  } \
  static inline void x_hashtable_##suffix##_destroy(XHashtable_##suffix* table) \
  { \
    x_hashtable_destroy((XHashtable*)table); \
  } \
  static inline size_t x_hashtable_##suffix##_count(const XHashtable_##suffix* table) \
  { \
    return x_hashtable_count((const XHashtable*)table); \
  }

#define X_HASHTABLE_TYPE_PTR_KEY_PTR_VALUE_NAMED(tk, tv, suffix) \
  typedef XHashtable XHashtable_##suffix; \
  static inline XHashtable_##suffix* x_hashtable_##suffix##_create(void) \
  { \
    return (XHashtable_##suffix*)x_hashtable_create_ex(sizeof(tk), false, true, sizeof(tv), false, true); \
  } \
  static inline bool x_hashtable_##suffix##_set(XHashtable_##suffix* table, tk key, tv value) \
  { \
    return x_hashtable_set((XHashtable*)table, key, value); \
  } \
  static inline bool x_hashtable_##suffix##_get(XHashtable_##suffix* table, tk key, tv* out_value) \
  { \
    return x_hashtable_get((XHashtable*)table, key, out_value); \
  } \
  static inline bool x_hashtable_##suffix##_has(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_has((XHashtable*)table, key); \
  } \
  static inline bool x_hashtable_##suffix##_remove(XHashtable_##suffix* table, tk key) \
  { \
    return x_hashtable_remove((XHashtable*)table, key); \
  } \
  static inline void x_hashtable_##suffix##_destroy(XHashtable_##suffix* table) \
  { \
    x_hashtable_destroy((XHashtable*)table); \
  } \
  static inline size_t x_hashtable_##suffix##_count(const XHashtable_##suffix* table) \
  { \
    return x_hashtable_count((const XHashtable*)table); \
  }

#define X_HASHTABLE_TYPE_CSTR_KEY_PTR_VALUE_NAMED(tv, suffix) \
  typedef XHashtable XHashtable_##suffix; \
  static inline XHashtable_##suffix* x_hashtable_##suffix##_create(void) \
  { \
    return (XHashtable_##suffix*)x_hashtable_create_ex(sizeof(char*), true, true, sizeof(tv), false, true); \
  } \
  static inline bool x_hashtable_##suffix##_set(XHashtable_##suffix* table, const char* key, tv value) \
  { \
    return x_hashtable_set((XHashtable*)table, key, value); \
  } \
  static inline bool x_hashtable_##suffix##_get(XHashtable_##suffix* table, const char* key, tv* out_value) \
  { \
    return x_hashtable_get((XHashtable*)table, key, out_value); \
  } \
  static inline bool x_hashtable_##suffix##_has(XHashtable_##suffix* table, const char* key) \
  { \
    return x_hashtable_has((XHashtable*)table, key); \
  } \
  static inline bool x_hashtable_##suffix##_remove(XHashtable_##suffix* table, const char* key) \
  { \
    return x_hashtable_remove((XHashtable*)table, key); \
  } \
  static inline void x_hashtable_##suffix##_destroy(XHashtable_##suffix* table) \
  { \
    x_hashtable_destroy((XHashtable*)table); \
  } \
  static inline size_t x_hashtable_##suffix##_count(const XHashtable_##suffix* table) \
  { \
    return x_hashtable_count((const XHashtable*)table); \
  }

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
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_HASHTABLE_ALLOC(sz)        malloc(sz)
#endif

#ifndef X_HASHTABLE_CALLOC
/**
 * @brief Internal macro for allocating zero-initialized memory.
 * To override how this header allocates zero-initialized memory, define this macro with a
 * different implementation before including this header.
 * @param n  The number of elements to alloc.
 * @param sz The size of each element.
 */
#define X_HASHTABLE_CALLOC(n,sz)     calloc((n),(sz))
#endif

#ifndef X_HASHTABLE_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_HASHTABLE_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  X_HASHTABLE_API static inline void* key_at(XHashtable* t, size_t i)
  {
    return (char*)t->keys + i * t->key_size;
  }

  X_HASHTABLE_API static inline void* value_at(XHashtable* t, size_t i)
  {
    return (char*)t->values + i * t->value_size;
  }

  X_HASHTABLE_API size_t x_hashtable_hash_bytes(const void* key, size_t size)
  {
    const unsigned char* data = (const unsigned char*)key;
    size_t hash = 5381;
    size_t i;
    for (i = 0; i < size; i++)
    {
      hash = ((hash << 5) + hash) + data[i];
    }
    return hash;
  }

  X_HASHTABLE_API size_t x_hashtable_hash_cstr(const void* key, size_t _)
  {
    const char* str;
    size_t len;

    X_UNUSED(_);
    str = (const char*)key;
    len = strlen(str);
    return x_hashtable_hash_bytes(str, len);
  }

  X_HASHTABLE_API bool x_hashtable_compare_cstr(const void* a, const void* b)
  {
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcmp(str_a, str_b) == 0;
  }

  X_HASHTABLE_API void x_hashtable_free_cstr(void* a)
  {
    X_HASHTABLE_FREE(*(char**)a);
  }

  X_HASHTABLE_API void x_hashtable_clone_cstr(void* dest, const void* src)
  {
    char* mem =  
#if defined(_MSC_VER)
      _strdup((const char*) src);
#else
    strdup((const char*) src);
#endif
    *(char**)dest = mem;
  }

  X_HASHTABLE_API static bool x_hashtable_resize(XHashtable* table, size_t new_capacity);

  X_HASHTABLE_API XHashtable* x_hashtable_create_ex(size_t key_size, bool key_null_terminated, bool key_is_pointer,
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

    if (!ht)
    {
      return NULL;
    }

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
    size_t capacity = X_HASHTABLE_INITIAL_CAPACITY;

    if (!table)
    {
      return NULL;
    }

    table->entries = (XHashEntry*)X_HASHTABLE_CALLOC(capacity, sizeof(XHashEntry));
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
    table->key_is_pointer = false;
    table->value_is_pointer = false;
    table->key_is_null_terminated = false;
    table->value_is_null_terminated = false;

    table->fn_key_hash = fn_key_hash;
    table->fn_key_compare = fn_key_compare;
    table->fn_key_copy = fn_key_copy;
    table->fn_key_free = fn_key_free;
    table->fn_value_copy = fn_value_copy;
    table->fn_value_free = fn_value_free;

    return table;
  }

  X_HASHTABLE_API static size_t probe_index(XHashtable* table, const void* key, size_t *out_index_found, bool *found)
  {
    size_t capacity = table->capacity;
    size_t hash = table->fn_key_hash(key, table->key_size);
    size_t index = hash % capacity;
    size_t first_deleted = (size_t)-1;
    size_t i;

    for (i = 0; i < capacity; i++)
    {
      size_t idx = (index + i) % capacity;
      XHashEntry* entry = &table->entries[idx];

      if (entry->state == X_HASH_ENTRY_FREE)
      {
        if (first_deleted != (size_t)-1)
        {
          *out_index_found = first_deleted;
        }
        else
        {
          *out_index_found = idx;
        }

        *found = false;
        return idx;
      }
      else if (entry->state == X_HASH_ENTRY_DELETED)
      {
        if (first_deleted == (size_t)-1)
        {
          first_deleted = idx;
        }
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

    *out_index_found = (size_t)-1;
    *found = false;
    return (size_t)-1;
  }

  X_HASHTABLE_API bool x_hashtable_set(XHashtable* table, const void* key, const void* value)
  {
    size_t idx;
    bool found;
    XHashEntry* entry;

    if (!table || !key)
    {
      return false;
    }

    if ((double)(table->count + 1) / table->capacity > X_HASHTABLE_LOAD_FACTOR)
    {
      if (!x_hashtable_resize(table, table->capacity * 2))
      {
        return false;
      }
    }

    probe_index(table,
        (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
        &idx, &found);
    if (idx == (size_t)-1)
    {
      return false;
    }

    entry = &table->entries[idx];

    if (found)
    {
      void* value_ptr = value_at(table, idx);
      if (table->fn_value_free)
      {
        table->fn_value_free(value_ptr);
      }
      if (table->fn_value_copy)
      {
        table->fn_value_copy(value_ptr, value);
      }
      else
      {
        memcpy(value_ptr,
            (table->value_is_pointer && !table->value_is_null_terminated) ? &value : value,
            table->value_size);
      }
    }
    else
    {
      if (table->fn_key_copy)
      {
        table->fn_key_copy(key_at(table, idx), key);
      }
      else
      {
        memcpy(key_at(table, idx),
            (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
            table->key_size);
      }

      if (table->fn_value_copy)
      {
        table->fn_value_copy(value_at(table, idx), value);
      }
      else
      {
        memcpy(value_at(table, idx),
            (table->value_is_pointer && !table->value_is_null_terminated) ? &value : value,
            table->value_size);
      }

      entry->state = X_HASH_ENTRY_OCCUPIED;
      table->count++;
    }

    return true;
  }

  X_HASHTABLE_API bool x_hashtable_get(XHashtable* table, const void* key, void* out_value)
  {
    size_t idx;
    bool found;

    if (!table || !key || !out_value)
    {
      return false;
    }

    probe_index(table,
        (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
        &idx, &found);

    if (found)
    {
      void* value_ptr = value_at(table, idx);
      memcpy(out_value,
          (table->value_is_pointer && !table->value_is_null_terminated) ? &value_ptr : value_ptr,
          table->value_size);
      return true;
    }
    return false;
  }

  X_HASHTABLE_API bool x_hashtable_has(XHashtable* table, const void* key)
  {
    size_t idx;
    bool found;

    if (!table || !key)
    {
      return false;
    }

    probe_index(table,
        (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
        &idx, &found);
    return found;
  }

  X_HASHTABLE_API bool x_hashtable_remove(XHashtable* table, const void* key)
  {
    size_t idx;
    bool found;
    XHashEntry* entry;
    void* key_ptr;
    void* value_ptr;

    if (!table || !key)
    {
      return false;
    }

    probe_index(table,
        (table->key_is_pointer && !table->key_is_null_terminated) ? &key : key,
        &idx, &found);
    if (!found)
    {
      return false;
    }

    entry = &table->entries[idx];
    key_ptr = key_at(table, idx);
    value_ptr = value_at(table, idx);

    if (table->fn_key_free)
    {
      table->fn_key_free(key_ptr);
    }
    if (table->fn_value_free)
    {
      table->fn_value_free(value_ptr);
    }

    entry->state = X_HASH_ENTRY_DELETED;
    table->count--;
    return true;
  }

  X_HASHTABLE_API void x_hashtable_destroy(XHashtable* table)
  {
    size_t i;

    if (!table)
    {
      return;
    }

    for (i = 0; i < table->capacity; i++)
    {
      if (table->entries[i].state == X_HASH_ENTRY_OCCUPIED)
      {
        void* key_ptr = key_at(table, i);
        void* value_ptr = value_at(table, i);

        if (table->fn_key_free)
        {
          table->fn_key_free(key_ptr);
        }
        if (table->fn_value_free)
        {
          table->fn_value_free(value_ptr);
        }
      }
    }

    X_HASHTABLE_FREE(table->entries);
    X_HASHTABLE_FREE(table->keys);
    X_HASHTABLE_FREE(table->values);
    X_HASHTABLE_FREE(table);
  }

  X_HASHTABLE_API size_t x_hashtable_count(const XHashtable* table)
  {
    return table ? table->count : 0;
  }

  X_HASHTABLE_API static bool x_hashtable_resize(XHashtable* table, size_t new_capacity)
  {
    XHashEntry* new_entries;
    void* new_keys;
    void* new_values;
    XHashEntry* old_entries;
    void* old_keys;
    void* old_values;
    size_t old_capacity;
    size_t i;

    if (!table || new_capacity <= table->capacity)
    {
      return false;
    }

    new_entries = (XHashEntry*)X_HASHTABLE_CALLOC(new_capacity, sizeof(XHashEntry));
    new_keys = X_HASHTABLE_ALLOC(table->key_size * new_capacity);
    new_values = X_HASHTABLE_ALLOC(table->value_size * new_capacity);

    if (!new_entries || !new_keys || !new_values)
    {
      X_HASHTABLE_FREE(new_entries);
      X_HASHTABLE_FREE(new_keys);
      X_HASHTABLE_FREE(new_values);
      return false;
    }

    old_entries = table->entries;
    old_keys = table->keys;
    old_values = table->values;
    old_capacity = table->capacity;

    table->entries = new_entries;
    table->keys = new_keys;
    table->values = new_values;
    table->capacity = new_capacity;
    table->count = 0;

    for (i = 0; i < old_capacity; i++)
    {
      if (old_entries[i].state == X_HASH_ENTRY_OCCUPIED)
      {
        void* key_ptr = (char*)old_keys + i * table->key_size;
        void* value_ptr = (char*)old_values + i * table->value_size;

        x_hashtable_set(table,
            table->key_is_null_terminated ? *(void**)key_ptr : (table->key_is_pointer ? *(void**)key_ptr : key_ptr),
            table->value_is_null_terminated ? *(void**)value_ptr : (table->value_is_pointer ? *(void**)value_ptr : value_ptr));

        if (table->fn_key_free)
        {
          table->fn_key_free(key_ptr);
        }
        if (table->fn_value_free)
        {
          table->fn_value_free(value_ptr);
        }
      }
    }

    X_HASHTABLE_FREE(old_entries);
    X_HASHTABLE_FREE(old_keys);
    X_HASHTABLE_FREE(old_values);

    return true;
  }

  X_HASHTABLE_API static inline const void* x_hashtable__user_key_ptr(XHashtable* t, size_t i)
  {
    void* slot = key_at(t, i);

    if (t->key_is_pointer || t->key_is_null_terminated)
    {
      return *(void**)slot;
    }

    return slot;
  }

  X_HASHTABLE_API static inline void* x_hashtable__user_value_ptr(XHashtable* t, size_t i)
  {
    void* slot = value_at(t, i);

    if (t->value_is_pointer || t->value_is_null_terminated)
    {
      return *(void**)slot;
    }

    return slot;
  }

  X_HASHTABLE_API bool x_hashtable_iter_begin(XHashtable* table, XHashtableIter* it)
  {
    if (!table || !it)
    {
      return false;
    }

    it->table = table;
    it->index = (size_t)-1;
    return true;
  }

  X_HASHTABLE_API bool x_hashtable_iter_next(XHashtableIter* it, void** out_key, void** out_value)
  {
    XHashtable* t;
    size_t start;
    size_t i;

    if (!it || !it->table || !out_key || !out_value)
    {
      return false;
    }

    t = it->table;

    start = it->index;
    if (start == (size_t)-1)
    {
      start = 0;
    }
    else
    {
      start = start + 1;
    }

    for (i = start; i < t->capacity; i++)
    {
      if (t->entries[i].state == X_HASH_ENTRY_OCCUPIED)
      {
        it->index = i;
        *out_key = (void*)x_hashtable__user_key_ptr(t, i);
        *out_value = x_hashtable__user_value_ptr(t, i);
        return true;
      }
    }

    it->index = t->capacity;
    return false;
  }

  X_HASHTABLE_API bool x_hashtable_iter_next_const(XHashtableIter* it, const void** out_key, const void** out_value)
  {
    void* v = NULL;
    bool ok;

    if (!it || !out_key || !out_value)
    {
      return false;
    }

    ok = x_hashtable_iter_next(it, (void**)out_key, &v);
    if (!ok)
    {
      return false;
    }

    *out_value = (const void*)v;
    return true;
  }

  X_HASHTABLE_API size_t x_hashtable_iter_slot(const XHashtableIter* it)
  {
    if (!it || !it->table)
    {
      return 0;
    }

    return it->index;
  }

#ifdef __cplusplus
}
#endif

#endif /* X_IMPL_HASHTABLE */
#endif /* X_HASHTABLE_H */
