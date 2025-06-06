#include <stdx_common.h>
#define STDX_IMPLEMENTATION_TEST
#include <stdx_test.h>
#define STDX_IMPLEMENTATION_HASHTABLE
#include <stdx_hashtable.h>
#include <stdx_allocator.h>
#include <stdx_arena.h>

int test_x_hashtable_basic_malloc()
{
  XHashtable* ht = x_hashtable_create(sizeof(char*), sizeof(int), stdx_hash_str, stdx_str_eq);

  const char* key = "answer";
  x_hashtable_set(ht, &key, X_VALUE_PTR(int,42));

  ASSERT_TRUE(x_hashtable_has(ht, &key));
  ASSERT_TRUE(x_hashtable_count(ht) == 1);

  int out = 0;
  ASSERT_TRUE(x_hashtable_get(ht, &key, &out));
  ASSERT_TRUE(out == 42);

  x_hashtable_remove(ht, &key);
  ASSERT_FALSE(x_hashtable_has(ht, &key));
  ASSERT_TRUE(x_hashtable_count(ht) == 0);

  x_hashtable_destroy(ht);
  return 0;
}

int test_x_hashtable_rehash_malloc()
{
  XHashtable* ht = x_hashtable_create(sizeof(char*), sizeof(int), stdx_hash_str, stdx_str_eq);

  // Insert enough keys to trigger a rehash
  char keybuf[32];
  for (int i = 0; i < 100; ++i)
  {
    snprintf(keybuf, sizeof(keybuf), "key%d", i);
    int value = i;
    x_hashtable_set(ht, &keybuf, &value);
  }
  ASSERT_TRUE(x_hashtable_count(ht) == 100);

  // Verify some entries
  for (int i = 0; i < 100; ++i)
  {
    snprintf(keybuf, sizeof(keybuf), "key%d", i);
    int out = -1;
    ASSERT_TRUE(x_hashtable_get(ht, &keybuf, &out));
    ASSERT_TRUE(out == i);
  }

  x_hashtable_destroy(ht);
  return 0;
}

int test_x_hashtable_with_arena()
{
  XArena* arena = x_arena_create(2048);
  XAllocator a = x_arena_allocator(arena);

  XHashtable* ht = x_hashtable_create_ex(sizeof(char*), sizeof(int), stdx_hash_str, stdx_str_eq, &a);

  const char* key = "fruit";
  x_hashtable_set(ht, &key, X_VALUE_PTR(int, 17));
  ASSERT_TRUE(x_hashtable_count(ht) == 1);

  int out = 0;
  ASSERT_TRUE(x_hashtable_get(ht, &key, &out));
  ASSERT_TRUE(out == 17);

  ASSERT_TRUE(x_hashtable_has(ht, &key));

  // Removing still works
  x_hashtable_remove(ht, &key);
  ASSERT_FALSE(x_hashtable_has(ht, &key));
  ASSERT_TRUE(x_hashtable_count(ht) == 0);

  x_arena_destroy(arena); // cleans everything
  return 0;
}

int test_x_hashtable_rehash_with_arena()
{
  XArena* arena = x_arena_create(4096);
  XAllocator a = x_arena_allocator(arena);

  XHashtable* ht = x_hashtable_create_ex(sizeof(char*), sizeof(int),
      stdx_hash_str, stdx_str_eq, &a);

  char keybuf[32];
  for (int i = 0; i < 100; ++i)
  {
    snprintf(keybuf, sizeof(keybuf), "k%d", i);
    int value = i * 2;
    x_hashtable_set(ht, &keybuf, &value);
  }

  for (int i = 0; i < 100; ++i)
  {
    snprintf(keybuf, sizeof(keybuf), "k%d", i);
    int out = -1;
    ASSERT_TRUE(x_hashtable_get(ht, &keybuf, &out));
    ASSERT_TRUE(out == i * 2);
  }

  x_arena_destroy(arena);
  return 0;
}

int test_x_hashtable_iteration()
{
  XHashtable* ht =
    x_hashtable_create(sizeof(char*), sizeof(int), stdx_hash_str, stdx_str_eq);

  const char* keys[] = {"a", "b", "c"};
  int vals[] = {1, 2, 3};

  for (int i = 0; i < 3; ++i)
    x_hashtable_set(ht, &keys[i], &vals[i]);

  int seen = 0;
  XHashIter iter;
  x_hashtable_iter_init(&iter, ht);
  void* k; void* v;
  while (x_hashtable_iter_next(&iter, &k, &v))
  {
    ASSERT_TRUE(*(char**)k != NULL);
    ASSERT_TRUE(*(int*)v > 0);
    seen++;
  }
  ASSERT_TRUE(seen == 3);
  x_hashtable_destroy(ht);
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    STDX_TEST(test_x_hashtable_basic_malloc),
    STDX_TEST(test_x_hashtable_rehash_malloc),
    STDX_TEST(test_x_hashtable_with_arena),
    STDX_TEST(test_x_hashtable_rehash_with_arena),
    STDX_TEST(test_x_hashtable_iteration)
  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}
