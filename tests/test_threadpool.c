#include <stdx_common.h>
#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_THREAD
#include <stdx_thread.h>
#include <stdx_log.h>

#define NUM_TASKS 100

#define SUPPRESS_OUTPUT 1

XMutex* count_lock;
XCondVar* count_cv;
int completed_tasks = 0;

void print_task(void* arg)
{
  X_UNUSED(arg);
#if !SUPPRESS_OUTPUT
  log_info("Task %d running in thread\n", n);
#endif
  //thread_sleep_ms(5);

  x_thread_mutex_lock(count_lock);
  completed_tasks++;
  if (completed_tasks == NUM_TASKS)
    x_thread_condvar_signal(count_cv);
  x_thread_mutex_unlock(count_lock);
}

int test_threadpool_execution(void)
{
  completed_tasks = 0;

  XThreadPool* pool = threadpool_create(4);
  ASSERT_TRUE(pool != NULL);

  int args[NUM_TASKS];
  for (int i = 0; i < NUM_TASKS; ++i)
  {
    args[i] = i;
    ASSERT_TRUE(threadpool_enqueue(pool, print_task, &args[i]) == 0); // Assuming 0 means success
  }

  // Wait until all tasks complete
  x_thread_mutex_lock(count_lock);
  while (completed_tasks < NUM_TASKS)
    x_thread_condvar_wait(count_cv, count_lock);
  x_thread_mutex_unlock(count_lock);

  ASSERT_TRUE(completed_tasks == NUM_TASKS);

  threadpool_destroy(pool);
  return 0;
}

int test_enqueue_after_destroy(void)
{
  XThreadPool* pool = threadpool_create(2);
  ASSERT_TRUE(pool != 0);
  threadpool_destroy(pool);

  int dummy = 42;
  int result = threadpool_enqueue(pool, print_task, &dummy);
  ASSERT_TRUE(result != 0);
  return 0;
}

int main()
{
  x_thread_mutex_init(&count_lock);
  x_thread_condvar_init(&count_cv);

  STDXTestCase tests[] =
  {
    X_TEST(test_threadpool_execution),
    X_TEST(test_enqueue_after_destroy),
  };

  int result = x_tests_run(tests, sizeof(tests) / sizeof(tests[0]));

  x_thread_mutex_destroy(count_lock);
  x_thread_condvar_destroy(count_cv);

  return result;
}
