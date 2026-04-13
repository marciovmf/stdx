/**
 * STDX - Generic Hashtable 
 * Part of the STDX General Purpose C Library by marciovmf 
 * License: MIT 
 * <https://github.com/marciovmf/stdx>
 *
 * ## Overview
 *
 *  Provides a generic, dynamic array implementation with support for
 *  random access, insertion, deletion, and stack-like operations 
 *  (push/pop). Useful for managing homogeneous collections in C with
 *  automatic resizing.
 *
 * ## How to compile
 *
 * To compile the implementation define `X_IMPL_ARRAY`
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * `X_ARRAY_ALLOC` / `X_ARRAY_REALLOC` / `X_ARRAY_FREE` before including.
 *
 * ## Typed usage (recommended)
 *
 * The array core operates on `void*` elements and an explicit element size.
 * For convenience this header also provides macros that generate type-safe
 * wrappers on top of the generic API.
 *
 * These macros create:
 *
 * - a typed alias for `XArray`
 * - typed wrapper functions (`create`, `push`, `get`, `data`, `count`, etc.)
 *
 * This avoids manual casts and makes the API easier and safer to use.
 *
 * ### Example
 *
 * ```
 * X_ARRAY_TYPE(int)
 * XArray_int* arr = x_array_int_create(16);
 * x_array_int_push(arr, 10);
 * x_array_int_push(arr, 20);
 * int* value = x_array_int_get(arr, 1);
 * printf("%d\n", *value);
 * ```
 *
 * ### Named variants
 *
 * If the element type is not a valid identifier (for example
 * `unsigned int`, `const char*`, or complex types), use the named version:
 *
 * ```
 * X_ARRAY_TYPE_NAMED(unsigned int, uint)
 * XArray_uint* arr = x_array_uint_create(8);
 * ```
 *
 * The second argument defines the suffix used to generate the type
 * and function names.
 *
 * ### Example with struct types
 *
 * ```
 * typedef struct
 * {
 *   float x;
 *   float y;
 * } Point;
 *
 * X_ARRAY_TYPE(Point)
 * XArray_Point* points = x_array_Point_create(4);
 *
 * Point p = {1.0f, 2.0f};
 * x_array_Point_push(points, p);
 * ```
 *
 * Internally these wrappers call the generic API, so there is no
 * additional runtime overhead.
 *
 * ## Dependencies
 *
 *  stdx_common.h
 *
 */

#ifndef X_ARRAY_H
#define X_ARRAY_H

#include "stdx_common.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef X_ARRAY_API
#define X_ARRAY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define X_ARRAY_VERSION_MAJOR 1
#define X_ARRAY_VERSION_MINOR 0
#define X_ARRAY_VERSION_PATCH 0

