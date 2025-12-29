/**
 * STDX - Minimal Unit Test framework
 * Part of the STDX General Purpose C Library by marciovmf
 * <https://github.com/marciovmf/stdx>
 * License: MIT
 *
 * ## Overview
 *
 * - Lightweight, self-contained C test runner
 * - Colored PASS/FAIL output using x_log
 * - Assertion macros for booleans, equality, floats
 * - Signal handling for crash diagnostics (SIGSEGV, SIGABRT, etc.)
 *
 * ## How to compile
 *
 * To compile the implementation define `X_IMPL_TEST`
 * in **one** source file before including this header.
 *
 * ## Usage
 *
 * - Define your test functions to return int32_t (0 for pass, 1 for fail)
 * - Use `ASSERT_*` macros for checks
 * - Register with `X_TEST(name)` and run with `x_tests_run(...)`
 * - Define `X_IMPL_TEST` before including to enable main runner.
 *
 * ### Example
 *
 * ```` 
 *   int32_t test_example() {
 *     ASSERT_TRUE(2 + 2 == 4);
 *     return 0;
 *   }
 *
 *   int32_t main() {
 *     STDXTestCase tests[] = {
 *       X_TEST(test_example)
 *     };
 *     return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]));
 *   }
 * ```` 
 *
 * ## Dependencies
 *
 * x_log.h
 */
#ifndef X_TEST_H
#define X_TEST_H

#define X_TEST_VERSION_MAJOR 1
#define X_TEST_VERSION_MINOR 0
#define X_TEST_VERSION_PATCH 0

#define X_TEST_VERSION (X_TEST_VERSION_MAJOR * 10000 + X_TEST_VERSION_MINOR * 100 + X_TEST_VERSION_PATCH)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef X_IMPL_TEST
#ifndef X_IMPL_LOG
#define X_INTERNAL_LOGGER_IMPL
#define X_IMPL_LOG
#endif
#endif
#include <stdx_log.h>

#ifdef X_IMPL_TEST
#ifndef X_IMPL_TIME
#define X_INTERNAL_TIME_IMPL
#define X_IMPL_TIME
#endif
#endif
#include <stdx_time.h>


#include <stdint.h>
#include <math.h>

#ifndef X_TEST_SUCCESS  
#define X_TEST_SUCCESS 0
#endif

#ifndef X_TEST_FAIL  
#define X_TEST_FAIL -1 
#endif

#define XLOG_GREEN(msg, ...)  x_log_raw(stdout, XLOG_LEVEL_INFO, XLOG_COLOR_GREEN, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define XLOG_WHITE(msg, ...)  x_log_raw(stdout, XLOG_LEVEL_INFO, XLOG_COLOR_WHITE, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define XLOG_RED(msg, ...)    x_log_raw(stderr, XLOG_LEVEL_INFO, XLOG_COLOR_RED, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)

  /**
   * @brief Assert that an expression evaluates to true; logs an error and returns 1 from the test on failure.
   * @param expr Boolean expression to evaluate.
   * @return Nothing (on failure, returns 1 from the calling function).
   */
