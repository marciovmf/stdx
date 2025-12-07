/*
 * STDX - CPU information utility
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * This module provides a portable function to obtain CPU information
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_CPUID
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "stdx_cpuid.h"
 */
#ifndef STDX_CPUID_H
#define STDX_CPUID_H

#ifdef __cplusplus
extern "C" {
#endif

#define STDX_CPUID_VERSION_MAJOR 1
#define STDX_CPUID_VERSION_MINOR 0
#define STDX_CPUID_VERSION_PATCH 0

#define STDX_CPUID_VERSION (STDX_CPUID_VERSION_MAJOR * 10000 + STDX_CPUID_VERSION_MINOR * 100 + STDX_CPUID_VERSION_PATCH)

#include <stdint.h>
#include <stdbool.h>

#if defined(_MSC_VER) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
#define STDX_CPUID_SUPPORTED 1
#else
#define STDX_CPUID_SUPPORTED 0
#endif

  typedef enum
  {
    CPU_FEATURE_NONE    = 0,
    CPU_FEATURE_SSE     = 1 << 0,
    CPU_FEATURE_SSE2    = 1 << 1,
    CPU_FEATURE_SSE3    = 1 << 2,
    CPU_FEATURE_SSSE3   = 1 << 3,
    CPU_FEATURE_SSE41   = 1 << 4,
    CPU_FEATURE_SSE42   = 1 << 5,
    CPU_FEATURE_AVX     = 1 << 6,
    CPU_FEATURE_AVX2    = 1 << 7,
    CPU_FEATURE_AVX512F = 1 << 8,
    CPU_FEATURE_NEON    = 1 << 16,
    CPU_FEATURE_AES     = 1 << 17,
    CPU_FEATURE_CRC32   = 1 << 18,
  } CPUFeature;

  typedef struct
  {
    int logical_cpus;
    int physical_cores;
    int sockets;
    bool hyperthreading;
    int cache_size_l1_kb;
    int cache_size_l2_kb;
    int cache_size_l3_kb;
    char brand_string[49];  // 3 * 16bytes + null terminator = 49
    CPUFeature feature_flags;
  } XCPUInfo;


  XCPUInfo x_cpu_info();

#ifdef __cplusplus
}
#endif

#ifdef STDX_IMPLEMENTATION_CPUID

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif

#if defined(__linux__) || defined(__ANDROID__)
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/auxv.h>
#include <asm/hwcap.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>
#endif

#include <string.h>

static void stdx_cpuid(uint32_t eax, uint32_t ecx,
    uint32_t* out_eax, uint32_t* out_ebx,
    uint32_t* out_ecx, uint32_t* out_edx)
{
#if defined(_MSC_VER)
  int regs[4];
  __cpuidex(regs, eax, ecx);
  if (out_eax) *out_eax = regs[0];
  if (out_ebx) *out_ebx = regs[1];
  if (out_ecx) *out_ecx = regs[2];
  if (out_edx) *out_edx = regs[3];

#elif defined(__GNUC__) || defined(__clang__)
  uint32_t a, b, c, d;
  __asm__ __volatile__(
      "cpuid"
      : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
      : "a"(eax), "c"(ecx));
  if (out_eax) *out_eax = a;
  if (out_ebx) *out_ebx = b;
  if (out_ecx) *out_ecx = c;
  if (out_edx) *out_edx = d;
#else
  if (out_eax) *out_eax = 0;
  if (out_ebx) *out_ebx = 0;
  if (out_ecx) *out_ecx = 0;
  if (out_edx) *out_edx = 0;
#endif
}

static inline uint32_t stdx_cpuid_max_leaf(uint32_t leaf_type)
{
  uint32_t a = 0;
  stdx_cpuid(leaf_type, 0, &a, NULL, NULL, NULL);
  return a;
}

XCPUInfo x_cpu_info()
{
  XCPUInfo info = {0};

  // Brand string
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  {
    uint32_t regs[4];
    char* p = info.brand_string;

    for (uint32_t i = 0; i < 3; ++i) {
      stdx_cpuid(0x80000002 + i, 0, &regs[0], &regs[1], &regs[2], &regs[3]);
      memcpy(p +  0, &regs[0], 4);
      memcpy(p +  4, &regs[1], 4);
      memcpy(p +  8, &regs[2], 4);
      memcpy(p + 12, &regs[3], 4);
      p += 16;
    }
    info.brand_string[48] = '\0';
  }
#endif

  //--------------------------------------------
  // Core/thread detection
  //--------------------------------------------

#if defined(_WIN32)
  DWORD len = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buf = malloc(len);
  if (buf && GetLogicalProcessorInformationEx(RelationProcessorCore, buf, &len))
  {
    char* p = (char*)buf, *end = p + len;
    while (p < end) {
      SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* e = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)p;
      if (e->Relationship == RelationProcessorCore) {
        info.physical_cores++;
        KAFFINITY m = e->Processor.GroupMask[0].Mask;
        for (; m; m >>= 1)
          info.logical_cpus += m & 1;
      }
      p += e->Size;
    }
    free(buf);
  }
  info.hyperthreading = info.logical_cpus > info.physical_cores;
  info.sockets = 1;