#define X_ARRAY_VERSION (X_ARRAY_VERSION_MAJOR * 10000 + X_ARRAY_VERSION_MINOR * 100 + X_ARRAY_VERSION_PATCH)

  typedef enum
  {
    XARRAY_OK                         = 0,  /* Success */
    XARRAY_INVALID_RANGE              = 1,  /* Invalid range access */
    XARRAY_MEMORY_ALLOCATION_FAILED   = 2,
    XARRAY_INDEX_OUT_OF_BOUNDS        = 3,
    XARRAY_EMPTY                      = 4   /* Array is empty */
  } XArrayError;

  typedef struct XArray_t XArray;

  /**
   * @brief Create a new dynamic array.
   * @param elementSize Size in bytes of a single element stored in the array.
   * @param capacity Initial number of elements the array can hold.
   * @return Pointer to the newly created array, or NULL on failure.
   */
  X_ARRAY_API XArray* x_array_create(size_t elementSize, size_t capacity);

  /**
   * @brief Get a pointer to the element at the given index.
   * @param arr Pointer to the array.
   * @param index Zero-based index of the element.
   * @return Pointer to the element at the given index.
   */
  X_ARRAY_API void* x_array_get(XArray* arr, unsigned int index);

  /**
   * @brief Get a pointer to the underlying data buffer.
   * @param arr Pointer to the array.
   * @return Pointer to the internal contiguous data storage.
   */
  X_ARRAY_API void* x_array_data(XArray* arr);

  /**
   * @brief Append an element to the end of the array.
   * @param arr Pointer to the array.
   * @param data Pointer to the element data to copy into the array.
   * @return Error code indicating success or failure.
   */
  X_ARRAY_API XArrayError x_array_add(XArray* arr, void* data);

  /**
   * @brief Insert an element at the specified index.
   * @param arr Pointer to the array.
   * @param data Pointer to the element data to copy into the array.
   * @param index Index at which the element will be inserted.
   * @return Error code indicating success or failure.
   */
  X_ARRAY_API XArrayError x_array_insert(XArray* arr, void* data, unsigned int index);

  /**
   * @brief Delete a range of elements from the array.
   * @param arr Pointer to the array.
   * @param start Index of the first element to delete.
   * @param end Index of the last element to delete (inclusive).
   * @return Error code indicating success or failure.
   */
  X_ARRAY_API XArrayError x_array_delete_range(XArray* arr, unsigned int start, unsigned int end);

  /**
   * @brief Remove all elements from the array without freeing its storage.
   * @param arr Pointer to the array.
   * @return Nothing.
   */
  X_ARRAY_API void x_array_clear(XArray* arr);

  /**
   * @brief Delete the element at the specified index.
   * @param arr Pointer to the array.
   * @param index Index of the element to remove.
   * @return Nothing.
   */
  X_ARRAY_API void x_array_delete_at(XArray* arr, unsigned int index);

  /**
   * @brief Destroy the array and free all associated memory.
   * @param arr Pointer to the array.
   * @return Nothing.
   */
  X_ARRAY_API void x_array_destroy(XArray* arr);

  /**
   * @brief Get the number of elements currently stored in the array.
   * @param arr Pointer to the array.
   * @return Number of elements in the array.
   */
  X_ARRAY_API uint32_t x_array_count(XArray* arr);

  /**
   * @brief Get the current capacity of the array.
   * @param arr Pointer to the array.
   * @return Maximum number of elements the array can hold without resizing.
   */
  X_ARRAY_API uint32_t x_array_capacity(XArray* arr);

  /**
   * @brief Push an element onto the end of the array.
   * @param array Pointer to the array.
   * @param value Pointer to the element data to copy into the array.
   * @return Nothing.
   */
  X_ARRAY_API void x_array_push(XArray* array, void* value);

  /**
   * @brief Remove the last element from the array.
   * @param array Pointer to the array.
   * @return Nothing.
   */
  X_ARRAY_API void x_array_pop(XArray* array);

  /**
   * @brief Get a pointer to the last element in the array.
   * @param array Pointer to the array.
   * @return Pointer to the last element.
   */
  X_ARRAY_API void* x_array_back(XArray* array);

  /**
   * @brief Check whether the array is empty.
   * @param array Pointer to the array.
   * @return True if the array contains no elements, false otherwise.
   */
  X_ARRAY_API bool x_array_is_empty(XArray* array);


  /* Declare a typed array alias and typed inline wrappers. */
#define X_ARRAY_TYPE(T) \
  X_ARRAY_TYPE_NAMED(T, T)

  /* Use this form for multi-token types such as unsigned int or struct Foo. */
