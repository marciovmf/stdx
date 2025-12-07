
#include <stdx_common.h>
#define STDX_IMPLEMENTATION_TEST
#include <stdx_test.h>
#define STDX_IMPLEMENTATION_CPUID
#include <stdx_cpuid.h>

int test_cpuid(void)
{
  XCPUInfo info = x_cpu_info();
  printf("Brand           : %s\n", info.brand_string);
  printf("Logical CPUs    : %6d\n", info.logical_cpus);
  printf("Physical cores  : %6d\n", info.physical_cores);
  printf("Hyperthreading  : %6s\n", info.hyperthreading ? "yes" : "no");
  printf("L1 Cache        : %6d KB\n", info.cache_size_l1_kb);
  printf("L2 Cache        : %6d KB\n", info.cache_size_l2_kb);
  printf("L3 Cache        : %6d KB\n", info.cache_size_l3_kb);
  printf("Feature Flags   :%s%s%s%s%s%s%s%s%s%s%s%s\n",
      (info.feature_flags & CPU_FEATURE_SSE) ? " sse" : "",
      (info.feature_flags & CPU_FEATURE_SSE2) ? " sse2" : "",
      (info.feature_flags & CPU_FEATURE_SSE3) ? " sse3" : "",
      (info.feature_flags & CPU_FEATURE_SSSE3) ? " ssse3" : "",
      (info.feature_flags & CPU_FEATURE_SSE41) ? " sse41" : "",
      (info.feature_flags & CPU_FEATURE_SSE42) ? " sse42" : "",
      (info.feature_flags & CPU_FEATURE_AVX) ? " avx" : "",
      (info.feature_flags & CPU_FEATURE_AVX2) ? " avx2" : "",
      (info.feature_flags & CPU_FEATURE_AVX512F) ? " avx512f" : "",
      (info.feature_flags & CPU_FEATURE_NEON) ? " neon" : "",
      (info.feature_flags & CPU_FEATURE_AES) ? " aes" : "",
      (info.feature_flags & CPU_FEATURE_CRC32) ? " crc32" : "");


  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    STDX_TEST(test_cpuid)
  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}
