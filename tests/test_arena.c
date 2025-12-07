#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_ARENA
#define X_ARENA_TESTING
#include <stdx_arena.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

int test_x_arena_create_destroy()
{
  XArena* arena = x_arena_create(1024);
  ASSERT_TRUE(arena != NULL);
  ASSERT_TRUE(arena->chunks != NULL);
  ASSERT_TRUE(arena->chunk_size == 1024);
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_simple_alloc()
{
  XArena* arena = x_arena_create(128);
  void* ptr = x_arena_alloc(arena, 64);
  ASSERT_TRUE(ptr != NULL);
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_multi_alloc_same_chunk()
{
  XArena* arena = x_arena_create(128);
  void* a = x_arena_alloc(arena, 32);
  void* b = x_arena_alloc(arena, 32);
  void* c = x_arena_alloc(arena, 32);
  ASSERT_TRUE(a && b && c);
  ASSERT_TRUE(a != b && b != c && a != c);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 1); /* all fit head */
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_alloc_triggers_new_chunk()
{
  XArena* arena = x_arena_create(64);
  void* a = x_arena_alloc(arena, 60);
  void* b = x_arena_alloc(arena, 60); /* should trigger new chunk */
  ASSERT_TRUE(a && b);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 2);
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_alloc_large_block()
{
  XArena* arena = x_arena_create(64);
  void* big = x_arena_alloc(arena, 512);
  ASSERT_TRUE(big != NULL);
  // The newest chunk is at head; it should be >= 512
  ASSERT_TRUE(x_arena_head_capacity(arena) >= 512);
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_reset_allows_reuse()
{
  XArena* arena = x_arena_create(128);
  void* a = x_arena_alloc(arena, 64);
  ASSERT_TRUE(a != NULL);

  x_arena_reset(arena);
  // After reset, head is same chunk and used is 0
  ASSERT_TRUE(x_arena_chunk_count(arena) == 1);
  ASSERT_TRUE(x_arena_head_used(arena) == 0);
  void* b = x_arena_alloc(arena, 64);
  ASSERT_TRUE(b != NULL);

  // Don’t assert a == b; alignment/padding may differ.
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_null_and_zero_alloc()
{
  XArena* arena = x_arena_create(128);
  void* a = x_arena_alloc(arena, 0);
  ASSERT_TRUE(a == NULL);

  void* b = x_arena_alloc(NULL, 64);
  ASSERT_TRUE(b == NULL);
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_alignment_respected()
{
  XArena* arena = x_arena_create(128);

  void* p1 = x_arena_alloc(arena, 1);      /* odd size to stress alignment */
  void* p2 = x_arena_alloc(arena, 8);
  void* p3 = x_arena_alloc(arena, 24);

  ASSERT_TRUE(p1 && p2 && p3);

  uintptr_t a1 = (uintptr_t)p1;
  uintptr_t a2 = (uintptr_t)p2;
  uintptr_t a3 = (uintptr_t)p3;

  ASSERT_TRUE((a1 % X_ARENA_ALIGN) == 0);
  ASSERT_TRUE((a2 % X_ARENA_ALIGN) == 0);
  ASSERT_TRUE((a3 % X_ARENA_ALIGN) == 0);

  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_alloc_zero_sets_bytes()
{
  XArena* arena = x_arena_create(128);

  size_t n = 37; // odd size to catch partial words
  uint8_t* p = (uint8_t*)x_arena_alloc_zero(arena, n);
  ASSERT_TRUE(p != NULL);

  for (size_t i = 0; i < n; ++i)
    ASSERT_TRUE(p[i] == 0);

  // size==0 returns NULL
  ASSERT_TRUE(x_arena_alloc_zero(arena, 0) == NULL);

  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_strdup_copies_and_null_terminates()
{
  XArena* arena = x_arena_create(128);

  const char* s = "underive the world";
  char* d = x_arena_strdup(arena, s);
  ASSERT_TRUE(d != NULL);
  ASSERT_TRUE(strcmp(d, s) == 0);

  size_t n = strlen(s);
  ASSERT_TRUE(d[n] == '\0');

  // NULL input returns NULL
  ASSERT_TRUE(x_arena_strdup(arena, NULL) == NULL);

  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_reset_keep_head_frees_extra_chunks()
{
  XArena* arena = x_arena_create(64);

  // Fill first chunk, then force second
  ASSERT_TRUE(x_arena_alloc(arena, 60) != NULL);
  ASSERT_TRUE(x_arena_alloc(arena, 60) != NULL);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 2);

  x_arena_reset_keep_head(arena);

  ASSERT_TRUE(x_arena_chunk_count(arena) == 1);
  ASSERT_TRUE(x_arena_head_used(arena) == 0);

  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_trim_keeps_first_n_chunks()
{
  XArena* arena = x_arena_create(64);

  // Build 3 chunks total
  ASSERT_TRUE(x_arena_alloc(arena, 60) != NULL); // chunk 1
  ASSERT_TRUE(x_arena_alloc(arena, 60) != NULL); // chunk 2
  ASSERT_TRUE(x_arena_alloc(arena, 60) != NULL); // chunk 3

  ASSERT_TRUE(x_arena_chunk_count(arena) == 3);

  x_arena_trim(arena, 2);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 2);

  x_arena_trim(arena, 1);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 1);

  // Trimming to a larger keep_n than present should be a no-op
  x_arena_trim(arena, 5);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 1);

  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_mark_release_rewinds_and_frees_chunks()
{
  XArena* arena = x_arena_create(64);

  // Some baseline allocations
  ASSERT_TRUE(x_arena_alloc(arena, 16) != NULL);
  size_t used_before_mark = x_arena_head_used(arena);
  XArenaMark m = x_arena_mark(arena);

  // Allocate enough to force a new chunk
  ASSERT_TRUE(x_arena_alloc(arena, 60) != NULL);
  ASSERT_TRUE(x_arena_alloc(arena, 32) != NULL);
  ASSERT_TRUE(x_arena_chunk_count(arena) >= 2);

  x_arena_release(arena, m);

  // After release: only chunks up to the mark remain, head used restored
  ASSERT_TRUE(x_arena_chunk_count(arena) == 1);
  ASSERT_TRUE(x_arena_head_used(arena) == used_before_mark);

  // And we can allocate again cleanly
  ASSERT_TRUE(x_arena_alloc(arena, 24) != NULL);

  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_spike_then_trim_recovers_memory_pressure()
{
  XArena* arena = x_arena_create(64);

  // steady state
  ASSERT_TRUE(x_arena_alloc(arena, 16) != NULL);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 1);

  // spike: many big allocations create more chunks
  for (int i = 0; i < 5; ++i)
    ASSERT_TRUE(x_arena_alloc(arena, 60) != NULL);

  ASSERT_TRUE(x_arena_chunk_count(arena) > 1);

  // recover: keep only the head, zero used
  x_arena_reset_keep_head(arena);
  ASSERT_TRUE(x_arena_chunk_count(arena) == 1);
  ASSERT_TRUE(x_arena_head_used(arena) == 0);

  x_arena_destroy(arena);
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_x_arena_create_destroy),
    X_TEST(test_x_arena_simple_alloc),
    X_TEST(test_x_arena_multi_alloc_same_chunk),
    X_TEST(test_x_arena_alloc_triggers_new_chunk),
    X_TEST(test_x_arena_alloc_large_block),
    X_TEST(test_x_arena_reset_allows_reuse),
    X_TEST(test_x_arena_null_and_zero_alloc),
    X_TEST(test_x_arena_alignment_respected),
    X_TEST(test_x_arena_strdup_copies_and_null_terminates),
    X_TEST(test_x_arena_reset_keep_head_frees_extra_chunks),
    X_TEST(test_x_arena_trim_keeps_first_n_chunks),
    X_TEST(test_x_arena_mark_release_rewinds_and_frees_chunks),
    X_TEST(test_x_arena_spike_then_trim_recovers_memory_pressure)
  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]));
}