#elif defined(__APPLE__)
  size_t sz = sizeof(int);
  sysctlbyname("hw.logicalcpu", &info.logical_cpus, &sz, NULL, 0);
  sysctlbyname("hw.physicalcpu", &info.physical_cores, &sz, NULL, 0);
  info.hyperthreading = info.logical_cpus > info.physical_cores;
  info.sockets = 1;

#elif defined(__linux__)
  info.logical_cpus = sysconf(_SC_NPROCESSORS_ONLN);

  FILE* f = fopen("/proc/cpuinfo", "r");
  if (f)
  {
    int seen[4096] = {0}, phy = -1, core = -1;
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
      if (!memcmp(line, "physical id", 11))
        sscanf(line, "physical id : %d", &phy);
      if (!memcmp(line, "core id", 7))
        sscanf(line, "core id : %d", &core);
      if (phy >= 0 && core >= 0)
      {
        int k = (phy << 12) | core;
        if (!seen[k])
        {
          seen[k] = 1;
          info.physical_cores++;
        }
        core = -1;
      }
    }
    fclose(f);
  }
  if (info.physical_cores == 0)
    info.physical_cores = info.logical_cpus;
  info.hyperthreading = info.logical_cpus > info.physical_cores;
  info.sockets = 1;
#endif


  //--------------------------------------------
  // Cache sizes (x86/64 only)
  //--------------------------------------------

#if STDX_CPUID_SUPPORTED
  for (int level = 1; level <= 3; ++level)
  {
    for (int i = 0; i < 8; ++i)
    {
      uint32_t eax, ebx, ecx, edx;
      stdx_cpuid(4, i, &eax, &ebx, &ecx, &edx);
      int type = eax & 0x1F;
      if (type == 0) continue;
      int lvl = (eax >> 5) & 0x7;
      if (lvl != level) continue;

      int ways  = ((ebx >> 22) & 0x3FF) + 1;
      int parts = ((ebx >> 12) & 0x3FF) + 1;
      int lines = (ebx & 0xFFF) + 1;
      int sets  = ecx + 1;
      int size_kb = (ways * parts * lines * sets) / 1024;

      if (level == 1) info.cache_size_l1_kb = size_kb;
      if (level == 2) info.cache_size_l2_kb = size_kb;
      if (level == 3) info.cache_size_l3_kb = size_kb;
    }
  }
#endif

  //--------------------------------------------
  // Features
  //--------------------------------------------

  info.feature_flags = CPU_FEATURE_NONE;

#if STDX_CPUID_SUPPORTED
  {
    uint32_t eax, ebx, ecx, edx;

    // Basic feature bits
    stdx_cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    if (edx & (1 << 25)) info.feature_flags |= CPU_FEATURE_SSE;
    if (edx & (1 << 26)) info.feature_flags |= CPU_FEATURE_SSE2;
    if (ecx & (1 << 0))  info.feature_flags |= CPU_FEATURE_SSE3;
    if (ecx & (1 << 9))  info.feature_flags |= CPU_FEATURE_SSSE3;
    if (ecx & (1 << 19)) info.feature_flags |= CPU_FEATURE_SSE41;
    if (ecx & (1 << 20)) info.feature_flags |= CPU_FEATURE_SSE42;
    if (ecx & (1 << 28)) info.feature_flags |= CPU_FEATURE_AVX;

    if (stdx_cpuid_max_leaf(0) >= 7)
    {
      stdx_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
      if (ebx & (1 << 5))   info.feature_flags |= CPU_FEATURE_AVX2;
      if (ebx & (1 << 16))  info.feature_flags |= CPU_FEATURE_AVX512F;
    }
  }
#endif

#if defined(__aarch64__) || defined(__arm__)
  {
    unsigned long hwcap = 0;

#if defined(__linux__) || defined(__ANDROID__)
    hwcap = getauxval(AT_HWCAP);
#elif defined(__APPLE__)
    hwcap = 0; // NEON is always available on Apple ARM
#endif

    if (hwcap & HWCAP_NEON)  info.feature_flags |= CPU_FEATURE_NEON;
#if defined(HWCAP_CRC32)
    if (hwcap & HWCAP_CRC32) info.feature_flags |= CPU_FEATURE_CRC32;
#endif
#if defined(HWCAP_AES)
    if (hwcap & HWCAP_AES)   info.feature_flags |= CPU_FEATURE_AES;
#endif

    // On Apple ARM, assume NEON/AES/CRC32 are supported
#if defined(__APPLE__) && defined(__aarch64__)
    info.feature_flags |= CPU_FEATURE_NEON | CPU_FEATURE_AES | CPU_FEATURE_CRC32;
#endif
  }
#endif


  return info;
}

#endif  // STDX_IMPLEMENTATION_CPUID
#endif // STDX_CPUID_H
