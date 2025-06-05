#include "stdx_common.h"
#include <stdint.h>
#include <wchar.h>
#define STDX_IMPLEMENTATION_TEST
#include <stdx_test.h>
#define STDX_IMPLEMENTATION_STRING
#include <stdx_string.h>
#include <stdx_log.h>

int test_str_starts_with(void)
{
  ASSERT_TRUE(x_cstr_starts_with("hello world", "hello"));
  ASSERT_FALSE(x_cstr_starts_with("hello world", "world"));
  ASSERT_TRUE(x_cstr_starts_with("", ""));
  ASSERT_FALSE(x_cstr_starts_with("abc", ""));
  return STDX_TEST_SUCCESS;
}

/**
 * Does the normal ascii function handle
 * שָׁלוֹם;  utf8 in the comments ?
 *
 */
int test_str_ends_with(void)
{
  ASSERT_TRUE(x_cstr_ends_with("hello world", "world"));
  ASSERT_FALSE(x_cstr_ends_with("hello world", "hello"));
  ASSERT_TRUE(x_cstr_ends_with("", ""));
  ASSERT_FALSE(x_cstr_ends_with("abc", ""));
  return STDX_TEST_SUCCESS;
}

int test_x_smallstr_basic(void)
{
  XSmallstr s;
  size_t len = smallstr(&s, "test123");
  ASSERT_EQ(len, 7);
  ASSERT_EQ(x_smallstr_length(&s), 7);
  return STDX_TEST_SUCCESS;
}

int test_x_smallstr_truncation(void)
{
  XSmallstr s;
  char long_str[1024] = {0};
  memset(long_str, 'a', sizeof(long_str) - 1);
  long_str[sizeof(long_str) - 1] = '\0';
  size_t len = smallstr(&s, long_str);
  ASSERT_TRUE(len <= STDX_SMALLSTR_MAX_LENGTH);
  ASSERT_EQ(x_smallstr_length(&s), len);
  return STDX_TEST_SUCCESS;
}

int test_x_smallstr_format(void)
{
  XSmallstr s;
  size_t len = x_smallstr_format(&s, "val: %d", 42);
  ASSERT_TRUE(strncmp(x_smallstr_cstr(&s), "val: 42", 7) == 0);
  ASSERT_TRUE(len > 0);
  ASSERT_TRUE(x_cstr_starts_with(s.buf, "val: 42"));
  return STDX_TEST_SUCCESS;
}

int test_x_smallstr_clear(void)
{
  XSmallstr s;
  smallstr(&s, "clear me");
  x_smallstr_clear(&s);
  ASSERT_EQ(x_smallstr_length(&s), 0);
  return STDX_TEST_SUCCESS;
}

int test_str_hash(void)
{
  uint32_t hash1 = x_cstr_hash("test");
  uint32_t hash2 = x_cstr_hash("test");
  uint32_t hash3 = x_cstr_hash("different");
  ASSERT_EQ(hash1, hash2);
  ASSERT_NEQ(hash1, hash3);
  return STDX_TEST_SUCCESS;
}

int test_x_strview_empty(void)
{
  ASSERT_TRUE(x_strview_empty(x_strview("")));
  ASSERT_FALSE(x_strview_empty(x_strview("a")));
  return 0;
}

int test_x_strview_eq_and_cmp(void)
{
  XStrview a = x_strview("hello");
  XStrview b = x_strview("hello");
  XStrview c = x_strview("world");

  ASSERT_TRUE(x_strview_eq(a, b));
  ASSERT_FALSE(x_strview_eq(a, c));
  ASSERT_TRUE(x_strview_cmp(a, b) == 0);
  ASSERT_TRUE(x_strview_cmp(a, c) < 0);
  ASSERT_TRUE(x_strview_cmp(c, a) > 0);
  return 0;
}

int test_x_strview_case_eq_and_cmp(void)
{
  ASSERT_TRUE(x_strview_eq_case(x_strview("HELLO"), x_strview("hello")));
  ASSERT_TRUE(x_strview_cmp_case(x_strview("HELLO"), x_strview("hello"))  == 0);
  ASSERT_TRUE(x_strview_cmp_case(x_strview("abc"), x_strview("DEF"))      < 0);
  return 0;
}

int test_x_strview_substr(void)
{
  XStrview sv = x_strview("abcdef");
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_substr(sv, 0, 3), "abc"));
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_substr(sv, 2, 2), "cd"));
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_substr(sv, 4, 10), "ef")); // len clipped
  return 0;
}

int test_x_strview_trim(void)
{
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_trim_left(x_strview("   abc")), "abc"));
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_trim_right(x_strview("abc   ")), "abc"));
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_trim(x_strview("   abc   ")), "abc"));
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_trim(x_strview("abc")), "abc"));
  ASSERT_TRUE(x_strview_eq_cstr(x_strview_trim(x_strview("   ")), ""));
  return 0;
}

