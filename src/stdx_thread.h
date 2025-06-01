/*
 * STDX - Multithreading Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a portable threading abstraction for C programs. Includes:
 *   - Thread creation and joining
 *   - Mutexes and condition variables
 *   - Sleep/yield utilities
 *   - A thread pool for concurrent task execution
 *
 * Designed to abstract platform-specific APIs (e.g., pthreads, Win32)
 * behind a consistent and lightweight int32_terface.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_THREAD
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Dependencies: stdx_alloc.h (optional for thread pool)
 * Usage: #include "stdx_thread.h"
 */

#ifndef STDX_THREAD_H
#define STDX_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#define STDX_THREADING_VERSION_MAJOR 1
#define STDX_THREADING_VERSION_MINOR 0
#define STDX_THREADING_VERSION_PATCH 0

#define STDX_THREADING_VERSION (STDX_THREADING_VERSION_MAJOR * 10000 + STDX_THREADING_VERSION_MINOR * 100 + STDX_THREADING_VERSION_PATCH)

#include <stdint.h>

  typedef struct XXThread XThread;
  typedef struct XXMutex XMutex;
  typedef struct XXCondVar XCondVar;
  typedef struct XThreadPool_t XThreadPool;
  typedef struct XTask_t XTask;
  typedef void (*XThreadTask_fn)(void* arg);
  typedef void* (*x_thread_func_t)(void*);

  // ---------------------------------------------------------------------------
  // Basic thread operations
  // ---------------------------------------------------------------------------

  int32_t   x_thread_create(XThread** t, x_thread_func_t func, void* arg);
  void  x_thread_join(XThread* t);
  void  x_thread_destroy(XThread* t);

  // ---------------------------------------------------------------------------
  // Thread Synchronization
  // ---------------------------------------------------------------------------

  int32_t   x_thread_mutex_init(XMutex** m);
  void  x_thread_mutex_lock(XMutex* m);
  void  x_thread_mutex_unlock(XMutex* m);
  void  x_thread_mutex_destroy(XMutex* m);
  int32_t   x_thread_condvar_init(XCondVar** cv);
  void  x_thread_condvar_wait(XCondVar* cv, XMutex* m);
  void  x_thread_condvar_signal(XCondVar* cv);
  void  x_thread_condvar_broadcast(XCondVar* cv);
  void  x_thread_condvar_destroy(XCondVar* cv);
  void  x_thread_sleep_ms(int ms);
  void  x_thread_yield();


  // ---------------------------------------------------------------------------
  // Thread pool
  // ---------------------------------------------------------------------------

  XThreadPool* threadpool_create(int num_threads);
  int32_t threadpool_enqueue(XThreadPool* pool, XThreadTask_fn fn, void* arg);
  void threadpool_destroy(XThreadPool* pool);


#ifdef STDX_IMPLEMENTATION_THREAD

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32

#include <windows.h>

  struct XXThread { HANDLE handle; };
  struct XXMutex  { CRITICAL_SECTION cs; };
  struct XXCondVar { CONDITION_VARIABLE cv; };

  struct XThreadWrapper
  {
    x_thread_func_t func;
    void* arg;
  };

  static DWORD WINAPI x_thread_proc(LPVOID param)
  {
    struct XThreadWrapper* wrap = (struct XThreadWrapper*)param;
    void* result = wrap->func(wrap->arg);
    free(wrap);
    return (DWORD)(uintptr_t)result;
  }

  int32_t x_thread_create(XThread** t, x_thread_func_t func, void* arg)
  {
    if (!t || !func) return -1;
    *t = malloc(sizeof(XThread));
    struct XThreadWrapper* wrap = malloc(sizeof(struct XThreadWrapper));
    wrap->func = func;
    wrap->arg = arg;
    (*t)->handle = CreateThread(NULL, 0, x_thread_proc, wrap, 0, NULL);
    return (*t)->handle ? 0 : -1;
  }

  void x_thread_join(XThread* t)
  {
    if (t && t->handle) {
      WaitForSingleObject(t->handle, INFINITE);
      CloseHandle(t->handle);
    }
  }

  void x_thread_destroy(XThread* t)
  {
    if (t) free(t);
  }

  int32_t x_thread_mutex_init(XMutex** m)
  {
    *m = malloc(sizeof(XMutex));
    InitializeCriticalSection(&(*m)->cs);
    return 0;
  }

  void x_thread_mutex_lock(XMutex* m)
  {
    EnterCriticalSection(&m->cs);
  }

  void x_thread_mutex_unlock(XMutex* m)
  {
    LeaveCriticalSection(&m->cs);
  }

  void x_thread_mutex_destroy(XMutex* m)
  {
    DeleteCriticalSection(&m->cs);
    free(m);
  }

  int32_t x_thread_condvar_init(XCondVar** cv)
  {
    *cv = malloc(sizeof(XCondVar));
    InitializeConditionVariable(&(*cv)->cv);
    return 0;
  }

  void x_thread_condvar_wait(XCondVar* cv, XMutex* m)
  {
    SleepConditionVariableCS(&cv->cv, &m->cs, INFINITE);
  }

  void x_thread_condvar_signal(XCondVar* cv)
  {
    WakeConditionVariable(&cv->cv);
  }

  void x_thread_condvar_broadcast(XCondVar* cv)
  {
    WakeAllConditionVariable(&cv->cv);
  }

  void x_thread_condvar_destroy(XCondVar* cv)
  {
    free(cv);
  }

  void x_thread_sleep_ms(int ms)
  {
    Sleep(ms);
  }

  void x_thread_yield()
  {
    Sleep(0);
  }

