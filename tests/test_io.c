#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_IO
#include <stdx_io.h>

#include <stdlib.h>

#define TEMP_FILE "test_tmp_io_file.txt"
#define STR1 "Hello, world!"
#define STR2 " Goodbye."

int test_write_and_read_text()
{
  ASSERT_TRUE(x_io_write_text(TEMP_FILE, STR1));
  char *text = x_io_read_text(TEMP_FILE, NULL);
  ASSERT_TRUE(text);
  ASSERT_TRUE(strcmp(text, STR1) == 0);
  free(text);
  return 0;
}

int test_append_text()
{
  ASSERT_TRUE(x_io_append_text(TEMP_FILE, STR2));

  char *text = x_io_read_text(TEMP_FILE, NULL);
  ASSERT_TRUE(text);
  ASSERT_TRUE(strcmp(text, STR1 STR2) == 0);
  free(text);
  return 0;
}

int test_read_all()
{
  XFile *f = x_io_open(TEMP_FILE, "rb");
  ASSERT_TRUE(f);

  size_t len = 0;
  char *contents = x_io_read_all(f, &len);
  ASSERT_TRUE(contents);
  ASSERT_TRUE(len == strlen(STR1 STR2));
  ASSERT_TRUE(memcmp(contents, STR1 STR2, len) == 0);
  ASSERT_TRUE(contents[len] == '\0');

  x_io_close(f);
  free(contents);
  return 0;
}

int test_seek_tell()
{
  XFile *f = x_io_open(TEMP_FILE, "rb");
  ASSERT_TRUE(f);

  ASSERT_TRUE(x_io_seek(f, 7, SEEK_SET));
  long pos = x_io_tell(f);
  ASSERT_TRUE(pos == 7);

  char c;
  size_t r = x_io_read(f, &c, 1);
  ASSERT_TRUE(r == 1);
  ASSERT_TRUE(c == 'w');

  x_io_close(f);
  return 0;
}

int test_eof_error()
{
  XFile *f = x_io_open(TEMP_FILE, "rb");
  ASSERT_TRUE(f);

  ASSERT_TRUE(x_io_seek(f, 0, SEEK_END));
  char c;
  size_t r = x_io_read(f, &c, 1);
  ASSERT_TRUE(r == 0);
  ASSERT_TRUE(x_io_eof(f));

  x_io_clearerr(f);
  ASSERT_TRUE(!x_io_error(f));
  ASSERT_TRUE(!x_io_eof(f));

  x_io_close(f);
  return 0;
}

int test_allocator_usage()
{
  XFile *f = x_io_open(TEMP_FILE, "rb");
  ASSERT_TRUE(f);
  size_t len = 0;
  char *contents = x_io_read_all(f, &len);
  ASSERT_TRUE(contents);
  ASSERT_TRUE(memcmp(contents, STR1 STR2, len) == 0);
  x_io_close(f);
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_write_and_read_text),
    X_TEST(test_append_text),
    X_TEST(test_read_all),
    X_TEST(test_seek_tell),
    X_TEST(test_eof_error),
    X_TEST(test_allocator_usage)
  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]));
}