int test_x_strview_find_and_rfind(void)
{
  XStrview sv = x_strview("abacada");

  ASSERT_TRUE(x_strview_find(sv, 'a') == 0);
  ASSERT_TRUE(x_strview_find(sv, 'c') == 3);
  ASSERT_TRUE(x_strview_find(sv, 'x') == -1);

  ASSERT_TRUE(x_strview_rfind(sv, 'a') == 6);
  ASSERT_TRUE(x_strview_rfind(sv, 'b') == 1);
  ASSERT_TRUE(x_strview_rfind(sv, 'x') == -1);
  return 0;
}

int test_x_strview_split_at(void)
{

  { // Basic split test
    XStrview sv = x_strview("key:value");
    XStrview left, right;
    ASSERT_TRUE(x_strview_split_at(sv, ':', &left, &right));
    ASSERT_TRUE(x_strview_eq_cstr(left, "key"));
    ASSERT_TRUE(x_strview_eq_cstr(right, "value"));
  }

  { // Test advancing the stringview by tokens
    const char* results[] =
    { "wako", "yako", "dotty" };
    XStrview csv = x_strview("wako,yako,dotty");
    XStrview token;
    int i = 0;
    while (x_strview_next_token(&csv, ',', &token))
    {
      ASSERT_TRUE(x_strview_eq(x_strview(results[i]), token) == 1);
      i++;
    }
  }

  { // Test splitting by non existent separator
    XStrview sv = x_strview("novalue");
    XStrview left, right;
    ASSERT_FALSE(x_strview_split_at(sv, ':', &left, &right));
  }
  return 0;
}

int test_xwsmallstr_tokenize(void)
{
  //XWSmallstr s;
  //x_wsmallstr_from_wcstr(&s, L"אחד,שתיים,שלוש");
  //XWSmallstr token;
  //int count = 0;

  //while(x_wsallstr_next_token(&iter, &s, L','))
  //{
  //  ASSERT_TRUE(token.length > 0);
  //}
  //ASSERT_TRUE(count == 3);
  return 0;
}

int test_cstr_wcstr_conversions(void)
{
  const char* ascii = "Café UTF-8";
  wchar_t wide[128];
  char back[128];

  size_t wlen = x_cstr_to_wcstr(ascii, wide, 128);
  size_t clen = x_wcstr_to_cstr(wide, back, 128);

  ASSERT_TRUE(wlen > 0);
  ASSERT_TRUE(clen > 0);
  ASSERT_TRUE(strcmp(ascii, back) == 0);
  return 0;
}

int test_wcstr_starts_ends_with(void)
{
  const wchar_t* text = L"HelloWorld";
  ASSERT_TRUE(x_wcstr_starts_with(text, L"Hello"));
  ASSERT_TRUE(x_wcstr_ends_with(text, L"World"));
  ASSERT_TRUE(!x_wcstr_starts_with(text, L"world"));
  ASSERT_TRUE(!x_wcstr_ends_with(text, L"Hello"));
  return 0;
}

int test_wcstr_starts_ends_with_ci(void)
{
  const wchar_t* text = L"HelloWorld";
  ASSERT_TRUE(x_wcstr_starts_with_case(text, L"hello"));
  ASSERT_TRUE(x_wcstr_ends_with_case(text, L"world"));
  ASSERT_TRUE(!x_wcstr_starts_with_case(text, L"nope"));
  return 0;
}

int test_wcstr_casecmp(void)
{
  ASSERT_TRUE(x_wcstr_casecmp(L"Hello", L"hello") == 0);
  ASSERT_TRUE(x_wcstr_casecmp(L"ABC", L"abc") == 0);
  ASSERT_TRUE(x_wcstr_casecmp(L"Hello", L"HELLO!") != 0);
  return 0;
}

int test_chinese_wcstr(void)
{
  const wchar_t* greeting = L"你好世界"; // "Hello world"
  ASSERT_TRUE(x_wcstr_starts_with(greeting, L"你"));   // starts with "you"
  ASSERT_TRUE(x_wcstr_ends_with(greeting, L"世界"));   // ends with "world"
  ASSERT_FALSE(x_wcstr_casecmp(greeting, L"你好世界")); // No case in chinese
  return 0;
}

int test_hebrew_wcstr(void)
{
  const wchar_t* word = L"שָׁלוֹם";  // "Shalom" with niqqud (diacritics)
  const wchar_t* plain = L"שלום"; // plain "Shalom"

  // Will NOT match with x_wcstr_casecmp due to Unicode combining chars
  ASSERT_TRUE(x_wcstr_casecmp(word, word) == 0);
  ASSERT_TRUE(x_wcstr_casecmp(plain, plain) == 0);
  ASSERT_TRUE(x_wcstr_casecmp(word, plain) != 0); // Fails due to combining chars
  return 0;
}

