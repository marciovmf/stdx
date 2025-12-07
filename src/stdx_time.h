/*
 * STDX - Time Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * To compile the implementation define X_IMPL_TIME
 * in **one** source file before including this header.
 *
 * Notes:
 *  Provides a portable threading abstraction for C programs.
 *  Includes Time based comparisson, measurement and arithmetic
 *  ooperations
 */

#ifndef X_TIME_H
#define X_TIME_H

#define X_TIME_VERSION_MAJOR 1
#define X_TIME_VERSION_MINOR 0
#define X_TIME_VERSION_PATCH 0
#define X_TIME_VERSION (X_TIME_VERSION_MAJOR * 10000 + X_TIME_VERSION_MINOR * 100 + X_TIME_VERSION_PATCH)

#ifdef _WIN32
#include <windows.h>
#include <stdint.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XTime_t
  {
    double seconds;
  } XTime;

  typedef struct XTimer_t
  {
#ifdef _WIN32
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
#else
    struct timespec start;
#endif
  } XTimer;

  XTime   x_timer_elapsed(const XTimer* t);     // Return elapsed time since x_timer_start
  double  x_time_milliseconds(XTime t);         // Convert to milliseconds
  double  x_time_microseconds(XTime t);         // Convert to microseconds
  double  x_time_nanoseconds(XTime t);          // Convert to nanoseconds
  XTime   x_time_diff(XTime end, XTime start);  // Compute difference between two times
  XTime   x_time_add(XTime a, XTime b);         // Add two XTime
  XTime   x_time_sub(XTime a, XTime b);         // Subtract two XTime
  int32_t x_time_equals(XTime a, XTime b);      // Compares two XTime
  int32_t x_time_less_than(XTime a, XTime b);
  int32_t x_time_greater_than(XTime a, XTime b);
  void    x_time_sleep(XTime t);                // Cross-platform sleep
  XTime   x_time_now();                         // Wall-clock time since Unix epoch

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_TIME

#ifdef __cplusplus
extern "C" {
#endif

// Start the timer
void x_timer_start(XTimer* t)
{
#ifdef _WIN32
  QueryPerformanceFrequency(&t->freq);
  QueryPerformanceCounter(&t->start);
#else
  clock_gettime(CLOCK_MONOTONIC, &t->start);
#endif
}

// Return elapsed time since x_timer_start
XTime x_timer_elapsed(const XTimer* t)
{
  XTime result;
#ifdef _WIN32
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  result.seconds = (double)(now.QuadPart - t->start.QuadPart) / t->freq.QuadPart;
#else
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  result.seconds = (now.tv_sec - t->start.tv_sec) + (now.tv_nsec - t->start.tv_nsec) / 1e9;
#endif
  return result;
}

double x_time_milliseconds(XTime t)
{
  return t.seconds * 1000.0;
}

double x_time_microseconds(XTime t)
{
  return t.seconds * 1e6;
}

double x_time_nanoseconds(XTime t)
{
  return t.seconds * 1e9;
}

XTime x_time_diff(XTime end, XTime start)
{
  XTime diff = { .seconds = end.seconds - start.seconds };
  return diff;
}

XTime x_time_add(XTime a, XTime b)
{
  return (XTime){ .seconds = a.seconds + b.seconds };
}

XTime x_time_sub(XTime a, XTime b)
{
  return (XTime){ .seconds = a.seconds - b.seconds };
}

int32_t x_time_equals(XTime a, XTime b)
{
  return a.seconds == b.seconds;
}

int32_t x_time_less_than(XTime a, XTime b)
{
  return a.seconds < b.seconds;
}

int32_t x_time_greater_than(XTime a, XTime b)
{
  return a.seconds > b.seconds;
}

// Cross-platform sleep
void x_time_sleep(XTime t)
{
#ifdef _WIN32
  Sleep((DWORD)(t.seconds * 1000.0));
#else
  struct timespec ts;
  ts.tv_sec = (time_t)t.seconds;
  ts.tv_nsec = (long)((t.seconds - ts.tv_sec) * 1e9);
  nanosleep(&ts, NULL);
#endif
}

// Wall-clock time since Unix epoch
XTime x_time_now()
{
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  uint64_t ticks = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  double seconds = ticks / 10000000.0 - 11644473600.0; // FILETIME starts in 1601
  return (XTime){ .seconds = seconds };
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (XTime){ .seconds = ts.tv_sec + ts.tv_nsec / 1e9 };
#endif
}

#ifdef __cplusplus
}
#endif

#endif  // X_IMPL_TIME
#endif  // X_TIME_H
