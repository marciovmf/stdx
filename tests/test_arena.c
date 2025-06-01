#define STDX_IMPLEMENTATION_TEST
#include <stdx_test.h>
#define STDX_IMPLEMENTATION_ARENA
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
  ASSERT_TRUE(a != NULL);
  ASSERT_TRUE(b != NULL);
  ASSERT_TRUE(c != NULL);
  ASSERT_TRUE(a != b && b != c && a != c); // all distinct
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_alloc_triggers_new_chunk()
{
  XArena* arena = x_arena_create(64);
  void* a = x_arena_alloc(arena, 60);
  void* b = x_arena_alloc(arena, 60); // should trigger new chunk
  ASSERT_TRUE(a != NULL && b != NULL);
  ASSERT_TRUE(a != b);

  XArenaChunk* first = arena->chunks;
  XArenaChunk* second = first->next;
  ASSERT_TRUE(second != NULL); // new chunk added
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_alloc_large_block()
{
  XArena* arena = x_arena_create(64);
  void* big = x_arena_alloc(arena, 512);
  ASSERT_TRUE(big != NULL);

  // Should be allocated in a dedicated chunk
  XArenaChunk* first = arena->chunks;
  ASSERT_TRUE(first->capacity >= 512);
  x_arena_destroy(arena);
  return 0;
}

int test_x_arena_reset_allows_reuse()
{
  XArena* arena = x_arena_create(128);
  void* a = x_arena_alloc(arena, 64);
  ASSERT_TRUE(a != NULL);

  x_arena_reset(arena);
  void* b = x_arena_alloc(arena, 64);
  ASSERT_TRUE(b != NULL);
  ASSERT_TRUE(a == b); // likely same pointer reused
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

int main()
{
  STDXTestCase tests[] =
  {
    STDX_TEST(test_x_arena_create_destroy),
    STDX_TEST(test_x_arena_simple_alloc),
    STDX_TEST(test_x_arena_multi_alloc_same_chunk),
    STDX_TEST(test_x_arena_alloc_triggers_new_chunk),
    STDX_TEST(test_x_arena_alloc_large_block),
    STDX_TEST(test_x_arena_reset_allows_reuse),
    STDX_TEST(test_x_arena_null_and_zero_alloc)
  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}

