/*
 * STDX - Multithreading Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * To compile the implementation define X_IMPL_THREAD
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * X_THREAD_ALLOC / X_THREAD_FREE before including.
 *
 * Notes:
 *  Designed to abstract platform-specific APIs (e.g., pthreads, Win32)
 *  behind a consistent and lightweight int32_terface.
 *  This header provides functions for:
 *   - Thread creation and joining
 *   - Mutexes and condition variables
 *   - Sleep/yield utilities
 *   - A thread pool for concurrent task execution
 */

#ifndef X_THREAD_H
#define X_THREAD_H

#include <stdint.h>

#define X_THREADING_VERSION_MAJOR 1
#define X_THREADING_VERSION_MINOR 0
#define X_THREADING_VERSION_PATCH 0
#define X_THREADING_VERSION (X_THREADING_VERSION_MAJOR * 10000 + X_THREADING_VERSION_MINOR * 100 + X_THREADING_VERSION_PATCH)

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XThread_t XThread;
  typedef struct XMutex_t XMutex;
  typedef struct XCondVar_t XCondVar;
  typedef struct XThreadPool_t XThreadPool;
  typedef struct XTask_t XTask;
  typedef void (*XThreadTask_fn)(void* arg);
  typedef void* (*x_thread_func_t)(void*);
  // Basic thread operations
  int32_t x_thread_create(XThread** t, x_thread_func_t func, void* arg);
  void    x_thread_join(XThread* t);
  void    x_thread_destroy(XThread* t);
  // Thread Synchronization
  int32_t x_thread_mutex_init(XMutex** m);
  void    x_thread_mutex_lock(XMutex* m);
  void    x_thread_mutex_unlock(XMutex* m);
  void    x_thread_mutex_destroy(XMutex* m);
  int32_t x_thread_condvar_init(XCondVar** cv);
  void    x_thread_condvar_wait(XCondVar* cv, XMutex* m);
  void    x_thread_condvar_signal(XCondVar* cv);
  void    x_thread_condvar_broadcast(XCondVar* cv);
  void    x_thread_condvar_destroy(XCondVar* cv);
  void    x_thread_sleep_ms(int ms);
  void    x_thread_yield();
  // Thread pool
  XThreadPool*  x_threadpool_create(int num_threads);
  int32_t       x_threadpool_enqueue(XThreadPool* pool, XThreadTask_fn fn, void* arg);
  void          x_threadpool_destroy(XThreadPool* pool);

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_THREAD

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#endif // _WIN32

#ifndef X_THREAD_ALLOC
#define X_THREAD_ALLOC(sz)        malloc(sz)
#define X_THREAD_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

  struct XThread_t { HANDLE handle; };
  struct XMutex_t  { CRITICAL_SECTION cs; };
  struct XCondVar_t { CONDITION_VARIABLE cv; };

  struct XThreadWrapper
  {
    x_thread_func_t func;
    void* arg;
  };

  static DWORD WINAPI x_thread_proc(LPVOID param)
  {
    struct XThreadWrapper* wrap = (struct XThreadWrapper*)param;
    void* result = wrap->func(wrap->arg);
    X_THREAD_FREE(wrap);
    return (DWORD)(uintptr_t)result;
  }

  int32_t x_thread_create(XThread** t, x_thread_func_t func, void* arg)
  {
    if (!t || !func) return -1;
    *t = X_THREAD_ALLOC(sizeof(XThread));
    struct XThreadWrapper* wrap = X_THREAD_ALLOC(sizeof(struct XThreadWrapper));
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
    if (t) X_THREAD_FREE(t);
  }

  int32_t x_thread_mutex_init(XMutex** m)
  {
    *m = X_THREAD_ALLOC(sizeof(XMutex));
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
    X_THREAD_FREE(m);
  }

  int32_t x_thread_condvar_init(XCondVar** cv)
  {
    *cv = X_THREAD_ALLOC(sizeof(XCondVar));
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
    X_THREAD_FREE(cv);
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

  struct XThread { pXThread id; };
  struct mutex  { px_thread_XMutex m; };
  struct condvar { px_thread_cond_t cv; };

  int32_t x_thread_create(XThread** t, x_thread_func_t func, void* arg)
  {
    if (!t || !func) return -1;
    *t = X_THREAD_ALLOC(sizeof(XThread));
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
    if (t) X_THREAD_FREE(t);
  }

  int32_t x_thread_mutexinit(XMutex** m) {
    *m = X_THREAD_ALLOC(sizeof(XMutex));
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
    X_THREAD_FREE(m);
  }

  int32_t x_thread_condvar_init(XCondVar** cv)
  {
    *cv = X_THREAD_ALLOC(sizeof(XCondVar));
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
    X_THREAD_FREE(cv);
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

#endif // WIN_32

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
        X_THREAD_FREE(task);
      }
    }
    return NULL;
  }

  XThreadPool* x_threadpool_create(int num_threads)
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

  int32_t x_threadpool_enqueue(XThreadPool* pool, XThreadTask_fn fn, void* arg)
  {
    if (!fn || !pool || pool->magic != THREADPOOL_MAGIC) return -1;

    XTask* task = X_THREAD_ALLOC(sizeof(XTask));
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

  void x_threadpool_destroy(XThreadPool* pool)
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

    X_THREAD_FREE(pool->threads);
    x_thread_mutex_destroy(pool->lock);
    x_thread_condvar_destroy(pool->cv);

    // Clean up remaining tasks
    while (pool->head)
    {
      XTask* tmp = pool->head;
      pool->head = tmp->next;
      X_THREAD_FREE(tmp);
    }

    X_THREAD_FREE(pool);
  }

#ifdef __cplusplus
}
#endif

#endif  // X_IMPL_THREAD
#endif  // X_THREAD_H