int test_arabic_wcstr(void)
{
  const wchar_t* phrase = L"السلام عليكم";
  ASSERT_TRUE(x_wcstr_starts_with(phrase, L"السلام"));
  ASSERT_TRUE(x_wcstr_ends_with(phrase, L"عليكم"));
  return 0;
}

int test_conversion_utf8_and_back(void)
{
  const char* utf8 = "שלום"; // UTF-8 Hebrew word
  wchar_t wbuf[128];
  char back[128];

  size_t wlen = x_cstr_to_wcstr(utf8, wbuf, 128);
  size_t clen = x_wcstr_to_cstr(wbuf, back, 128);

  ASSERT_TRUE(wlen > 0 && clen > 0);
  ASSERT_TRUE(strcmp(utf8, back) == 0); // round-trip must match
  return 0;
}

int test_xwstrview_basic(void)
{
  XWStrview sv = x_wstrview(L" שלום ");
  ASSERT_TRUE(!x_wstrview_empty(sv));
  ASSERT_TRUE(x_wstrview_eq(sv, x_wstrview(L" שלום ")));
  ASSERT_TRUE(x_wstrview_cmp(sv, x_wstrview(L" עולם ")) != 0);
  return 0;
}

int test_xwstrview_trim(void)
{
  XWStrview sv = x_wstrview(L" \t\nשלום\n ");
  XWStrview trimmed = x_wstrview_trim(sv);
  ASSERT_TRUE(x_wstrview_eq(trimmed, x_wstrview(L"שלום")));
  return 0;
}

int test_xwstrview_substr(void)
{
  XWStrview full = x_wstrview(L"שלוםעולם");
  XWStrview mid = x_wstrview_substr(full, 2, 2);
  ASSERT_TRUE(mid.length == 2);
  ASSERT_TRUE(mid.data[0] == L'ו' && mid.data[1] == L'ם');
  return 0;
}

int test_wsmallstr_functions(void)
{
  XWSmallstr ws;
  x_wsmallstr_from_wcstr(&ws, L"  שלום  ");
  x_wsmallstr_trim(&ws);
  size_t len = wcslen(ws.buf);
  ASSERT_TRUE(x_wsmallstr_len(&ws) == len);
  ASSERT_TRUE(x_wsmallstr_len(&ws) == 4);
  ASSERT_TRUE(wcslen(ws.buf) == 4);
  ASSERT_TRUE(x_wsmallstr_cmp_cstr(&ws, L"שלום") == 0);
  return 0;
}

int test_x_strview_utf8_find_cp()
{
  const char* data = "🌍";
  const char* ptr = data;
  const char* end = data + strlen(data);
  size_t len = 0;

  uint32_t cp = x_utf8_decode(ptr, end, &len);
  ASSERT_TRUE(cp == 0x1F30D);
  ASSERT_TRUE(len == x_utf8_strlen(data));

  XStrview sv = x_strview("a🌍b🌍c");
  ASSERT_TRUE(x_utf8_strlen(sv.data) == 5);            // 5 UTf-8 characters
  ASSERT_TRUE(x_strview_utf8_find(sv, 0x1F30D) == 1);  // first 🌍
  ASSERT_TRUE(x_strview_utf8_rfind(sv, 0x1F30D) == 6); // second 🌍
  ASSERT_TRUE(x_strview_utf8_find(sv, 'b') == 5);      // ASCII still works via its codepoint
  ASSERT_TRUE(x_strview_utf8_find(sv, 'z') == -1);     // not found
  return 0;
}

int test_x_strview_utf8_rfind()
{
  XStrview sv = x_strview("hélllo");
  ASSERT_TRUE(x_strview_utf8_rfind(sv, 'l') == 5);
  ASSERT_TRUE(x_strview_utf8_rfind(sv, 'x') == -1);
  return 0;
}

int test_x_strview_utf8_split_at()
{
  {
    XStrview sv = x_strview("a✓b✓c");
    XStrview left, right;

    ASSERT_TRUE(x_strview_utf8_split_at(sv, 0x2713, &left, &right));
    ASSERT_TRUE(x_strview_eq_cstr(left, "a"));
    ASSERT_TRUE(x_strview_eq_cstr(right, "b✓c"));
  }

  {
    XStrview sv = x_strview("a✓b✓c");
    XStrview tok;
    const char* expected[] = {"a", "b", "c"};
    const char** e = expected;
    while (x_strview_utf8_next_token(&sv, 0x2713, &tok))
    {
      // expect "a", "b", "c"
      ASSERT_TRUE(x_strview_eq_cstr(tok, *e++));

    }
  }

  {
    XStrview sv = x_strview("héllo,world");
    XStrview left, right;
    bool ok = x_strview_utf8_split_at(sv, ',', &left, &right);

    ASSERT_TRUE(ok);
    ASSERT_TRUE(x_strview_eq_cstr(left, "héllo"));
    ASSERT_TRUE(x_strview_eq_cstr(right, "world"));
  }

  return 0;
}

