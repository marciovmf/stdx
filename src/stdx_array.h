/*
 * STDX - Dynamic Array
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a generic, dynamic array implementation with support for
 * random access, insertion, deletion, and stack-like operations 
 * (push/pop). Useful for managing homogeneous collections in C with
 * automatic resizing.
 *
 * Header-only and modular. Designed for performance and flexibility.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_ARRAY
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Dependencies: stdx_common.h
 * Usage: #include "stdx_array.h"
 */

#ifndef STDX_ARRAY_H
#define STDX_ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#define STDX_ARRAY_VERSION_MAJOR 1
#define STDX_ARRAY_VERSION_MINOR 0
#define STDX_ARRAY_VERSION_PATCH 0

#define STDX_ARRAY_VERSION (STDX_ARRAY_VERSION_MAJOR * 10000 + STDX_ARRAY_VERSION_MINOR * 100 + STDX_ARRAY_VERSION_PATCH)

#include <stdx_common.h>
#include <stdint.h>
#include <stdbool.h>

  typedef enum
  {
    XARRAY_OK                         = 0,
    XARRAY_INVALID_RANGE              = 1,
    XARRAY_MEMORY_ALLOCATION_FAILED   = 2,
    XARRAY_INDEX_OUT_OF_BOUNDS        = 3,
    XARRAY_EMPTY                      = 4

  } XArrayError;

  typedef struct XArray_t XArray;

  XArray*       x_array_create(size_t elementSize, size_t capacity);
  XPtr          x_array_get(XArray* arr, size_t index);
  XPtr          x_array_getdata(XArray* arr);
  XArrayError   x_array_add(XArray* arr, void* data);
  XArrayError   x_array_insert(XArray* arr, void* data, size_t index);
  XArrayError   x_array_delete_range(XArray* arr, size_t start, size_t end);
  void          x_array_clear(XArray* arr);
  void          x_array_delete_at(XArray* arr, size_t index);
  void          x_array_destroy(XArray* arr);  
  uint32_t      x_array_count(XArray* arr);
  uint32_t      x_array_capacity(XArray* arr);
  void          x_array_push(XArray* array, void* value);
  void          x_array_pop(XArray* array);
  XPtr          x_array_top(XArray* array);
  bool          x_array_is_empty(XArray* array);


#ifdef STDX_IMPLEMENTATION_ARRAY

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

  struct XArray_t
  {
    void *array;
    size_t size;
    size_t capacity;
    size_t elementSize;
  };

  XArray* x_array_create(size_t elementSize, size_t capacity)
  {
    XArray* arr = (XArray*) malloc(sizeof(XArray));
    if (arr == NULL)
    {
      return NULL;
    }

    arr->array = malloc(capacity * elementSize);
    arr->size = 0;
    arr->capacity = capacity;
    arr->elementSize = elementSize;

    X_ASSERT(capacity > 0);
    return arr;
  }

  XArrayError x_array_add(XArray* arr, void* data)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);

    if (arr->size >= arr->capacity)
    {
      arr->capacity = arr->capacity == 0 ? arr->capacity : arr->capacity * 2;
      arr->array = realloc(arr->array, arr->capacity * arr->elementSize);
      if (!arr->array)
      {
        return XARRAY_MEMORY_ALLOCATION_FAILED;
      }
    }

    if (data != NULL)
      memcpy((uint8_t*)arr->array + (arr->size * arr->elementSize), data, arr->elementSize);

    arr->size++;
    return XARRAY_OK;
  }

  XArrayError x_array_insert(XArray* arr, void* data, size_t index)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);

    if (index > arr->size)
    {
      return XARRAY_INDEX_OUT_OF_BOUNDS;
    }

    if (arr->size >= arr->capacity)
    {
      arr->capacity = arr->capacity == 0 ? 1 : arr->capacity * 2;
      arr->array = realloc(arr->array, arr->capacity * arr->elementSize);
      if (!arr->array)
      {
        return XARRAY_MEMORY_ALLOCATION_FAILED;
      }
    }

    memmove((uint8_t*)arr->array + ((index + 1) * arr->elementSize),
        (uint8_t*)arr->array + (index * arr->elementSize),
        (arr->size - index) * arr->elementSize);
    memcpy((uint8_t*)arr->array + (index * arr->elementSize), data, arr->elementSize);
    arr->size++;
    return XARRAY_OK;
  }

  XPtr x_array_get(XArray* arr, size_t index)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);
    if (index >= arr->size)
    {
      return X_PTR_ERR(XARRAY_INDEX_OUT_OF_BOUNDS);
    }

    return X_PTR_OK((uint8_t*)arr->array + (index * arr->elementSize));
  }

  void x_array_destroy(XArray* arr)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);

    free(arr->array);
    free(arr);
  }

  XArrayError x_array_delete_range(XArray* arr, size_t start, size_t end)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);

    if (start >= arr->size || end >= arr->size || start > end)
    {
      return XARRAY_INVALID_RANGE;
    }

    size_t deleteCount = end - start + 1;
    memmove(
        (uint8_t*)arr->array + (start * arr->elementSize),       // Destination
        (uint8_t*)arr->array + ((end + 1) * arr->elementSize),   // Source
        (arr->size - end - 1) * arr->elementSize);            // Size
    arr->size -= deleteCount;
    return XARRAY_OK;
  }

  void x_array_clear(XArray* arr)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);
    arr->size = 0;
  }

  uint32_t x_array_count(XArray* arr)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);
    return (uint32_t) arr->size;
  }

  uint32_t x_array_capacity(XArray* arr)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);
    return (uint32_t) arr->capacity;
  }

  void x_array_delete_at(XArray* arr, size_t index)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);
    x_array_delete_range(arr, index, index);
  }

  XPtr x_array_getdata(XArray* arr)
  {
    X_ASSERT(arr->array != NULL);
    X_ASSERT(arr->capacity > 0);
    return X_PTR_OK(arr->array);
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

  XPtr x_array_top(XArray* array)
  {
    uint32_t count = x_array_count(array);
    if (count == 0) return X_PTR_ERR(XARRAY_EMPTY);
    return x_array_get(array, count - 1);
  }

  bool x_array_is_empty(XArray* array)
  {
    return x_array_count(array) == 0;
  }

#endif // STDX_IMPLEMENTATION_ARRAY

#ifdef __cplusplus
}
#endif

#endif // STDX_ARRAY_H
