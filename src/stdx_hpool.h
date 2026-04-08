/**
 * STDX - Handle Pool
 * Part of the STDX General Purpose C Library by marciovmf
 * License: MIT
 * <https://github.com/marciovmf/stdx>
 *
 * ## Overview
 *
 *  A paged object pool with versioned handles and fast iteration.
 *
 *  This module provides a stable storage container where objects are
 *  referenced through lightweight handles instead of raw pointers.
 *  It is designed for systems that create and destroy objects frequently
 *  while still needing
 *   - O(1) allocation and free
 *   - safe handle validation
 *   - stable storage (objects are never moved)
 *   - fast iteration over live objects
 *
 *  Implementation details:
 *   - **Paged storage**
 *       Objects are stored in pages. New pages are allocated as needed,
 *       and existing objects never move in memory.
 *
 *   - **Versioned handles**
 *       A handle is `{ index, version }`. When a slot is freed the version
 *       is incremented, automatically invalidating any stale handles that
 *       still reference the old object.
 *
 *   - **Free list**
 *       Freed slots are recycled through a linked free list, allowing
 *       constant-time allocation without scanning.
 *
 *   - **Alive list**
 *       Live objects are tracked in a dense array so iteration does not
 *       need to scan empty slots.
 *
 *
 *  Key properties:
 *
 *   - Handle = `{ index, version }`
 *   - Storage grows by pages (no moving existing items)
 *   - Free is per-item (free list) + version bump to kill stale handles
 *   - Iteration is dense over `alive[]` (no hole scanning)
 *
 *
 * ## Basic Usage
 *
 *  1) Define the pool
 *
 *      XHPool pool;
 *
 *  2) Initialize it
 *
 *      XHPoolConfig cfg = {0};
 *      cfg.page_capacity = 1024;   // slots per page (must be power of two)
 *      cfg.initial_pages = 1;
 *
 *      x_hpool_init(&pool, sizeof(MyType), cfg, NULL, NULL, NULL);
 *
 *  3) Allocate an object
 *
 *      XHandle h = x_hpool_alloc(&pool);
 *
 *      MyType* obj = (MyType*)x_hpool_get(&pool, h);
 *
 *  4) Use the object normally through the returned pointer.
 *
 *  5) Free the object
 *
 *      x_hpool_free(&pool, h);
 *
 *
 * ## Iterating Over Live Objects
 *
 *  Iteration skips empty slots and visits only live items.
 *
 *      XHPoolIter it;
 *      XHandle h;
 *      MyType* obj;
 *
 *      for (obj = x_hpool_iter_begin(&pool, &it, &h);
 *           obj;
 *           obj = x_hpool_iter_next(&pool, &it, &h))
 *      {
 *          // use obj
 *      }
 *
 *  Or use the convenience macro:
 *
 *      X_HPOOL_FOREACH(&pool, MyType, it, h, obj)
 *      {
 *          // use obj
 *      }
 *
 *
 * ## Optional Constructors / Destructors
 *
 *  A constructor and destructor may be supplied when the pool is
 *  initialized. These functions are automatically invoked when items
 *  are allocated or freed.
 *
 *      void ctor(void* user, void* item);
 *      void dtor(void* user, void* item);
 *
 *      x_hpool_init(&pool, sizeof(MyType), cfg, ctor, dtor, user_ptr);
 *
 *
 * ## How to compile
 *
 *  To compile the implementation define `X_IMPL_HPOOL`
 *  in **one** source file before including this header.
 *
 *      #define X_IMPL_HPOOL
 *      #include "stdx_hpool.h"
 *
 *
 * ## Custom memory allocation
 *
 *  To override how this module allocates memory define the following
 *  macros before including this header:
 *
 *      X_HPOOL_ALLOC(sz)
 *      X_HPOOL_REALLOC(ptr, sz)
 *      X_HPOOL_FREE(ptr)
 *
 */

#ifndef STDX_HPOOL_H
#define STDX_HPOOL_H

#include "stdx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define X_HPOOL_VERSION_MAJOR 1
#define X_HPOOL_VERSION_MINOR 0
#define X_HPOOL_VERSION_PATCH 0

#define X_HPOOL_VERSION (X_HPOOL_VERSION_MAJOR * 10000 + X_HPOOL_VERSION_MINOR * 100 + X_HPOOL_VERSION_PATCH)

#ifndef X_HPOOL_API
#define X_HPOOL_API
#endif

