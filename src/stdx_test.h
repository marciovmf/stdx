/*
 * STDX - Minimal Unit Test framework
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Features:
 *   - Lightweight, self-contained C test runner
 *   - Colored PASS/FAIL output using x_log
 *   - Assertion macros for booleans, equality, floats
 *   - Signal handling for crash diagnostics (SIGSEGV, SIGABRT, etc.)
 *
 * Usage:
 *   - Define your test functions to return int32_t (0 for pass, 1 for fail)
 *   - Use `ASSERT_*` macros for checks
 *   - Register with STDX_TEST(name) and run with `stdx_run_tests(...)`
 *   - Define STDX_IMPLEMENTATION_TEST before including to enable main runner
 *
 * Example:
 *   int32_t test_example() {
 *     ASSERT_TRUE(2 + 2 == 4);
 *     return 0;
 *   }
 *   int32_t main() {
 *     STDXTestCase tests[] = {
 *       STDX_TEST(test_example)
 *     };
 *     return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
 *   }
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_TEST
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Dependencies: stdx_log.h
 * Usage: #include "stdx_test.h"
 */
#ifndef STDX_TEST_H
#define STDX_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#define STDX_TEST_VERSION_MAJOR 1
#define STDX_TEST_VERSION_MINOR 0
#define STDX_TEST_VERSION_PATCH 0

#define STDX_TEST_VERSION (STDX_TEST_VERSION_MAJOR * 10000 + STDX_TEST_VERSION_MINOR * 100 + STDX_TEST_VERSION_PATCH)

#ifdef STDX_IMPLEMENTATION_TEST
  #ifndef STDX_IMPLEMENTATION_LOG
    #define STDX_INTERNAL_LOGGER_IMPLEMENTATION
    #define STDX_IMPLEMENTATION_LOG
  #endif
#endif
#include <stdx_log.h>

#ifdef STDX_IMPLEMENTATION_TEST
  #ifndef STDX_IMPLEMENTATION_TIME
    #define STDX_INTERNAL_TIME_IMPLEMENTATION
    #define STDX_IMPLEMENTATION_TIME
  #endif
#endif
#include <stdx_time.h>


#include <stdint.h>
#include <math.h>

#ifndef STDX_TEST_SUCCESS  
#define STDX_TEST_SUCCESS 0
#endif

#ifndef STDX_TEST_FAIL  
#define STDX_TEST_FAIL -1 
#endif

#define XLOG_GREEN(msg, ...)  x_log_raw(XLOG_LEVEL_INFO, XLOG_COLOR_GREEN, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define XLOG_WHITE(msg, ...)  x_log_raw(XLOG_LEVEL_INFO, XLOG_COLOR_WHITE, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define XLOG_RED(msg, ...)    x_log_raw(XLOG_LEVEL_INFO, XLOG_COLOR_RED, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)

// Original macros without message and format string support
#define ASSERT_TRUE(expr) do { \
  if (!(expr)) { \
    x_log_error("\t%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, (#expr)); \
    return 1; \
  } \
} while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(actual, expected) do { \
  if ((actual) != (expected)) { \
    x_log_error("\t%s:%d: Assertion failed: %s == %s", __FILE__, __LINE__, #actual, #expected); \
    return 1; \
  } \
} while (0)

#define STDX_TEST_FLOAT_EPSILON 0.1f
#define ASSERT_FLOAT_EQ(actual, expected) do { \
  if (fabs((actual) - (expected)) > STDX_TEST_FLOAT_EPSILON) { \
    x_log_error("\t%s:%d: Assertion failed: %s == %s", __FILE__, __LINE__, #actual, #expected); \
    return 1; \
  } \
} while (0)

#define ASSERT_NEQ(actual, expected) do { \
  if ((actual) == (expected)) { \
    x_log_error("\t%s:%d: Assertion failed: %s != %s", __FILE__, __LINE__, #actual, #expected); \
    return 1; \
  } \
} while (0)

typedef int32_t (*STDXTestFunction)();

typedef struct
{
  const char *name;
  STDXTestFunction func;
} STDXTestCase;

#define STDX_TEST(name) {#name, name}

#ifdef STDX_IMPLEMENTATION_TEST

#include <stdio.h>
#include <signal.h>


#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif

static void x_test_internalOnSignal(int signal)
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

int stdx_run_tests(STDXTestCase* tests, int32_t num_tests)
{ 
  signal(SIGABRT, x_test_internalOnSignal);
  signal(SIGFPE,  x_test_internalOnSignal);
  signal(SIGILL,  x_test_internalOnSignal);
  signal(SIGINT,  x_test_internalOnSignal);
  signal(SIGSEGV, x_test_internalOnSignal);
  signal(SIGTERM, x_test_internalOnSignal);

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
#endif  // STDX_IMPLEMENTATION_TEST


#ifdef STDX_INTERNAL_TIME_IMPLEMENTATION
  #undef STDX_IMPLEMENTATION_TIME
  #undef STDX_INTERNAL_TIME_IMPLEMENTATION
#endif


#ifdef STDX_INTERNAL_LOGGER_IMPLEMENTATION
  #undef STDX_IMPLEMENTATION_LOG
  #undef STDX_INTERNAL_LOG_IMPLEMENTATION
#endif


#ifdef __cplusplus
}
#endif

#endif // STDX_TEST_H