int test_x_smallstr_utf8_tokenizer()
{
  //  XSmallstr s;
  //  x_smallstr_from_cstr(&s, "a✓b✓c");
  //  XSmallstr tok;
  //
  //  const char* expected[] = {"a", "b", "c"};
  //  const char** e = expected;
  //  //while (x_smallstr_utf8_token_iter_next(&iter, &tok))
  //  while(x_smallstr_next_token(&s, 0x2713, &tok))
  //  {
  //    ASSERT_TRUE(x_strview_eq_cstr(x_strview_init(tok.buf, tok.length), *e++));
  //  }
  //
  return 0;
}

int test_x_strview_utf8_next_token()
{
  XStrview input = x_strview("uno,dos,tres");
  XStrview token;

  ASSERT_TRUE(x_strview_utf8_next_token(&input, ',', &token));
  ASSERT_TRUE(x_strview_eq(token, x_strview("uno")));
  ASSERT_TRUE(x_strview_utf8_next_token(&input, ',', &token));
  ASSERT_TRUE(x_strview_eq(token, x_strview("dos")));
  ASSERT_TRUE(x_strview_utf8_next_token(&input, ',', &token));
  ASSERT_TRUE(x_strview_eq(token, x_strview("tres")));
  ASSERT_TRUE(!x_strview_utf8_next_token(&input, ',', &token));
  return 0;
}

int test_x_strview_utf8_starts_with_cstr()
{
  XStrview sv = x_strview("héllo 🌍");
  ASSERT_TRUE(x_strview_utf8_starts_with_cstr(sv, "hé"));
  ASSERT_TRUE(!x_strview_utf8_starts_with_cstr(sv, "🌍"));
  return 0;
}

int test_x_strview_utf8_ends_with_cstr()
{
  XStrview sv = x_strview("héllo 🌍");
  ASSERT_TRUE(x_strview_utf8_ends_with_cstr(sv, "🌍"));
  ASSERT_TRUE(x_strview_utf8_ends_with_cstr(sv, "o 🌍"));
  ASSERT_TRUE(!x_strview_utf8_ends_with_cstr(sv, "héllo"));
  return 0;
}

int main()
{

  x_utf8_set_locale();

  STDXTestCase tests[] =
  {
    STDX_TEST(test_x_strview_utf8_ends_with_cstr),
    STDX_TEST(test_x_strview_utf8_starts_with_cstr),
    STDX_TEST(test_x_strview_utf8_next_token),
    STDX_TEST(test_x_strview_utf8_find_cp),
    STDX_TEST(test_x_strview_utf8_rfind),
    STDX_TEST(test_x_strview_utf8_split_at),
    STDX_TEST(test_x_smallstr_utf8_tokenizer),

    STDX_TEST(test_wsmallstr_functions),
    STDX_TEST(test_xwstrview_basic),
    STDX_TEST(test_xwstrview_trim),
    STDX_TEST(test_xwstrview_substr),

    STDX_TEST(test_chinese_wcstr),
    STDX_TEST(test_arabic_wcstr),
    STDX_TEST(test_conversion_utf8_and_back),

    STDX_TEST(test_cstr_wcstr_conversions),
    STDX_TEST(test_wcstr_starts_ends_with),
    STDX_TEST(test_wcstr_casecmp),
    STDX_TEST(test_wcstr_starts_ends_with_ci),

    STDX_TEST(test_str_starts_with),
    STDX_TEST(test_str_ends_with),
    STDX_TEST(test_x_smallstr_basic),
    STDX_TEST(test_x_smallstr_truncation),
    STDX_TEST(test_x_smallstr_format),
    STDX_TEST(test_x_smallstr_clear),
    STDX_TEST(test_str_hash),

    STDX_TEST(test_x_strview_empty),
    STDX_TEST(test_x_strview_eq_and_cmp),
    STDX_TEST(test_x_strview_case_eq_and_cmp),
    STDX_TEST(test_x_strview_substr),
    STDX_TEST(test_x_strview_trim),
    STDX_TEST(test_x_strview_find_and_rfind),
    STDX_TEST(test_x_strview_split_at),
    STDX_TEST(test_xwsmallstr_tokenize)

  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}
