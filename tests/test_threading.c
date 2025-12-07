#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_THREAD
#include <stdx_thread.h>

// Threading globals
XMutex* lock_a;
XCondVar* not_empty;
int buffer = 0;
bool has_data = false;
#define THREAD_PRODUCED_VALUE 42

// Utility functions
static void* producer(void* _)
{
  x_thread_mutex_lock(lock_a);
  buffer = THREAD_PRODUCED_VALUE;
  has_data = true;
  x_thread_condvar_signal(not_empty);
  x_thread_mutex_unlock(lock_a);
  return NULL;
}

static void* consumer(void* _)
{
  x_thread_mutex_lock(lock_a);
  while (!has_data)
  {
    x_thread_condvar_wait(not_empty, lock_a);
  }

  has_data = false;
  x_thread_mutex_unlock(lock_a);
  return &buffer;
}

// Test functions
int test_producer_consumer(void)
{
  x_thread_mutex_init(&lock_a);
  x_thread_condvar_init(&not_empty);

  XThread *p, *c;
  x_thread_create(&p, producer, NULL);
  x_thread_create(&c, consumer, NULL);

  x_thread_join(p);
  x_thread_join(c);

  ASSERT_TRUE(buffer == THREAD_PRODUCED_VALUE);

  x_thread_destroy(p);
  x_thread_destroy(c);
  x_thread_condvar_destroy(not_empty);
  x_thread_mutex_destroy(lock_a);
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_producer_consumer),
  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]));
}