#else // POSIX

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

  struct XThread { pXThread id; };
  struct mutex  { px_thread_XMutex m; };
  struct condvar { px_thread_cond_t cv; };

  int32_t x_thread_create(XThread** t, x_thread_func_t func, void* arg)
  {
    if (!t || !func) return -1;
    *t = malloc(sizeof(XThread));
    return px_thread_create(&(*t)->id, NULL, func, arg);
  }

  void x_thread_join(XThread* t)
  {
    if (t) {
      px_thread_join(t->id, NULL);
    }
  }

  void x_thread_destroy(XThread* t)
  {
    if (t) free(t);
  }

  int32_t x_thread_mutexinit(XMutex** m) {
    *m = malloc(sizeof(XMutex));
    px_thread_x_thread_mutexinit(&(*m)->m, NULL);
    return 0;
  }

  void x_thread_mutex_lock(XMutex* m)
  {
    px_thread_x_thread_mutexlock(&m->m);
  }

  void x_thread_mutex_unlock(XMutex* m)
  {
    px_thread_x_thread_mutexunlock(&m->m);
  }

  void x_thread_mutex_destroy(XMutex* m)
  {
    px_thread_x_thread_mutexdestroy(&m->m);
    free(m);
  }

  int32_t x_thread_condvar_init(XCondVar** cv)
  {
    *cv = malloc(sizeof(XCondVar));
    px_thread_cond_init(&(*cv)->cv, NULL);
    return 0;
  }

  void x_thread_condvar_wait(XCondVar* cv, XMutex* m)
  {
    px_thread_cond_wait(&cv->cv, &m->m);
  }

  void x_thread_condvar_signal(XCondVar* cv)
  {
    px_thread_cond_signal(&cv->cv);
  }

  void x_thread_condvar_broadcast(XCondVar* cv)
  {
    px_thread_cond_broadcast(&cv->cv);
  }

  void x_thread_condvar_destroy(XCondVar* cv)
  {
    px_thread_cond_destroy(&cv->cv);
    free(cv);
  }

  void x_thread_sleep_ms(int ms)
  {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
  }

  void x_thread_yield()
  {
    sched_yield();
  }

#endif




#define THREADPOOL_MAGIC 0xDEADBEEF

  struct XTask_t
  {
    XThreadTask_fn fn;
    void* arg;
    XTask* next;
  };

  struct XThreadPool_t
  {
    uint32_t magic;
    XThread** threads;
    int32_t num_threads;

    XTask* head;
    XTask* tail;

    XMutex* lock;
    XCondVar* cv;

    bool stop;
  };

  static void* thread_main(void* arg)
  {
    XThreadPool* pool = (XThreadPool*)arg;

    while (1)
    {
      x_thread_mutex_lock(pool->lock);
      while (!pool->head && !pool->stop)
      {
        x_thread_condvar_wait(pool->cv, pool->lock);
      }

      if (pool->stop && !pool->head)
      {
        x_thread_mutex_unlock(pool->lock);
        break;
      }

      XTask* task = pool->head;
      if (task)
      {
        pool->head = task->next;
        if (!pool->head) pool->tail = NULL;
      }
      x_thread_mutex_unlock(pool->lock);

      if (task)
      {
        task->fn(task->arg);
        free(task);
      }
    }
    return NULL;
  }

  XThreadPool* threadpool_create(int num_threads)
  {
    if (num_threads <= 0)
      return NULL;

    XThreadPool* pool = calloc(1, sizeof(XThreadPool));
    pool->num_threads = num_threads;
    pool->threads = calloc(num_threads, sizeof(XThread*));
    x_thread_mutex_init(&pool->lock);
    x_thread_condvar_init(&pool->cv);

    for (int i = 0; i < num_threads; ++i)
    {
      x_thread_create(&pool->threads[i], thread_main, pool);
    }
    pool->magic = THREADPOOL_MAGIC;
    return pool;
  }

  int32_t threadpool_enqueue(XThreadPool* pool, XThreadTask_fn fn, void* arg)
  {
    if (!fn || !pool || pool->magic != THREADPOOL_MAGIC) return -1;

    XTask* task = malloc(sizeof(XTask));
    task->fn = fn;
    task->arg = arg;
    task->next = NULL;

    x_thread_mutex_lock(pool->lock);
    if (pool->tail)
    {
      pool->tail->next = task;
      pool->tail = task;
    } else
    {
      pool->head = pool->tail = task;
    }
    x_thread_condvar_signal(pool->cv);
    x_thread_mutex_unlock(pool->lock);

    return 0;
  }

  void threadpool_destroy(XThreadPool* pool)
  {
    if (!pool) return;

    x_thread_mutex_lock(pool->lock);
    pool->stop = true;
    pool->magic = 0;
    x_thread_condvar_broadcast(pool->cv);
    x_thread_mutex_unlock(pool->lock);

    for (int i = 0; i < pool->num_threads; ++i)
    {
      x_thread_join(pool->threads[i]);
      x_thread_destroy(pool->threads[i]);
    }

    free(pool->threads);
    x_thread_mutex_destroy(pool->lock);
    x_thread_condvar_destroy(pool->cv);

    // Clean up remaining tasks
    while (pool->head)
    {
      XTask* tmp = pool->head;
      pool->head = tmp->next;
      free(tmp);
    }

    free(pool);
  }


#endif  // STDX_IMPLEMENTATION_THREAD

#ifdef __cplusplus
}
#endif

#endif // STDX_THREAD_H