#define X_ARRAY_TYPE_NAMED(T, suffix) \
  typedef XArray XArray_##suffix; \
  static inline XArray_##suffix* x_array_##suffix##_create(size_t capacity) \
  { \
    return (XArray_##suffix*)x_array_create(sizeof(T), capacity); \
  } \
  static inline void x_array_##suffix##_destroy(XArray_##suffix* arr) \
  { \
    x_array_destroy((XArray*)arr); \
  } \
  static inline XArrayError x_array_##suffix##_add_ptr(XArray_##suffix* arr, const T* value_ptr) \
  { \
    return x_array_add((XArray*)arr, (void*)value_ptr); \
  } \
  static inline XArrayError x_array_##suffix##_add(XArray_##suffix* arr, T value) \
  { \
    T value_copy = value; \
    return x_array_add((XArray*)arr, &value_copy); \
  } \
  static inline XArrayError x_array_##suffix##_insert_ptr(XArray_##suffix* arr, const T* value_ptr, unsigned int index) \
  { \
    return x_array_insert((XArray*)arr, (void*)value_ptr, index); \
  } \
  static inline XArrayError x_array_##suffix##_insert(XArray_##suffix* arr, T value, unsigned int index) \
  { \
    T value_copy = value; \
    return x_array_insert((XArray*)arr, &value_copy, index); \
  } \
  static inline XArrayError x_array_##suffix##_push_ptr(XArray_##suffix* arr, const T* value_ptr) \
  { \
    return x_array_add((XArray*)arr, (void*)value_ptr); \
  } \
  static inline XArrayError x_array_##suffix##_push(XArray_##suffix* arr, T value) \
  { \
    T value_copy = value; \
    return x_array_add((XArray*)arr, &value_copy); \
  } \
  static inline void x_array_##suffix##_pop(XArray_##suffix* arr) \
  { \
    x_array_pop((XArray*)arr); \
  } \
  static inline T* x_array_##suffix##_get(XArray_##suffix* arr, unsigned int index) \
  { \
    return (T*)x_array_get((XArray*)arr, index); \
  } \
  static inline const T* x_array_##suffix##_get_const(const XArray_##suffix* arr, unsigned int index) \
  { \
    return (const T*)x_array_get((XArray*)arr, index); \
  } \
  static inline T* x_array_##suffix##_data(XArray_##suffix* arr) \
  { \
    return (T*)x_array_data((XArray*)arr); \
  } \
  static inline const T* x_array_##suffix##_data_const(const XArray_##suffix* arr) \
  { \
    return (const T*)x_array_data((XArray*)arr); \
  } \
  static inline T* x_array_##suffix##_back(XArray_##suffix* arr) \
  { \
    return (T*)x_array_back((XArray*)arr); \
  } \
  static inline const T* x_array_##suffix##_back_const(const XArray_##suffix* arr) \
  { \
    return (const T*)x_array_back((XArray*)arr); \
  } \
  static inline void x_array_##suffix##_clear(XArray_##suffix* arr) \
  { \
    x_array_clear((XArray*)arr); \
  } \
  static inline void x_array_##suffix##_delete_at(XArray_##suffix* arr, unsigned int index) \
  { \
    x_array_delete_at((XArray*)arr, index); \
  } \
  static inline XArrayError x_array_##suffix##_delete_range(XArray_##suffix* arr, unsigned int start, unsigned int end) \
  { \
    return x_array_delete_range((XArray*)arr, start, end); \
  } \
  static inline uint32_t x_array_##suffix##_count(XArray_##suffix* arr) \
  { \
    return x_array_count((XArray*)arr); \
  } \
  static inline uint32_t x_array_##suffix##_capacity(XArray_##suffix* arr) \
  { \
    return x_array_capacity((XArray*)arr); \
  } \
  static inline bool x_array_##suffix##_is_empty(XArray_##suffix* arr) \
  { \
    return x_array_is_empty((XArray*)arr); \
  }

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_ARRAY

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef X_ARRAY_ALLOC
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_ARRAY_ALLOC(sz) malloc(sz)
#endif

#ifndef X_ARRAY_REALLOC

/**
 * @brief Internal macro for resizing memory.
 * To override how this header resizes memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to resize.
 */
#define X_ARRAY_REALLOC(p,sz) realloc((p),(sz))
#endif

#ifndef X_ARRAY_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_ARRAY_FREE(p) free(p)
#endif


#ifdef __cplusplus
extern "C" {
#endif

  struct XArray_t
  {
    void *array;
    size_t size;
    size_t capacity;
    size_t elementSize;
  };

  XArray* x_array_create(size_t elementSize, size_t capacity)
  {
    XArray* arr = (XArray*) X_ARRAY_ALLOC(sizeof(XArray));
    if (arr == NULL)
    {
      return NULL;
    }

    arr->array = NULL;
    arr->size = 0;
    arr->capacity = capacity;
    arr->elementSize = elementSize;

    if (capacity > 0)
    {
      arr->array = X_ARRAY_ALLOC(capacity * elementSize);
      if (arr->array == NULL)
      {
        X_ARRAY_FREE(arr);
        return NULL;
      }
    }

    return arr;
  }

  XArrayError x_array_add(XArray* arr, void* data)
  {
    void* new_array;
    size_t new_capacity;

    X_ASSERT(arr != NULL);

    if (arr->size >= arr->capacity)
    {
      new_capacity = arr->capacity == 0 ? 1 : arr->capacity * 2;
      new_array = X_ARRAY_REALLOC(arr->array, new_capacity * arr->elementSize);
      if (new_array == NULL)
      {
        return XARRAY_MEMORY_ALLOCATION_FAILED;
      }

      arr->array = new_array;
      arr->capacity = new_capacity;
    }

    if (data != NULL)
    {
      memcpy((uint8_t*)arr->array + (arr->size * arr->elementSize), data, arr->elementSize);
    }

    arr->size++;
    return XARRAY_OK;
  }

  XArrayError x_array_insert(XArray* arr, void* data, unsigned int index)
  {
    void* new_array;
    size_t new_capacity;

    X_ASSERT(arr != NULL);

    if (index > arr->size)
    {
      return XARRAY_INDEX_OUT_OF_BOUNDS;
    }

    if (arr->size >= arr->capacity)
    {
      new_capacity = arr->capacity == 0 ? 1 : arr->capacity * 2;
      new_array = X_ARRAY_REALLOC(arr->array, new_capacity * arr->elementSize);
      if (new_array == NULL)
      {
        return XARRAY_MEMORY_ALLOCATION_FAILED;
      }

      arr->array = new_array;
      arr->capacity = new_capacity;
    }

    memmove((uint8_t*)arr->array + ((index + 1) * arr->elementSize),
      (uint8_t*)arr->array + (index * arr->elementSize),
      (arr->size - index) * arr->elementSize);

    if (data != NULL)
    {
      memcpy((uint8_t*)arr->array + (index * arr->elementSize), data, arr->elementSize);
    }

    arr->size++;
    return XARRAY_OK;
  }

  void* x_array_get(XArray* arr, unsigned int index)
  {
    X_ASSERT(arr != NULL);

    if (index >= arr->size)
    {
      return NULL;
    }

    return (uint8_t*)arr->array + (index * arr->elementSize);
  }

  void x_array_destroy(XArray* arr)
  {
    X_ASSERT(arr != NULL);

    X_ARRAY_FREE(arr->array);
    X_ARRAY_FREE(arr);
  }

  XArrayError x_array_delete_range(XArray* arr, unsigned int start, unsigned int end)
  {
    size_t delete_count;

    X_ASSERT(arr != NULL);

    if (start >= arr->size || end >= arr->size || start > end)
    {
      return XARRAY_INVALID_RANGE;
    }

    delete_count = end - start + 1;

    memmove(
      (uint8_t*)arr->array + (start * arr->elementSize),
      (uint8_t*)arr->array + ((end + 1) * arr->elementSize),
      (arr->size - end - 1) * arr->elementSize);

    arr->size -= delete_count;
    return XARRAY_OK;
  }

  void x_array_clear(XArray* arr)
  {
    X_ASSERT(arr != NULL);
    arr->size = 0;
  }

  uint32_t x_array_count(XArray* arr)
  {
    X_ASSERT(arr != NULL);
    return (uint32_t) arr->size;
  }

  uint32_t x_array_capacity(XArray* arr)
  {
    X_ASSERT(arr != NULL);
    return (uint32_t) arr->capacity;
  }

  void x_array_delete_at(XArray* arr, unsigned int index)
  {
    X_ASSERT(arr != NULL);
    x_array_delete_range(arr, index, index);
  }

  void* x_array_data(XArray* arr)
  {
    X_ASSERT(arr != NULL);
    return arr->array;
  }

  void x_array_push(XArray* array, void* value)
  {
    x_array_add(array, value);
  }

  void x_array_pop(XArray* array)
  {
    uint32_t count = x_array_count(array);
    if (count > 0) {
      x_array_delete_at(array, count - 1);
    }
  }

  void* x_array_back(XArray* array)
  {
    uint32_t count = x_array_count(array);

    if (count == 0)
    {
      return NULL;
    }

    return x_array_get(array, count - 1);
  }

  bool x_array_is_empty(XArray* array)
  {
    return x_array_count(array) == 0;
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_ARRAY
#endif // X_ARRAY_H
