/**
 * STDX - Multithreading Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * <https://github.com/marciovmf/stdx>
 * License: MIT
 *
 * ## Overview
 * Designed to abstract platform-specific APIs (e.g., pthreads, Win32)
 * behind a consistent and lightweight int32_terface.
 * This header provides functions for:
 * - Thread creation and joining
 * - Mutexes and condition variables
 * - Sleep/yield utilities
 * - A thread pool for concurrent task execution
 *
 * ## How to compile
 *
 * To compile the implementation define `X_IMPL_THREAD`
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * `X_THREAD_ALLOC` / `X_THREAD_FREE` before including.
 *
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

  typedef void (*XThreadTask)(void* arg);
  typedef void* (*XThreadFunc)(void*);

  typedef struct XThread XThread;
  typedef struct XMutex XMutex;
  typedef struct XCondVar XCondVar;
  typedef struct XThreadPool XThreadPool;
  typedef struct XTask XTask;

/**
 * @brief Create and start a new thread.
 * @param t Output pointer that receives the created thread handle.
 * @param func Thread entry function.
 * @param arg User argument passed to func.
 * @return 0 on success, non-zero on failure.
 */
int32_t x_thread_create(XThread** t, XThreadFunc func, void* arg);

/**
 * @brief Wait for a thread to finish execution.
 * @param t Thread handle.
 */
void x_thread_join(XThread* t);

/**
 * @brief Destroy a thread handle and release its resources.
 * @param t Thread handle.
 */
void x_thread_destroy(XThread* t);

/**
 * @brief Create a mutex.
 * @param m Output pointer that receives the created mutex handle.
 * @return 0 on success, non-zero on failure.
 */
int32_t x_thread_mutex_init(XMutex** m);

/**
 * @brief Lock a mutex, blocking until it becomes available.
 * @param m Mutex handle.
 */
void x_thread_mutex_lock(XMutex* m);

/**
 * @brief Unlock a mutex.
 * @param m Mutex handle.
 */
void x_thread_mutex_unlock(XMutex* m);

/**
 * @brief Destroy a mutex and release its resources.
 * @param m Mutex handle.
 */
void x_thread_mutex_destroy(XMutex* m);

/**
 * @brief Create a condition variable.
 * @param cv Output pointer that receives the created condition variable handle.
 * @return 0 on success, non-zero on failure.
 */
int32_t x_thread_condvar_init(XCondVar** cv);

/**
 * @brief Wait on a condition variable.
 * @param cv Condition variable handle.
 * @param m Mutex that will be atomically released while waiting and re-acquired before returning.
 */
void x_thread_condvar_wait(XCondVar* cv, XMutex* m);

/**
 * @brief Wake one thread waiting on a condition variable.
 * @param cv Condition variable handle.
 */
void x_thread_condvar_signal(XCondVar* cv);

/**
 * @brief Wake all threads waiting on a condition variable.
 * @param cv Condition variable handle.
 */
void x_thread_condvar_broadcast(XCondVar* cv);

/**
 * @brief Destroy a condition variable and release its resources.
 * @param cv Condition variable handle.
 */
void x_thread_condvar_destroy(XCondVar* cv);

/**
 * @brief Sleep the current thread for at least the given number of milliseconds.
 * @param ms Duration in milliseconds.
 */
void x_thread_sleep_ms(int ms);

/**
 * @brief Yield execution to allow other threads to run.
 */
void x_thread_yield(void);

/**
 * @brief Create a thread pool with a fixed number of worker threads.
 * @param num_threads Number of worker threads to start.
 * @return Thread pool handle, or NULL on failure.
 */
XThreadPool* x_threadpool_create(int num_threads);

/**
 * @brief Enqueue a task for execution by the thread pool.
 * @param pool Thread pool handle.
 * @param fn Task function to execute.
 * @param arg User argument passed to fn.
 * @return 0 on success, non-zero on failure.
 */
int32_t x_threadpool_enqueue(XThreadPool* pool, XThreadTask fn, void* arg);

/**
 * @brief Destroy a thread pool and release its resources.
 * @param pool Thread pool handle.
 */
void x_threadpool_destroy(XThreadPool* pool);

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

#include <stdlib.h>

#ifndef X_THREAD_ALLOC
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_THREAD_ALLOC(sz)        malloc(sz)
#endif

#ifndef X_THREAD_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_THREAD_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

  struct XThread { HANDLE handle; };
  struct XMutex  { CRITICAL_SECTION cs; };
  struct XCondVar { CONDITION_VARIABLE cv; };

  struct XThreadWrapper
  {
    XThreadFunc func;
    void* arg;
  };

  static DWORD WINAPI x_thread_proc(LPVOID param)
  {
    struct XThreadWrapper* wrap = (struct XThreadWrapper*)param;
    void* result = wrap->func(wrap->arg);
    X_THREAD_FREE(wrap);
    return (DWORD)(uintptr_t)result;
  }

  int32_t x_thread_create(XThread** t, XThreadFunc func, void* arg)
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
  struct XMutex  { px_thread_XMutex m; };
  struct XCondVar { px_thread_cond_t cv; };

  int32_t x_thread_create(XThread** t, XThreadFunc func, void* arg)
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

  struct XTask
  {
    XThreadTask fn;
    void* arg;
    XTask* next;
  };

  struct XThreadPool
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

  int32_t x_threadpool_enqueue(XThreadPool* pool, XThreadTask fn, void* arg)
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
