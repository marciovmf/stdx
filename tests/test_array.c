#define STDX_IMPLEMENTATION_TEST
#include <stdx_test.h>
#define STDX_IMPLEMENTATION_ARRAY
#include <stdx_array.h>

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
  ASSERT_TRUE(*(int*)x_array_get(arr, 0) == value);
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
  ASSERT_TRUE(*(int*)x_array_get(arr, 0) == value2);
  ASSERT_TRUE(*(int*)x_array_get(arr, 1) == value1);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_get()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  int value = 5;
  x_array_add(arr, &value);
  int* result = (int*)x_array_get(arr, 0);
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

  int* data = (int*)x_array_getdata(arr);
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
    x_array_add(arr, &values[i]);
  }
  x_array_delete_range(arr, 1, 3);
  ASSERT_TRUE(x_array_count(arr) == 2);
  ASSERT_TRUE(*(int*)x_array_get(arr, 0) == 1);
  ASSERT_TRUE(*(int*)x_array_get(arr, 1) == 5);
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
  x_array_delete_at(arr, 1);
  ASSERT_TRUE(x_array_count(arr) == 2);
  ASSERT_TRUE(*(int*)x_array_get(arr, 0) == 1);
  ASSERT_TRUE(*(int*)x_array_get(arr, 1) == 3);
  x_array_destroy(arr);
  return 0;
}

int test_x_array_push_and_top()
{
  XArray* arr = x_array_create(sizeof(int), 10);
  ASSERT_TRUE(arr != NULL);

  int value = 42;
  x_array_push(arr, &value);

  int* top = (int*)x_array_top(arr);
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

  int* top = (int*)x_array_top(arr);
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

  int* top = (int*)x_array_top(arr);
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
    TEST_CASE(test_x_array_create),
    TEST_CASE(test_x_array_add),
    TEST_CASE(test_x_array_insert),
    TEST_CASE(test_x_array_get),
    TEST_CASE(test_x_array_get_data),
    TEST_CASE(test_x_array_count),
    TEST_CASE(test_x_array_capacity),
    TEST_CASE(test_x_array_delete_range),
    TEST_CASE(test_x_array_clear),
    TEST_CASE(test_x_array_delete_at),
    TEST_CASE(test_x_array_push_and_top),
    TEST_CASE(test_x_array_push_multiple),
    TEST_CASE(test_x_array_pop),
    TEST_CASE(test_x_array_is_empty),
  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}
