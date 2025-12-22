/**
 * STDX - Array
 * Part of the STDX General Purpose C Library by marciovmf
 * <https://github.com/marciovmf/stdx>
 * License: MIT
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
 * ## Dependencies
 *  stdx_common.h
 */

#ifndef X_ARRAY_H
#define X_ARRAY_H

#include <stdx_common.h>
#include <stdint.h>
#include <stdbool.h>

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
  XArray* x_array_create(size_t elementSize, size_t capacity);

  /**
   * @brief Get a pointer to the element at the given index.
   * @param arr Pointer to the array.
   * @param index Zero-based index of the element.
   * @return Pointer to the element at the given index.
   */
  XPtr x_array_get(XArray* arr, unsigned int index);

  /**
   * @brief Get a pointer to the underlying data buffer.
   * @param arr Pointer to the array.
   * @return Pointer to the internal contiguous data storage.
   */
  XPtr x_array_getdata(XArray* arr);

  /**
   * @brief Append an element to the end of the array.
   * @param arr Pointer to the array.
   * @param data Pointer to the element data to copy into the array.
   * @return Error code indicating success or failure.
   */
  XArrayError x_array_add(XArray* arr, void* data);

  /**
   * @brief Insert an element at the specified index.
   * @param arr Pointer to the array.
   * @param data Pointer to the element data to copy into the array.
   * @param index Index at which the element will be inserted.
   * @return Error code indicating success or failure.
   */
  XArrayError x_array_insert(XArray* arr, void* data, unsigned int index);

  /**
   * @brief Delete a range of elements from the array.
   * @param arr Pointer to the array.
   * @param start Index of the first element to delete.
   * @param end Index one past the last element to delete.
   * @return Error code indicating success or failure.
   */
  XArrayError x_array_delete_range(XArray* arr, unsigned int start, unsigned int end);

  /**
   * @brief Remove all elements from the array without freeing its storage.
   * @param arr Pointer to the array.
   * @return Nothing.
   */
  void x_array_clear(XArray* arr);

  /**
   * @brief Delete the element at the specified index.
   * @param arr Pointer to the array.
   * @param index Index of the element to remove.
   * @return Nothing.
   */
  void x_array_delete_at(XArray* arr, unsigned int index);

  /**
   * @brief Destroy the array and free all associated memory.
   * @param arr Pointer to the array.
   * @return Nothing.
   */
  void x_array_destroy(XArray* arr);

  /**
   * @brief Get the number of elements currently stored in the array.
   * @param arr Pointer to the array.
   * @return Number of elements in the array.
   */
  uint32_t x_array_count(XArray* arr);

  /**
   * @brief Get the current capacity of the array.
   * @param arr Pointer to the array.
   * @return Maximum number of elements the array can hold without resizing.
   */
  uint32_t x_array_capacity(XArray* arr);

  /**
   * @brief Push an element onto the end of the array.
   * @param array Pointer to the array.
   * @param value Pointer to the element data to copy into the array.
   * @return Nothing.
   */
  void x_array_push(XArray* array, void* value);

  /**
   * @brief Remove the last element from the array.
   * @param array Pointer to the array.
   * @return Nothing.
   */
  void x_array_pop(XArray* array);

  /**
   * @brief Get a pointer to the last element in the array.
   * @param array Pointer to the array.
   * @return Pointer to the last element.
   */
  XPtr x_array_top(XArray* array);

  /**
   * @brief Check whether the array is empty.
   * @param array Pointer to the array.
   * @return True if the array contains no elements, false otherwise.
   */
  bool x_array_is_empty(XArray* array);

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

    arr->array = X_ARRAY_ALLOC(capacity * elementSize);
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
      arr->array = X_ARRAY_REALLOC(arr->array, arr->capacity * arr->elementSize);
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

  XArrayError x_array_insert(XArray* arr, void* data, unsigned int index)
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
      arr->array = X_ARRAY_REALLOC(arr->array, arr->capacity * arr->elementSize);
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

  XPtr x_array_get(XArray* arr, unsigned int index)
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

    X_ARRAY_FREE(arr->array);
    X_ARRAY_FREE(arr);
  }

  XArrayError x_array_delete_range(XArray* arr, unsigned int start, unsigned int end)
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

  void x_array_delete_at(XArray* arr, unsigned int index)
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

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_ARRAY
#endif // X_ARRAY_H
