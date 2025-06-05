#define STDX_IMPLEMENTATION_TEST
#include <stdx_test.h>
#define STDX_IMPLEMENTATION_TIME
#include <stdx_time.h>

int test_timer_elapsed_positive()
{
  XTimer timer;
  x_timer_start(&timer);

  XTime start = x_timer_elapsed(&timer);
  x_time_sleep((XTime){.seconds = 0.1});
  XTime end = x_timer_elapsed(&timer);

  XTime diff = x_time_sub(end, start);
  ASSERT_TRUE(diff.seconds > 0.09); // at least 90ms
  return 0;
}

int test_time_conversions()
{
  XTime t = { .seconds = 1.5 };
  ASSERT_TRUE((int)x_time_milliseconds(t) == 1500);
  ASSERT_TRUE((int)x_time_microseconds(t) == 1500000);
  ASSERT_TRUE((int)x_time_nanoseconds(t)  == 1500000000);
  return 0;
}

int test_time_arithmetic()
{
  XTime a = { .seconds = 2.0 };
  XTime b = { .seconds = 0.5 };
  XTime sum = x_time_add(a, b);
  XTime diff = x_time_sub(a, b);
  ASSERT_TRUE(sum.seconds == 2.5);
  ASSERT_TRUE(diff.seconds == 1.5);
  return 0;
}

int test_time_comparisons()
{
  XTime a = { .seconds = 1.0 };
  XTime b = { .seconds = 2.0 };
  ASSERT_TRUE(x_time_less_than(a, b));
  ASSERT_TRUE(x_time_greater_than(b, a));
  ASSERT_TRUE(!x_time_equals(a, b));
  ASSERT_TRUE(x_time_equals(a, a));
  return 0;
}

int test_time_sleep()
{
  XTimer timer;
  x_timer_start(&timer);
  x_time_sleep((XTime){ .seconds = 0.2 });

  XTime elapsed = x_timer_elapsed(&timer);
  ASSERT_TRUE(elapsed.seconds >= 0.18); // Allow slight jitter
  return 0;
}

int test_time_now()
{
  XTime now = x_time_now();
  ASSERT_TRUE(now.seconds > 1600000000); // Should be well past year 2020
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    STDX_TEST(test_timer_elapsed_positive),
    STDX_TEST(test_time_conversions),
    STDX_TEST(test_time_arithmetic),
    STDX_TEST(test_time_comparisons),
    STDX_TEST(test_time_sleep),
    STDX_TEST(test_time_now)
  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}
