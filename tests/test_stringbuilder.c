#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_STRBUILDER
#include <stdx_strbuilder.h>

int test_strbuilder_append_and_to_string()
{
  XStrBuilder* sb = x_strbuilder_create();
  ASSERT_TRUE(sb != NULL);

  x_strbuilder_append(sb, "Hello");
  x_strbuilder_append(sb, ", ");
  x_strbuilder_append(sb, "World!");

  const char* result = x_strbuilder_to_string(sb);
  ASSERT_TRUE(result != NULL);
  ASSERT_EQ(strcmp(result, "Hello, World!"), 0);

  x_strbuilder_destroy(sb);
  return 0;
}

int test_strbuilder_append_char()
{
  XStrBuilder* sb = x_strbuilder_create();
  ASSERT_TRUE(sb != NULL);

  x_strbuilder_append_char(sb, 'A');
  x_strbuilder_append_char(sb, 'B');
  x_strbuilder_append_char(sb, 'C');

  const char* result = x_strbuilder_to_string(sb);
  ASSERT_EQ(strcmp(result, "ABC"), 0);

  x_strbuilder_destroy(sb);
  return 0;
}

int test_strbuilder_append_format()
{
  XStrBuilder* sb = x_strbuilder_create();
  ASSERT_TRUE(sb != NULL);

  x_strbuilder_append_format(sb, "%d + %d = %d", 2, 3, 5);

  const char* result = x_strbuilder_to_string(sb);
  ASSERT_EQ(strcmp(result, "2 + 3 = 5"), 0);

  x_strbuilder_destroy(sb);
  return 0;
}

int test_strbuilder_append_substring()
{
  XStrBuilder* sb = x_strbuilder_create();
  ASSERT_TRUE(sb != NULL);

  const char* text = "substring test";
  x_strbuilder_append_substring(sb, text, 9);  // "substring"

  const char* result = x_strbuilder_to_string(sb);
  ASSERT_EQ(strcmp(result, "substring"), 0);

  x_strbuilder_destroy(sb);
  return 0;
}

int test_strbuilder_clear_and_length()
{
  XStrBuilder* sb = x_strbuilder_create();
  ASSERT_TRUE(sb != NULL);

  x_strbuilder_append(sb, "temp");
  ASSERT_EQ(x_strbuilder_length(sb), 4);

  x_strbuilder_clear(sb);
  ASSERT_EQ(x_strbuilder_length(sb), 0);

  const char* result = x_strbuilder_to_string(sb);
  ASSERT_EQ(strcmp(result, ""), 0);

  x_strbuilder_destroy(sb);
  return 0;
}

int test_strbuilder_append_utf8_substring_middle(void)
{
  XStrBuilder* sb = x_strbuilder_create();
  const char* text = "🙂😇😎👍"; // 4 emoji, each 4 bytes
                                 // Extract "😇😎" (start at codepoint 1, len 2)
  x_strbuilder_append_utf8_substring(sb, text, 1, 2);
  ASSERT_TRUE(strcmp(x_strbuilder_to_string(sb), "😇😎") == 0);
  x_strbuilder_destroy(sb);
  return 0;
}

int test_strbuilder_utf8_charlen_emoji(void)
{
  XStrBuilder* sb = x_strbuilder_create();
  x_strbuilder_append(sb, "🚀🌕🌍");
  ASSERT_TRUE(x_strbuilder_utf8_charlen(sb) == 3);
  x_strbuilder_destroy(sb);
  return 0;
}

int test_x_wstrbuilder_basic(void)
{
  XWStrBuilder* sb = x_wstrbuilder_create();
  x_wstrbuilder_append(sb, L"שלום");
  x_wstrbuilder_append_char(sb, L' ');
  x_wstrbuilder_append(sb, L"עולם");
  ASSERT_TRUE(wcscmp(x_wstrbuilder_to_string(sb), L"שלום עולם") == 0);
  ASSERT_TRUE(x_wstrbuilder_length(sb) == wcslen(L"שלום עולם"));
  x_wstrbuilder_destroy(sb);
  return 0;
}

int test_x_wstrbuilder_format(void)
{
  XWStrBuilder* sb = x_wstrbuilder_create();
  x_wstrbuilder_append_format(sb, L"%ls %d", L"תוצאה:", 42);
  ASSERT_TRUE(wcsstr(x_wstrbuilder_to_string(sb), L"תוצאה: 42") != NULL);
  x_wstrbuilder_destroy(sb);
  return 0;
}

int main()
{
  STDXTestCase tests[] = {
    X_TEST(test_strbuilder_append_and_to_string),
    X_TEST(test_strbuilder_append_char),
    X_TEST(test_strbuilder_append_format),
    X_TEST(test_strbuilder_append_substring),
    X_TEST(test_strbuilder_clear_and_length),
    X_TEST(test_strbuilder_append_utf8_substring_middle),
    X_TEST(test_strbuilder_utf8_charlen_emoji),
    X_TEST(test_x_wstrbuilder_format),
    X_TEST(test_x_wstrbuilder_basic),
  };

  return stdx_run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
