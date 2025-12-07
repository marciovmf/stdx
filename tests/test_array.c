#include <stdx_common.h>
#define X_IMPL_ARRAY
#include <stdx_array.h>
#define X_IMPL_TEST
#include <stdx_test.h>

int test_x_array_create()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(arr != NULL);
  ASSERT_TRUE(x_array_capacity(arr) == 10);
  ASSERT_TRUE(x_array_count(arr) == 0);
  x_array_destroy(arr);

  return 0;
}

int test_x_array_add()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  int value = 5;
  x_array_add(arr, &value);
  ASSERT_TRUE(x_array_count(arr) == 1);
  ASSERT_TRUE(*(int*)x_array_get(arr, 0).ptr == value);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_insert()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  int value1 = 5;
  int value2 = 10;
  x_array_add(arr, &value1);
  x_array_insert(arr, &value2, 0);
  ASSERT_TRUE(x_array_count(arr) == 2);
  ASSERT_TRUE(*(int*)x_array_get(arr, 0).ptr == value2);
  ASSERT_TRUE(*(int*)x_array_get(arr, 1).ptr == value1);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_get()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  int value = 5;
  x_array_add(arr, &value);
  int* result = (int*)x_array_get(arr, 0).ptr;
  ASSERT_TRUE(result != NULL);
  ASSERT_TRUE(*result == value);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_get_data()
{
  XArray* arr = x_array_create(sizeof(int), 5);
  ASSERT_TRUE(x_array_capacity(arr) == 5);
  int value;
  value = 10; x_array_add(arr, &value);
  value = 20; x_array_add(arr, &value);
  value = 30; x_array_add(arr, &value);
  value = 40; x_array_add(arr, &value);
  value = 50; x_array_add(arr, &value);
  // The next add shoud cause the the array to resize
  value = 60; x_array_add(arr, &value);
  ASSERT_TRUE(x_array_capacity(arr) == 10);
  value = 70; x_array_add(arr, &value);

  int* data = (int*)x_array_getdata(arr).ptr;
  ASSERT_TRUE(data != NULL);
  ASSERT_TRUE(data[0] == 10);
  ASSERT_TRUE(data[1] == 20);
  ASSERT_TRUE(data[2] == 30);
  ASSERT_TRUE(data[3] == 40);
  ASSERT_TRUE(data[4] == 50);
  ASSERT_TRUE(data[5] == 60);
  ASSERT_TRUE(data[6] == 70);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_count()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(x_array_count(arr) == 0);
  int value = 5;
  x_array_add(arr, &value);
  ASSERT_TRUE(x_array_count(arr) == 1);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_capacity()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(x_array_capacity(arr) == 10);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_delete_range()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  int values[] =
  {1, 2, 3, 4, 5};
  for (int i = 0; i < 5; i++)
  {
    ASSERT_TRUE(x_array_add(arr, &values[i]) == XARRAY_OK);
  }

  ASSERT_TRUE(x_array_delete_range(arr, 1, 10)    == XARRAY_INVALID_RANGE);
  ASSERT_TRUE(x_array_delete_range(arr, 10, 1)    == XARRAY_INVALID_RANGE);
  ASSERT_TRUE(x_array_delete_range(arr, 10, 10)   == XARRAY_INVALID_RANGE);
  ASSERT_TRUE(x_array_delete_range(arr, -10, -3)  == XARRAY_INVALID_RANGE);
  ASSERT_TRUE(x_array_delete_range(arr, -1, -30)  == XARRAY_INVALID_RANGE);

  ASSERT_TRUE(x_array_count(arr) == 5);
  ASSERT_TRUE(x_array_delete_range(arr, 1, 3) == XARRAY_OK);
  ASSERT_TRUE(x_array_count(arr) == 2);
  ASSERT_TRUE(X_PTR_IS_OK(x_array_get(arr, 0)));
  ASSERT_TRUE(X_PTR_IS_OK(x_array_get(arr, 1)));
  ASSERT_TRUE(*(int*)x_array_get(arr, 0).ptr == 1);
  ASSERT_TRUE(*(int*)x_array_get(arr, 1).ptr == 5);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_clear()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  int value = 5;
  x_array_add(arr, &value);
  x_array_clear(arr);
  ASSERT_TRUE(x_array_count(arr) == 0);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_delete_at()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  int values[] =
  {1, 2, 3};
  for (int i = 0; i < 3; i++)
  {
    x_array_add(arr, &values[i]);
  }
  ASSERT_TRUE(x_array_count(arr) == 3);
  x_array_delete_at(arr, 1);
  ASSERT_TRUE(x_array_count(arr) == 2);
  ASSERT_TRUE(*(int*)x_array_get(arr, 0).ptr == 1);
  ASSERT_TRUE(*(int*)x_array_get(arr, 1).ptr == 3);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_push_and_top()
{

  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(arr != NULL);

  int value = 42;
  x_array_push(arr, &value);

  int* top = (int*)x_array_top(arr).ptr;
  ASSERT_TRUE(top != NULL);
  ASSERT_EQ(*top, 42);

  x_array_destroy(arr);
  return 0;
}

int test_x_array_push_multiple()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(arr != NULL);

  int a = 1, b = 2, c = 3;
  x_array_push(arr, &a);
  x_array_push(arr, &b);
  x_array_push(arr, &c);

  int* top = (int*)x_array_top(arr).ptr;
  ASSERT_EQ(*top, 3);

  x_array_destroy(arr);
  return 0;
}

int test_x_array_pop()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(arr != NULL);

  int x = 100, y = 200;
  x_array_push(arr, &x);
  x_array_push(arr, &y);
  x_array_pop(arr);  // removes 200

  int* top = (int*)x_array_top(arr).ptr;
  ASSERT_EQ(*top, 100);

  x_array_destroy(arr);
  return 0;
}

int test_x_array_is_empty()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(arr != NULL);
  ASSERT_TRUE(x_array_is_empty(arr));

  int val = 7;
  x_array_push(arr, &val);
  ASSERT_FALSE(x_array_is_empty(arr));

  x_array_pop(arr);
  ASSERT_TRUE(x_array_is_empty(arr));

  x_array_destroy(arr);
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_x_array_create),
    X_TEST(test_x_array_add),
    X_TEST(test_x_array_insert),
    X_TEST(test_x_array_get),
    X_TEST(test_x_array_get_data),
    X_TEST(test_x_array_count),
    X_TEST(test_x_array_capacity),
    X_TEST(test_x_array_delete_range),
    X_TEST(test_x_array_clear),
    X_TEST(test_x_array_delete_at),
    X_TEST(test_x_array_push_and_top),
    X_TEST(test_x_array_push_multiple),
    X_TEST(test_x_array_pop),
    X_TEST(test_x_array_is_empty),
  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]));
}
