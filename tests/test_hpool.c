/* test_stdx_hpool.c */

#define X_IMPL_TEST
#include <stdx_test.h>

#define X_IMPL_HPOOL
#include "stdx_hpool.h"

#include <stdint.h>

/* ----------------------------- test helpers ----------------------------- */

typedef struct THPItem
{
  uint32_t magic;
  uint32_t value;
} THPItem;

static void thp_ctor(void* user, void* item)
{
  (void)user;

  THPItem* it;
  it = (THPItem*)item;

  it->magic = 0xC0FFEE01u;
  it->value = 0u;
}

static void thp_dtor(void* user, void* item)
{
  (void)user;

  THPItem* it;
  it = (THPItem*)item;

  it->magic = 0xDEADDEADu;
}

/* ------------------------------- test cases ------------------------------ */

static int test_x_hpool_init_and_term(void)
{
  XHPool p;
  XHPoolConfig cfg;

  cfg.page_capacity = 8u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);
  ASSERT_TRUE(x_hpool_page_capacity(&p) == 8u);
  ASSERT_TRUE(x_hpool_capacity(&p) == 8u);
  ASSERT_TRUE(x_hpool_alive_count(&p) == 0u);

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_alloc_get_free_basic(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h;
  THPItem* it;

  cfg.page_capacity = 8u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  h = x_hpool_alloc(&p);
  ASSERT_FALSE(x_handle_is_null(h) != 0);
  ASSERT_TRUE(x_hpool_alive_count(&p) == 1u);
  ASSERT_TRUE(x_hpool_is_alive(&p, h) != 0);

  it = (THPItem*)x_hpool_get(&p, h);
  ASSERT_TRUE(it != NULL);
  ASSERT_TRUE(it->magic == 0xC0FFEE01u);

  it->value = 123u;
  ASSERT_TRUE(((THPItem*)x_hpool_get(&p, h))->value == 123u);

  x_hpool_free(&p, h);
  ASSERT_TRUE(x_hpool_alive_count(&p) == 0u);
  ASSERT_FALSE(x_hpool_is_alive(&p, h) != 0);
  ASSERT_TRUE(x_hpool_get(&p, h) == NULL);

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_handle_stale_after_free(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h1;
  XHandle h2;

  cfg.page_capacity = 4u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  h1 = x_hpool_alloc(&p);
  ASSERT_FALSE(x_handle_is_null(h1) != 0);

  x_hpool_free(&p, h1);

  h2 = x_hpool_alloc(&p);
  ASSERT_FALSE(x_handle_is_null(h2) != 0);

  /* Old handle must be invalid */
  ASSERT_TRUE(x_hpool_get(&p, h1) == NULL);
  ASSERT_TRUE(x_hpool_is_alive(&p, h1) == 0);

  ASSERT_TRUE(x_hpool_get(&p, h2) != NULL);
  ASSERT_TRUE(x_hpool_is_alive(&p, h2) != 0);

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_grows_by_pages(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h[10];
  uint32_t i;

  cfg.page_capacity = 4u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);
  ASSERT_TRUE(x_hpool_capacity(&p) == 4u);

  for (i = 0u; i < 10u; i += 1u)
  {
    h[i] = x_hpool_alloc(&p);
    ASSERT_FALSE(x_handle_is_null(h[i]) != 0);
  }

  ASSERT_TRUE(x_hpool_alive_count(&p) == 10u);
  ASSERT_TRUE(x_hpool_capacity(&p) >= 10u);

  for (i = 0u; i < 10u; i += 1u)
  {
    ASSERT_TRUE(x_hpool_get(&p, h[i]) != NULL);
  }

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_iteration_foreach(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h;
  THPItem* it;
  uint32_t i;
  uint32_t sum;

  cfg.page_capacity = 8u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  for (i = 0u; i < 5u; i += 1u)
  {
    h = x_hpool_alloc(&p);
    ASSERT_FALSE(x_handle_is_null(h) != 0);

    it = (THPItem*)x_hpool_get(&p, h);
    ASSERT_TRUE(it != NULL);

    it->value = i + 1u;
  }

  sum = 0u;
  {
    XHandle it_h;
    THPItem* ptr;

    ptr = NULL;
    (void)it_h;

    X_HPOOL_FOREACH(&p, THPItem, it_it, it_h, ptr)
    {
      ASSERT_TRUE(ptr->magic == 0xC0FFEE01u);
      sum = sum + ptr->value;
    }
  }

  ASSERT_TRUE(sum == (1u + 2u + 3u + 4u + 5u));

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_free_compacts_alive_list(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h0;
  XHandle h1;
  XHandle h2;
  uint32_t count;

  cfg.page_capacity = 8u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  h0 = x_hpool_alloc(&p);
  h1 = x_hpool_alloc(&p);
  h2 = x_hpool_alloc(&p);

  ASSERT_TRUE(x_hpool_alive_count(&p) == 3u);

  x_hpool_free(&p, h1);

  ASSERT_TRUE(x_hpool_alive_count(&p) == 2u);
  ASSERT_TRUE(x_hpool_get(&p, h1) == NULL);

  count = 0u;
  {
    XHandle it_h;
    THPItem* ptr;

    ptr = NULL;
    (void)it_h;

    X_HPOOL_FOREACH(&p, THPItem, it_it, it_h, ptr)
    {
      ASSERT_TRUE(ptr->magic == 0xC0FFEE01u);
      count = count + 1u;
    }
  }

  ASSERT_TRUE(count == 2u);

  /* Remaining handles should still be alive */
  ASSERT_TRUE(x_hpool_is_alive(&p, h0) != 0);
  ASSERT_TRUE(x_hpool_is_alive(&p, h2) != 0);

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_clear_empties_pool(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h[6];
  uint32_t i;

  cfg.page_capacity = 4u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  for (i = 0u; i < 6u; i += 1u)
  {
    h[i] = x_hpool_alloc(&p);
    ASSERT_FALSE(x_handle_is_null(h[i]) != 0);
  }

  ASSERT_TRUE(x_hpool_alive_count(&p) == 6u);

  x_hpool_clear(&p);

  ASSERT_TRUE(x_hpool_alive_count(&p) == 0u);

  for (i = 0u; i < 6u; i += 1u)
  {
    ASSERT_TRUE(x_hpool_get(&p, h[i]) == NULL);
    ASSERT_TRUE(x_hpool_is_alive(&p, h[i]) == 0);
  }

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_get_unchecked_dead_returns_null(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h;
  uint32_t idx;

  cfg.page_capacity = 8u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  h = x_hpool_alloc(&p);
  ASSERT_FALSE(x_handle_is_null(h) != 0);

  idx = h.index;

  x_hpool_free(&p, h);

  ASSERT_TRUE(x_hpool_get_unchecked(&p, idx) == NULL);

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_get_fast_dead_returns_null(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h;
  void* ptr;

  cfg.page_capacity = 8u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  h = x_hpool_alloc(&p);
  ASSERT_FALSE(x_handle_is_null(h) != 0);

  x_hpool_free(&p, h);

  ptr = x_hpool_get_fast(&p, h);
  ASSERT_TRUE(ptr == NULL);

  x_hpool_term(&p);
  return 0;
}

static int test_x_hpool_is_alive_dead_is_false(void)
{
  XHPool p;
  XHPoolConfig cfg;
  XHandle h;

  cfg.page_capacity = 8u;
  cfg.initial_pages = 1u;

  ASSERT_TRUE(x_hpool_init(&p, sizeof(THPItem), cfg, thp_ctor, thp_dtor, NULL) != 0);

  h = x_hpool_alloc(&p);
  ASSERT_FALSE(x_handle_is_null(h) != 0);
  ASSERT_TRUE(x_hpool_is_alive(&p, h) != 0);

  x_hpool_free(&p, h);

  ASSERT_TRUE(x_hpool_is_alive(&p, h) == 0);

  x_hpool_term(&p);
  return 0;
}

/* ------------------------------- test runner ------------------------------ */

int main(void)
{
  STDXTestCase cases[] =
  {
    X_TEST(test_x_hpool_init_and_term),
    X_TEST(test_x_hpool_alloc_get_free_basic),
    X_TEST(test_x_hpool_handle_stale_after_free),
    X_TEST(test_x_hpool_grows_by_pages),
    X_TEST(test_x_hpool_iteration_foreach),
    X_TEST(test_x_hpool_free_compacts_alive_list),
    X_TEST(test_x_hpool_clear_empties_pool),
    X_TEST(test_x_hpool_get_unchecked_dead_returns_null),
    X_TEST(test_x_hpool_get_fast_dead_returns_null),
    X_TEST(test_x_hpool_is_alive_dead_is_false)
  };

  return x_tests_run(cases, (uint32_t)(sizeof(cases) / sizeof(cases[0])), NULL);
}