#define X_HPOOL_NULL_INDEX 0xFFFFFFFFu

  typedef struct XHandle
  {
    uint32_t index;
    uint32_t version;
  } XHandle;

  typedef void (*XHPoolCtorFn)(void* user, void* item);

  typedef void (*XHPoolDtorFn)(void* user, void* item);

  typedef struct XHPoolConfig
  {
    uint32_t page_capacity;   // e.g. 1024
    uint32_t initial_pages;   // e.g. 1
  } XHPoolConfig;

  typedef struct XHPoolIter
  {
    uint32_t alive_pos;
  } XHPoolIter;

  typedef struct XHPool
  {
    size_t item_size;
    uint32_t page_capacity;
    uint32_t page_shift;
    uint32_t page_mask;

    void** pages;
    uint32_t page_count;
    uint32_t page_cap;

    uint32_t free_head;
    uint32_t next_index;

    uint32_t* alive;
    uint32_t alive_count;
    uint32_t alive_cap;

    XHPoolCtorFn ctor;
    XHPoolDtorFn dtor;
    void* user;
  } XHPool;

  X_HPOOL_API XHandle x_handle_null(void);
  X_HPOOL_API int x_handle_is_null(XHandle h);
  X_HPOOL_API int x_hpool_init(XHPool* p, size_t item_size, XHPoolConfig cfg, XHPoolCtorFn ctor, XHPoolDtorFn dtor, void* user);
  X_HPOOL_API void x_hpool_term(XHPool* p);
  X_HPOOL_API uint32_t x_hpool_page_capacity(XHPool* p);
  X_HPOOL_API uint32_t x_hpool_capacity(XHPool* p);
  X_HPOOL_API uint32_t x_hpool_alive_count(XHPool* p);
  X_HPOOL_API int x_hpool_is_alive(XHPool* p, XHandle h);
  X_HPOOL_API void* x_hpool_get(XHPool* p, XHandle h);
  X_HPOOL_API void* x_hpool_get_fast(XHPool* p, XHandle h);
  X_HPOOL_API void* x_hpool_get_unchecked(XHPool* p, uint32_t index);
  X_HPOOL_API XHandle x_hpool_alloc(XHPool* p);
  X_HPOOL_API void x_hpool_free(XHPool* p, XHandle h);
  X_HPOOL_API void x_hpool_clear(XHPool* p);
  X_HPOOL_API void* x_hpool_iter_begin(XHPool* p, XHPoolIter* it, XHandle* out_h);
  X_HPOOL_API void* x_hpool_iter_next(XHPool* p, XHPoolIter* it, XHandle* out_h);