#define ASSERT_TRUE(expr) do { \
  if (!(expr)) { \
    x_log_error("\t%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, (#expr)); \
    return 1; \
  } \
} while (0)

/**
 * @brief Assert that an expression evaluates to false; logs an error and returns 1 from the test on failure.
 * @param expr Boolean expression to evaluate.
 * @return Nothing (on failure, returns 1 from the calling function).
 */
#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

/**
 * @brief Assert that two values are equal; logs an error and returns 1 from the test on failure.
 * @param actual Value produced by the code under test.
 * @param expected Value the test expects.
 * @return Nothing (on failure, returns 1 from the calling function).
 */
#define ASSERT_EQ(actual, expected) do { \
  if ((actual) != (expected)) { \
    x_log_error("\t%s:%d: Assertion failed: %s == %s", __FILE__, __LINE__, #actual, #expected); \
    return 1; \
  } \
} while (0)

/**
 * @brief Default epsilon used by ASSERT_FLOAT_EQ for approximate float comparisons.
 * @param None.
 * @return Nothing.
 */
#define X_TEST_FLOAT_EPSILON 0.1f

/**
 * @brief Assert that two floating-point values are approximately equal within X_TEST_FLOAT_EPSILON.
 * @param actual Floating-point value produced by the code under test.
 * @param expected Floating-point value the test expects.
 * @return Nothing (on failure, returns 1 from the calling function).
 */
#define ASSERT_FLOAT_EQ(actual, expected) do { \
  if (fabs((actual) - (expected)) > X_TEST_FLOAT_EPSILON) { \
    x_log_error("\t%s:%d: Assertion failed: %s == %s", __FILE__, __LINE__, #actual, #expected); \
    return 1; \
  } \
} while (0)

/**
 * @brief Assert that two values are not equal; logs an error and returns 1 from the test on failure.
 * @param actual Value produced by the code under test.
 * @param expected Value the test expects it to differ from.
 * @return Nothing (on failure, returns 1 from the calling function).
 */
#define ASSERT_NEQ(actual, expected) do { \
  if ((actual) == (expected)) { \
    x_log_error("\t%s:%d: Assertion failed: %s != %s", __FILE__, __LINE__, #actual, #expected); \
    return 1; \
  } \
} while (0)

/**
 * @brief Test function signature used by the STDX test runner.
 * @param None.
 * @return 0 on success, 1 (or non-zero) on failure.
 */
typedef int32_t (*STDXTestFunction)();

typedef struct
{
  const char *name;
  STDXTestFunction func;
} STDXTestCase;

#define X_TEST(name) {#name, name}

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_TEST

#include <stdio.h>
#include <signal.h>


#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

  static void s_tests_on_signal(int signal)
  {
    const char* signalName = "Unknown signal";
    switch(signal)
    {
      case SIGABRT: signalName = (const char*) "SIGABRT"; break;
      case SIGFPE:  signalName = (const char*) "SIGFPE"; break;
      case SIGILL:  signalName = (const char*) "SIGILL"; break;
      case SIGINT:  signalName = (const char*) "SIGINT"; break;
      case SIGSEGV: signalName = (const char*) "SIGSEGV"; break;
      case SIGTERM: signalName = (const char*) "SIGTERM"; break;
    }

    fflush(stderr);
    fflush(stdout);
    x_log_error("\n[!!!!]  Test Crashed! %s", signalName);
  }

  int x_tests_run(STDXTestCase* tests, int32_t num_tests)
  { 
    signal(SIGABRT, s_tests_on_signal);
    signal(SIGFPE,  s_tests_on_signal);
    signal(SIGILL,  s_tests_on_signal);
    signal(SIGINT,  s_tests_on_signal);
    signal(SIGSEGV, s_tests_on_signal);
    signal(SIGTERM, s_tests_on_signal);

    int32_t passed = 0; 
    double total_time = 0;
    for (int32_t i = 0; i < num_tests; ++i)
    {
      fflush(stdout);

      XTimer timer;
      x_timer_start(&timer);
      int32_t result = tests[i].func();
      XTime elapsed = x_timer_elapsed(&timer);
      const double milliseconds = x_time_milliseconds(elapsed);
      total_time += milliseconds;

      if (result == 0)
      {
        XLOG_WHITE(" [", 0);
        XLOG_GREEN("PASS", 0);
        XLOG_WHITE("]  %d/%d\t %f ms -> %s\n", i+1, num_tests, milliseconds, tests[i].name);
        passed++;
      }
      else
      {
        XLOG_WHITE(" [", 0);
        XLOG_RED("FAIL", 0);
        XLOG_WHITE("]  %d/%d\t %f ms -> %s\n", i+1, num_tests, milliseconds, tests[i].name);
      }
    }

    if (passed == num_tests)
      XLOG_GREEN(" Tests passed: %d / %d  - total time %f ms\n", passed, num_tests, total_time);
    else
      XLOG_RED(" Tests failed: %d / %d - total time %f ms\n", num_tests - passed, num_tests, total_time);

    return passed != num_tests;
  }

#ifdef __cplusplus
}
#endif

#endif  // X_IMPL_TEST

#ifdef X_INTERNAL_TIME_IMPL
#undef X_IMPL_TIME
#undef X_INTERNAL_TIME_IMPL
#endif

#ifdef X_INTERNAL_LOGGER_IMPL
#undef X_IMPL_LOG
#undef X_INTERNAL_LOG_IMPL
#endif
#endif // X_TEST_H