#define X_HPOOL_FOREACH(pHPool, Type, itVar, hVar, ptrVar) \
  for (XHPoolIter itVar = {0}, *x__hpool_once_##itVar = &(itVar); x__hpool_once_##itVar != NULL; x__hpool_once_##itVar = NULL) \
  for ((ptrVar) = (Type*)x_hpool_iter_begin((pHPool), &(itVar), &(hVar)); (ptrVar) != NULL; (ptrVar) = (Type*)x_hpool_iter_next((pHPool), &(itVar), &(hVar)))

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_HPOOL

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef X_HPOOL_ALLOC
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_HPOOL_ALLOC(sz) malloc(sz)
#endif

#ifndef X_HPOOL_REALLOC
/**
 * @brief Internal macro for resizing memory.
 * To override how this header resizes memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to resize.
 */
#define X_HPOOL_REALLOC(p,sz) realloc((p),(sz))
#endif

#ifndef X_HPOOL_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_HPOOL_FREE(p) free(p)
#endif


#ifdef __cplusplus
extern "C" {
#endif
  typedef struct XHPoolSlotHeader
  {
    uint32_t version;
    uint32_t next_free;
    uint32_t alive_pos;
    uint32_t flags;
  } XHPoolSlotHeader;

#define X_POOL_SLOT_ALIVE 1u

  X_HPOOL_API XHandle x_handle_null(void)
  {
    XHandle h;
    h.index = X_HPOOL_NULL_INDEX;
    h.version = 0u;
    return h;
  }

  X_HPOOL_API static int x__hpool_is_pow2_u32(uint32_t v)
  {
    if (v == 0u)
    {
      return 0;
    }

    return (v & (v - 1u)) == 0u;
  }

  X_HPOOL_API static uint32_t x__hpool_ctz_u32(uint32_t v)
  {
    uint32_t n;

    n = 0u;
    while ((v & 1u) == 0u)
    {
      v = v >> 1u;
      n = n + 1u;
    }

    return n;
  }

  X_HPOOL_API static uint32_t s_hpool_page_index(const XHPool* p, uint32_t index);

  X_HPOOL_API static uint32_t s_hpool_slot_index(const XHPool* p, uint32_t index);

  X_HPOOL_API static size_t x_hpool_slot_stride(const XHPool* p);

  X_HPOOL_API static uint8_t* s_hpool_page_base(const XHPool* p, uint32_t page_index);

  X_HPOOL_API static XHPoolSlotHeader* s_hpool_slot_hdr(const XHPool* p, uint32_t index);

  X_HPOOL_API static void* s_hpool_slot_item(const XHPool* p, XHPoolSlotHeader* hdr);

  X_HPOOL_API static void* s_hpool_item_by_index(const XHPool* p, uint32_t index);

  X_HPOOL_API static int s_hpool_ensure_pages_array(XHPool* p, uint32_t want);

  X_HPOOL_API static int s_hpool_add_page(XHPool* p);

  X_HPOOL_API static int s_hpool_ensure_page_for_index(XHPool* p, uint32_t index);

  X_HPOOL_API static int s_hpool_alive_reserve(XHPool* p, uint32_t want);

  X_HPOOL_API static void s_hpool_alive_add(XHPool* p, uint32_t index);

  X_HPOOL_API int x_handle_is_null(XHandle h)
  {
    return h.index == X_HPOOL_NULL_INDEX;
  }

  X_HPOOL_API uint32_t x_hpool_page_index(const XHPool* p, uint32_t index)
  {
    return index >> p->page_shift;
  }

  X_HPOOL_API uint32_t x_hpool_slot_index(const XHPool* p, uint32_t index)
  {
    return index & p->page_mask;
  }

  X_HPOOL_API size_t x_hpool_slot_stride(const XHPool* p)
  {
    size_t s;
    s = sizeof(XHPoolSlotHeader) + p->item_size;
    return s;
  }

  X_HPOOL_API uint8_t* x_hpool_page_base(const XHPool* p, uint32_t page_index)
  {
    return (uint8_t*)p->pages[page_index];
  }

  X_HPOOL_API XHPoolSlotHeader* x_hpool_slot_hdr(const XHPool* p, uint32_t index)
  {
    uint32_t page_index;
    uint32_t slot_index;
    size_t stride;
    uint8_t* base;
    uint8_t* ptr;

    page_index = x_hpool_page_index(p, index);
    slot_index = x_hpool_slot_index(p, index);
    stride = x_hpool_slot_stride(p);
    base = x_hpool_page_base(p, page_index);
    ptr = base + (size_t)slot_index * stride;

    return (XHPoolSlotHeader*)ptr;
  }

  X_HPOOL_API void* x_hpool_slot_item(const XHPool* p, XHPoolSlotHeader* hdr)
  {
    uint8_t* ptr;
    ptr = (uint8_t*)hdr;
    ptr = ptr + sizeof(XHPoolSlotHeader);
    return (void*)ptr;
  }

  X_HPOOL_API void* x_hpool_item_by_index(const XHPool* p, uint32_t index)
  {
    XHPoolSlotHeader* hdr;
    hdr = x_hpool_slot_hdr(p, index);
    return x_hpool_slot_item(p, hdr);
  }

  X_HPOOL_API int x_hpool_ensure_pages_array(XHPool* p, uint32_t want)
  {
    void** new_pages;
    uint32_t new_cap;

    if (want <= p->page_cap)
    {
      return 1;
    }

    new_cap = p->page_cap;
    if (new_cap == 0u)
    {
      new_cap = 1u;
    }

    while (new_cap < want)
    {
      new_cap = new_cap * 2u;
    }

    new_pages = (void**)X_HPOOL_REALLOC(p->pages, (size_t)new_cap * sizeof(void*));
    if (new_pages == NULL)
    {
      return 0;
    }

    p->pages = new_pages;
    p->page_cap = new_cap;

    return 1;
  }

  X_HPOOL_API int x_hpool_add_page(XHPool* p)
  {
    uint32_t i;
    size_t stride;
    size_t bytes;
    void* mem;

    if (x_hpool_ensure_pages_array(p, p->page_count + 1u) == 0)
    {
      return 0;
    }

    stride = x_hpool_slot_stride(p);
    bytes = (size_t)p->page_capacity * stride;
    mem = X_HPOOL_ALLOC(bytes);
    if (mem == NULL)
    {
      return 0;
    }

    memset(mem, 0, bytes);

    /* init slot headers */
    for (i = 0u; i < p->page_capacity; i += 1u)
    {
      XHPoolSlotHeader* hdr;
      uint8_t* base;
      uint8_t* ptr;

      base = (uint8_t*)mem;
      ptr = base + (size_t)i * stride;
      hdr = (XHPoolSlotHeader*)ptr;

      hdr->version = 0u;
      hdr->next_free = X_HPOOL_NULL_INDEX;
      hdr->alive_pos = 0u;
      hdr->flags = 0u;
    }

    p->pages[p->page_count] = mem;
    p->page_count = p->page_count + 1u;

    return 1;
  }

  X_HPOOL_API int x_hpool_ensure_page_for_index(XHPool* p, uint32_t index)
  {
    uint32_t need_pages;

    need_pages = x_hpool_page_index(p, index) + 1u;

    while (p->page_count < need_pages)
    {
      if (x_hpool_add_page(p) == 0)
      {
        return 0;
      }
    }

    return 1;
  }

  X_HPOOL_API int x_hpool_alive_reserve(XHPool* p, uint32_t want)
  {
    uint32_t new_cap;
    uint32_t* new_arr;

    if (want <= p->alive_cap)
    {
      return 1;
    }

    new_cap = p->alive_cap;
    if (new_cap == 0u)
    {
      new_cap = 64u;
    }

    while (new_cap < want)
    {
      new_cap = new_cap * 2u;
    }

    new_arr = (uint32_t*)X_HPOOL_REALLOC(p->alive, (size_t)new_cap * sizeof(uint32_t));
    if (new_arr == NULL)
    {
      return 0;
    }

    p->alive = new_arr;
    p->alive_cap = new_cap;

    return 1;
  }

  X_HPOOL_API void x_hpool_alive_add(XHPool* p, uint32_t index)
  {
    XHPoolSlotHeader* hdr;

    hdr = x_hpool_slot_hdr(p, index);

    p->alive[p->alive_count] = index;
    hdr->alive_pos = p->alive_count;
    hdr->flags = hdr->flags | X_POOL_SLOT_ALIVE;

    p->alive_count = p->alive_count + 1u;
  }

  X_HPOOL_API void x_hpool_alive_remove(XHPool* p, uint32_t index)
  {
    XHPoolSlotHeader* hdr;
    uint32_t pos;
    uint32_t last_pos;
    uint32_t moved_index;
    XHPoolSlotHeader* moved_hdr;

    hdr = x_hpool_slot_hdr(p, index);
    if ((hdr->flags & X_POOL_SLOT_ALIVE) == 0u)
    {
      return;
    }

    pos = hdr->alive_pos;
    last_pos = p->alive_count - 1u;

    if (pos != last_pos)
    {
      moved_index = p->alive[last_pos];
      p->alive[pos] = moved_index;

      moved_hdr = x_hpool_slot_hdr(p, moved_index);
      moved_hdr->alive_pos = pos;
    }

    p->alive_count = p->alive_count - 1u;

    hdr->alive_pos = 0u;
    hdr->flags = hdr->flags & ~X_POOL_SLOT_ALIVE;
  }

  X_HPOOL_API int x_hpool_init(XHPool* p,
      size_t item_size,
      XHPoolConfig cfg,
      XHPoolCtorFn ctor,
      XHPoolDtorFn dtor,
      void* user)
  {
    uint32_t i;

    if (p == NULL)
    {
      return 0;
    }

    if (item_size == 0u)
    {
      return 0;
    }

    if (cfg.page_capacity == 0u)
    {
      cfg.page_capacity = 1024u;
    }

    if (x__hpool_is_pow2_u32(cfg.page_capacity) == 0)
    {
      return 0;
    }

    memset(p, 0, sizeof(*p));

    p->item_size = item_size;
    p->page_capacity = cfg.page_capacity;
    p->page_shift = x__hpool_ctz_u32(cfg.page_capacity);
    p->page_mask = cfg.page_capacity - 1u;
    p->free_head = X_HPOOL_NULL_INDEX;
    p->next_index = 0u;
    p->ctor = ctor;
    p->dtor = dtor;
    p->user = user;

    if (cfg.initial_pages == 0u)
    {
      cfg.initial_pages = 1u;
    }

    for (i = 0u; i < cfg.initial_pages; i += 1u)
    {
      if (x_hpool_add_page(p) == 0)
      {
        return 0;
      }
    }

    return 1;
  }

  X_HPOOL_API void x_hpool_term(XHPool* p)
  {
    uint32_t i;

    if (p == NULL)
    {
      return;
    }

    /* call dtors for live items */
    if (p->dtor != NULL)
    {
      for (i = 0u; i < p->alive_count; i += 1u)
      {
        uint32_t index;
        void* item;

        index = p->alive[i];
        item = x_hpool_item_by_index(p, index);
        p->dtor(p->user, item);
      }
    }

    for (i = 0u; i < p->page_count; i += 1u)
    {
      X_HPOOL_FREE(p->pages[i]);
    }

    X_HPOOL_FREE(p->pages);
    X_HPOOL_FREE(p->alive);

    memset(p, 0, sizeof(*p));
  }

  X_HPOOL_API uint32_t x_hpool_page_capacity(XHPool* p)
  {
    if (p == NULL)
    {
      return 0u;
    }

    return p->page_capacity;
  }

  X_HPOOL_API uint32_t x_hpool_capacity(XHPool* p)
  {
    if (p == NULL)
    {
      return 0u;
    }

    return p->page_count * p->page_capacity;
  }

  X_HPOOL_API uint32_t x_hpool_alive_count(XHPool* p)
  {
    if (p == NULL)
    {
      return 0u;
    }

    return p->alive_count;
  }

  X_HPOOL_API int x_hpool_is_alive(XHPool* p, XHandle h)
  {
    XHPoolSlotHeader* hdr;

    if (p == NULL)
    {
      return 0;
    }

    if (x_handle_is_null(h) != 0)
    {
      return 0;
    }

    if (h.index >= x_hpool_capacity(p))
    {
      return 0;
    }

    hdr = x_hpool_slot_hdr(p, h.index);
    if (hdr->version != h.version)
    {
      return 0;
    }

    if ((hdr->flags & X_POOL_SLOT_ALIVE) == 0u)
    {
      return 0;
    }

    return 1;
  }

  X_HPOOL_API void* x_hpool_get(XHPool* p, XHandle h)
  {
    XHPoolSlotHeader* hdr;

    if (p == NULL)
    {
      return NULL;
    }

    if (x_handle_is_null(h) != 0)
    {
      return NULL;
    }

    if (h.index >= x_hpool_capacity(p))
    {
      return NULL;
    }

    hdr = x_hpool_slot_hdr(p, h.index);
    if (hdr->version != h.version)
    {
      return NULL;
    }

    if ((hdr->flags & X_POOL_SLOT_ALIVE) == 0u)
    {
      return NULL;
    }

    return x_hpool_slot_item(p, hdr);
  }

  X_HPOOL_API void* x_hpool_get_fast(XHPool* p, XHandle h)
  {
    XHPoolSlotHeader* hdr;

    X_ASSERT(p != NULL);
    X_ASSERT(x_handle_is_null(h) == 0);
    X_ASSERT(h.index < x_hpool_capacity(p));

    hdr = x_hpool_slot_hdr(p, h.index);
    if (hdr->version != h.version)
    {
      return NULL;
    }

    if ((hdr->flags & X_POOL_SLOT_ALIVE) == 0u)
    {
      return NULL;
    }

    return x_hpool_slot_item(p, hdr);
  }

  X_HPOOL_API void* x_hpool_get_unchecked(XHPool* p, uint32_t index)
  {
    XHPoolSlotHeader* hdr;

    if (p == NULL)
    {
      return NULL;
    }

    if (index >= x_hpool_capacity(p))
    {
      return NULL;
    }

    hdr = x_hpool_slot_hdr(p, index);
    if ((hdr->flags & X_POOL_SLOT_ALIVE) == 0u)
    {
      return NULL;
    }

    return x_hpool_slot_item(p, hdr);
  }

  X_HPOOL_API XHandle x_hpool_alloc(XHPool* p)
  {
    uint32_t index;
    XHPoolSlotHeader* hdr;
    void* item;
    XHandle h;

    h = x_handle_null();

    if (p == NULL)
    {
      return h;
    }

    if (p->free_head != X_HPOOL_NULL_INDEX)
    {
      index = p->free_head;
      hdr = x_hpool_slot_hdr(p, index);
      p->free_head = hdr->next_free;
      hdr->next_free = X_HPOOL_NULL_INDEX;
    }
    else
    {
      index = p->next_index;
      p->next_index = p->next_index + 1u;

      if (x_hpool_ensure_page_for_index(p, index) == 0)
      {
        return h;
      }

      hdr = x_hpool_slot_hdr(p, index);
    }

    if (x_hpool_alive_reserve(p, p->alive_count + 1u) == 0)
    {
      /* push slot back to free list if we popped from free list */
      if ((hdr->flags & X_POOL_SLOT_ALIVE) == 0u)
      {
        hdr->next_free = p->free_head;
        p->free_head = index;
      }
      return h;
    }

    item = x_hpool_slot_item(p, hdr);
    if (p->ctor != NULL)
    {
      p->ctor(p->user, item);
    }
    else
    {
      memset(item, 0, p->item_size);
    }

    x_hpool_alive_add(p, index);

    h.index = index;
    h.version = hdr->version;

    return h;
  }

  X_HPOOL_API void x_hpool_free(XHPool* p, XHandle h)
  {
    XHPoolSlotHeader* hdr;
    void* item;

    if (p == NULL)
    {
      return;
    }

    if (x_handle_is_null(h) != 0)
    {
      return;
    }

    if (h.index >= x_hpool_capacity(p))
    {
      return;
    }

    hdr = x_hpool_slot_hdr(p, h.index);
    if (hdr->version != h.version)
    {
      return;
    }

    if ((hdr->flags & X_POOL_SLOT_ALIVE) == 0u)
    {
      return;
    }

    item = x_hpool_slot_item(p, hdr);
    if (p->dtor != NULL)
    {
      p->dtor(p->user, item);
    }

    x_hpool_alive_remove(p, h.index);

    hdr->version = hdr->version + 1u;
    if (hdr->version == 0u)
    {
      hdr->version = 1u;
    }

    hdr->next_free = p->free_head;
    p->free_head = h.index;
  }

  X_HPOOL_API void x_hpool_clear(XHPool* p)
  {
    uint32_t i;
    uint32_t count;

    if (p == NULL)
    {
      return;
    }

    /* Copy count because we mutate alive_count during frees */
    count = p->alive_count;

    /* Free from the end to minimize swaps, but correctness doesn't depend on it */
    for (i = 0u; i < count; i += 1u)
    {
      uint32_t idx;
      XHPoolSlotHeader* hdr;
      XHandle h;

      idx = p->alive[p->alive_count - 1u];
      hdr = x_hpool_slot_hdr(p, idx);

      h.index = idx;
      h.version = hdr->version;

      x_hpool_free(p, h);
    }
  }

  /* Iteration: returns pointer to item, optionally outputs handle */
  X_HPOOL_API void* x_hpool_iter_begin(XHPool* p, XHPoolIter* it, XHandle* out_h)
  {
    uint32_t index;
    XHPoolSlotHeader* hdr;
    void* item;

    if (it != NULL)
    {
      it->alive_pos = 0u;
    }

    if (p == NULL)
    {
      return NULL;
    }

    if (p->alive_count == 0u)
    {
      return NULL;
    }

    index = p->alive[0];
    hdr = x_hpool_slot_hdr(p, index);
    item = x_hpool_slot_item(p, hdr);

    if (out_h != NULL)
    {
      out_h->index = index;
      out_h->version = hdr->version;
    }

    return item;
  }

  X_HPOOL_API void* x_hpool_iter_next(XHPool* p, XHPoolIter* it, XHandle* out_h)
  {
    uint32_t pos;
    uint32_t index;
    XHPoolSlotHeader* hdr;
    void* item;

    if (p == NULL)
    {
      return NULL;
    }

    if (it == NULL)
    {
      return NULL;
    }

    pos = it->alive_pos + 1u;
    it->alive_pos = pos;

    if (pos >= p->alive_count)
    {
      return NULL;
    }

    index = p->alive[pos];
    hdr = x_hpool_slot_hdr(p, index);
    item = x_hpool_slot_item(p, hdr);

    if (out_h != NULL)
    {
      out_h->index = index;
      out_h->version = hdr->version;
    }

    return item;
  }


#ifdef __cplusplus
}
#endif

#endif  // X_IMPL_HPOOL
#endif  // STDX_HPOOL_H
