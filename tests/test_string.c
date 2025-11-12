#include <stdx_common.h>
#include <stdint.h>
#include <wchar.h>
#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_STRING
#include <stdx_string.h>
#include <stdx_log.h>

// Helpers
static int sv_eq_cstr(XSlice sv, const char* s)
{
  size_t n = s ? strlen(s) : 0u;
  return sv.length == n && (n == 0 || memcmp(sv.data, s, n) == 0);
}

static size_t vappend_proxy(XSmallstr* s, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  size_t r = x_smallstr_vappendf(s, fmt, args);
  va_end(args);
  return r;
}

// Tests

int test_str_starts_with(void)
{
  ASSERT_TRUE(x_cstr_starts_with("hello world", "hello"));
  ASSERT_FALSE(x_cstr_starts_with("hello world", "world"));
  ASSERT_TRUE(x_cstr_starts_with("", ""));
  ASSERT_FALSE(x_cstr_starts_with("abc", ""));
  return X_TEST_SUCCESS;
}

int test_str_ends_with(void)
{
  ASSERT_TRUE(x_cstr_ends_with("hello world", "world"));
  ASSERT_FALSE(x_cstr_ends_with("hello world", "hello"));
  ASSERT_TRUE(x_cstr_ends_with("", ""));
  ASSERT_FALSE(x_cstr_ends_with("abc", ""));
  return X_TEST_SUCCESS;
}

int test_x_smallstr_basic(void)
{
  XSmallstr s;
  size_t len = smallstr(&s, "test123");
  ASSERT_EQ(len, 7);
  ASSERT_EQ(x_smallstr_length(&s), 7);
  return X_TEST_SUCCESS;
}

int test_x_smallstr_truncation(void)
{
  XSmallstr s;
  char long_str[1024] = {0};
  memset(long_str, 'a', sizeof(long_str) - 1);
  long_str[sizeof(long_str) - 1] = '\0';
  size_t len = smallstr(&s, long_str);
  ASSERT_TRUE(len <= X_SMALLSTR_MAX_LENGTH);
  ASSERT_EQ(x_smallstr_length(&s), len);
  return X_TEST_SUCCESS;
}

int test_x_smallstr_format(void)
{
  XSmallstr s;
  size_t len = x_smallstr_format(&s, "val: %d", 42);
  ASSERT_TRUE(strncmp(x_smallstr_cstr(&s), "val: 42", 7) == 0);
  ASSERT_TRUE(len > 0);
  ASSERT_TRUE(x_cstr_starts_with(s.buf, "val: 42"));
  return X_TEST_SUCCESS;
}

int test_x_smallstr_clear(void)
{
  XSmallstr s;
  smallstr(&s, "clear me");
  x_smallstr_clear(&s);
  ASSERT_EQ(x_smallstr_length(&s), 0);
  return X_TEST_SUCCESS;
}

int test_str_hash(void)
{
  uint32_t hash1 = x_cstr_hash("test");
  uint32_t hash2 = x_cstr_hash("test");
  uint32_t hash3 = x_cstr_hash("different");
  ASSERT_EQ(hash1, hash2);
  ASSERT_NEQ(hash1, hash3);
  return X_TEST_SUCCESS;
}

int test_x_slice_empty(void)
{
  ASSERT_TRUE(x_slice_is_empty(x_slice("")));
  ASSERT_FALSE(x_slice_is_empty(x_slice("a")));
  return 0;
}

int test_x_slice_eq_and_cmp(void)
{
  XSlice a = x_slice("hello");
  XSlice b = x_slice("hello");
  XSlice c = x_slice("world");

  ASSERT_TRUE(x_slice_eq(a, b));
  ASSERT_FALSE(x_slice_eq(a, c));
  ASSERT_TRUE(x_slice_cmp(a, b) == 0);
  ASSERT_TRUE(x_slice_cmp(a, c) < 0);
  ASSERT_TRUE(x_slice_cmp(c, a) > 0);
  return 0;
}

int test_x_slice_ci_eq_and_cmp(void)
{
  ASSERT_TRUE(x_slice_eq_ci(x_slice("HELLO"), x_slice("hello")));
  ASSERT_TRUE(x_slice_cmp_ci(x_slice("HELLO"), x_slice("hello"))  == 0);
  ASSERT_TRUE(x_slice_cmp_ci(x_slice("abc"), x_slice("DEF"))      < 0);
  return 0;
}

int test_x_slice_substr(void)
{
  XSlice sv = x_slice("abcdef");
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_substr(sv, 0, 3), "abc"));
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_substr(sv, 2, 2), "cd"));
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_substr(sv, 4, 10), "ef")); // len clipped
  return 0;
}

int test_x_slice_trim(void)
{
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_trim_left(x_slice("   abc")), "abc"));
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_trim_right(x_slice("abc   ")), "abc"));
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_trim(x_slice("   abc   ")), "abc"));
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_trim(x_slice("abc")), "abc"));
  ASSERT_TRUE(x_slice_eq_cstr(x_slice_trim(x_slice("   ")), ""));
  return 0;
}

int test_x_slice_find_and_rfind(void)
{
  XSlice sv = x_slice("abacada");

  ASSERT_TRUE(x_slice_find(sv, 'a') == 0);
  ASSERT_TRUE(x_slice_find(sv, 'c') == 3);
  ASSERT_TRUE(x_slice_find(sv, 'x') == -1);

  ASSERT_TRUE(x_slice_rfind(sv, 'a') == 6);
  ASSERT_TRUE(x_slice_rfind(sv, 'b') == 1);
  ASSERT_TRUE(x_slice_rfind(sv, 'x') == -1);
  return 0;
}

int test_x_slice_split_at(void)
{

  { // Basic split test
    XSlice sv = x_slice("key:value");
    XSlice left, right;
    ASSERT_TRUE(x_slice_split_at(sv, ':', &left, &right));
    ASSERT_TRUE(x_slice_eq_cstr(left, "key"));
    ASSERT_TRUE(x_slice_eq_cstr(right, "value"));
  }

  { // Test advancing the stringview by tokens
    const char* results[] =
    { "wako", "yako", "dotty" };
    XSlice csv = x_slice("wako,yako,dotty");
    XSlice token;
    int i = 0;
    while (x_slice_next_token(&csv, ',', &token))
    {
      ASSERT_TRUE(x_slice_eq(x_slice(results[i]), token) == 1);
      i++;
    }
  }

  { // Test splitting by non existent separator
    XSlice sv = x_slice("novalue");
    XSlice left, right;
    ASSERT_FALSE(x_slice_split_at(sv, ':', &left, &right));
  }
  return 0;
}

int test_x_wslice_next_token(void)
{
  {
    x_set_locale("Hebrew");
    XWStrview s = x_wslice( L"אחד,שתיים,שלוש");
    XWStrview token;
    int count = 0;

    while(x_wslice_next_token(&s, L',', &token))
    {
      ASSERT_TRUE(token.length > 0);
      count++;
    }
    ASSERT_TRUE(count == 3);
  }

  {
    x_set_locale(NULL);
    XWStrview input = x_wslice( L"foo,bar,baz");
    XWStrview token;
    int count = 0;

    while (x_wslice_next_token(&input, L',', &token))
    {
      count++;
    }
    ASSERT_TRUE(count == 3);
  }

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
  ASSERT_TRUE(x_wcstr_starts_with_ci(text, L"hello"));
  ASSERT_TRUE(x_wcstr_ends_with_ci(text, L"world"));
  ASSERT_TRUE(!x_wcstr_starts_with_ci(text, L"nope"));
  return 0;
}

int test_wcstr_cicmp(void)
{
  ASSERT_TRUE(x_wcstr_cicmp(L"Hello", L"hello") == 0);
  ASSERT_TRUE(x_wcstr_cicmp(L"ABC", L"abc") == 0);
  ASSERT_TRUE(x_wcstr_cicmp(L"Hello", L"HELLO!") != 0);
  return 0;
}

int test_chinese_wcstr(void)
{
  const wchar_t* greeting = L"你好世界"; // "Hello world"
  ASSERT_TRUE(x_wcstr_starts_with(greeting, L"你"));   // starts with "you"
  ASSERT_TRUE(x_wcstr_ends_with(greeting, L"世界"));   // ends with "world"
  ASSERT_FALSE(x_wcstr_cicmp(greeting, L"你好世界")); // No case in chinese
  return 0;
}

int test_hebrew_wcstr(void)
{
  const wchar_t* word = L"שָׁלוֹם";  // "Shalom" with niqqud (diacritics)
  const wchar_t* plain = L"שלום"; // plain "Shalom"

  // Will NOT match with x_wcstr_cicmp due to Unicode combining chars
  ASSERT_TRUE(x_wcstr_cicmp(word, word) == 0);
  ASSERT_TRUE(x_wcstr_cicmp(plain, plain) == 0);
  ASSERT_TRUE(x_wcstr_cicmp(word, plain) != 0); // Fails due to combining chars
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

int test_xwslice_basic(void)
{
  XWStrview sv = x_wslice(L" שלום ");
  ASSERT_TRUE(!x_wslice_empty(sv));
  ASSERT_TRUE(x_wslice_eq(sv, x_wslice(L" שלום ")));
  ASSERT_TRUE(x_wslice_cmp(sv, x_wslice(L" עולם ")) != 0);
  return 0;
}

int test_xwslice_trim(void)
{
  XWStrview sv = x_wslice(L" \t\nשלום\n ");
  XWStrview trimmed = x_wslice_trim(sv);
  ASSERT_TRUE(x_wslice_eq(trimmed, x_wslice(L"שלום")));
  return 0;
}

int test_xwslice_substr(void)
{
  XWStrview full = x_wslice(L"שלוםעולם");
  XWStrview mid = x_wslice_substr(full, 2, 2);
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
  ASSERT_TRUE(x_wsmallstr_length(&ws) == len);
  ASSERT_TRUE(x_wsmallstr_length(&ws) == 4);
  ASSERT_TRUE(wcslen(ws.buf) == 4);
  ASSERT_TRUE(x_wsmallstr_cmp_cstr(&ws, L"שלום") == 0);
  return 0;
}

int test_x_slice_utf8_find_cp()
{
  const char* data = "🌍";
  const char* ptr = data;
  const char* end = data + strlen(data);
  size_t len = 0;

  uint32_t cp = x_utf8_decode(ptr, end, &len);
  ASSERT_TRUE(cp == 0x1F30D);
  ASSERT_TRUE(len == x_utf8_strlen(data));

  XSlice sv = x_slice("a🌍b🌍c");
  ASSERT_TRUE(x_utf8_strlen(sv.data) == 5);            // 5 UTf-8 characters
  ASSERT_TRUE(x_slice_utf8_find(sv, 0x1F30D) == 1);  // first 🌍
  ASSERT_TRUE(x_slice_utf8_rfind(sv, 0x1F30D) == 6); // second 🌍
  ASSERT_TRUE(x_slice_utf8_find(sv, 'b') == 5);      // ASCII still works via its codepoint
  ASSERT_TRUE(x_slice_utf8_find(sv, 'z') == -1);     // not found
  return 0;
}

int test_x_slice_utf8_rfind()
{
  XSlice sv = x_slice("hélllo");
  ASSERT_TRUE(x_slice_utf8_rfind(sv, 'l') == 5);
  ASSERT_TRUE(x_slice_utf8_rfind(sv, 'x') == -1);
  return 0;
}

int test_x_slice_utf8_split_at()
{
  {
    XSlice sv = x_slice("a✓b✓c");
    XSlice left, right;

    ASSERT_TRUE(x_slice_utf8_split_at(sv, 0x2713, &left, &right));
    ASSERT_TRUE(x_slice_eq_cstr(left, "a"));
    ASSERT_TRUE(x_slice_eq_cstr(right, "b✓c"));
  }

  {
    XSlice sv = x_slice("a✓b✓c");
    XSlice tok;
    const char* expected[] = {"a", "b", "c"};
    const char** e = expected;
    while (x_slice_utf8_next_token(&sv, 0x2713, &tok))
    {
      // expect "a", "b", "c"
      ASSERT_TRUE(x_slice_eq_cstr(tok, *e++));

    }
  }

  {
    XSlice sv = x_slice("héllo,world");
    XSlice left, right;
    bool ok = x_slice_utf8_split_at(sv, ',', &left, &right);

    ASSERT_TRUE(ok);
    ASSERT_TRUE(x_slice_eq_cstr(left, "héllo"));
    ASSERT_TRUE(x_slice_eq_cstr(right, "world"));
  }

  return 0;
}

int test_x_slice_utf8_next_token()
{
  XSlice input = x_slice("uno,dos,tres");
  XSlice token;

  ASSERT_TRUE(x_slice_utf8_next_token(&input, ',', &token));
  ASSERT_TRUE(x_slice_eq(token, x_slice("uno")));
  ASSERT_TRUE(x_slice_utf8_next_token(&input, ',', &token));
  ASSERT_TRUE(x_slice_eq(token, x_slice("dos")));
  ASSERT_TRUE(x_slice_utf8_next_token(&input, ',', &token));
  ASSERT_TRUE(x_slice_eq(token, x_slice("tres")));
  ASSERT_TRUE(!x_slice_utf8_next_token(&input, ',', &token));
  return 0;
}

int test_x_slice_utf8_starts_with_cstr()
{
  XSlice sv = x_slice("héllo 🌍");
  ASSERT_TRUE(x_slice_utf8_starts_with_cstr(sv, "hé"));
  ASSERT_TRUE(!x_slice_utf8_starts_with_cstr(sv, "🌍"));
  return 0;
}

int test_x_slice_utf8_ends_with_cstr()
{
  XSlice sv = x_slice("héllo 🌍");
  ASSERT_TRUE(x_slice_utf8_ends_with_cstr(sv, "🌍"));
  ASSERT_TRUE(x_slice_utf8_ends_with_cstr(sv, "o 🌍"));
  ASSERT_TRUE(!x_slice_utf8_ends_with_cstr(sv, "héllo"));
  return 0;
}

int test_x_slice_from_cstr_basic()
{
  XSlice a = x_slice_from_cstr("hello");
  ASSERT_TRUE(sv_eq_cstr(a, "hello"));
  ASSERT_TRUE(a.data[ a.length ] == '\0'); // points into a C string

  XSlice b = x_slice_from_cstr(NULL);
  ASSERT_TRUE(b.length == 0);
  return 0;
}

int test_x_slice_from_smallstr_basic()
{
  XSmallstr s;
  x_smallstr_clear(&s);
  x_smallstr_append_cstr(&s, "abc");

  XSlice v = x_slice_from_smallstr(&s);
  ASSERT_TRUE(v.length == 3);
  ASSERT_TRUE(memcmp(v.data, "abc", 3) == 0);

  XSlice z = x_slice_from_smallstr(NULL);
  ASSERT_TRUE(z.length == 0);
  return 0;
}

int test_x_smallstr_append_slice_normal()
{
  XSmallstr s;
  x_smallstr_clear(&s);
  XSlice a = x_slice("foo");
  XSlice b = x_slice("bar");
  x_smallstr_append_slice(&s, a);
  x_smallstr_append_slice(&s, b);
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&s), "foobar") == 0);
  return 0;
}

int test_x_smallstr_append_n_basic()
{
  XSmallstr s;
  x_smallstr_clear(&s);
  x_smallstr_append_n(&s, "abcdef", 3);
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&s), "abc") == 0);
  // Appending null should not change the smallstr
  x_smallstr_append_n(&s, NULL, 5);
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&s), "abc") == 0);
  return 0;
}

int test_x_smallstr_appendf_basic()
{
  XSmallstr s;
  x_smallstr_clear(&s);

  x_smallstr_appendf(&s, "%d + %d = %d", 2, 3, 5);
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&s), "2 + 3 = 5") == 0);

  vappend_proxy(&s, " %s", "ok");
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&s), "2 + 3 = 5 ok") == 0);
  return 0;
}

int test_x_smallstr_appendf_truncate()
{
  XSmallstr s;
  x_smallstr_clear(&s);

  // Crates a string larger than a SmallStr;
  char big[ X_SMALLSTR_MAX_LENGTH * 2 ];
  memset(big, 'A', sizeof(big));
  big[ sizeof(big) - 1 ] = '\0';

  x_smallstr_appendf(&s, "%s", big);
  ASSERT_TRUE(s.length == X_SMALLSTR_MAX_LENGTH); // null terminator is handled by the function
  ASSERT_TRUE(s.buf[s.length] == '\0');
  return 0;
}

int test_x_slice_contains_char()
{
  XSlice sv = x_slice("hello");
  ASSERT_TRUE(x_slice_contains_char(sv, 'e'));
  ASSERT_TRUE(!x_slice_contains_char(sv, 'z'));
  return 0;
}

int test_x_slice_contains_utf8()
{
  XSlice sv = x_slice("a🌍b");
  ASSERT_TRUE(x_slice_contains_utf8(sv, 0x1F30D));  // EARTH GLOBE EUROPE-AFRICA
  ASSERT_TRUE(!x_slice_contains_utf8(sv, 0x1F600)); // 😀 not present
  return 0;
}

int test_x_smallstr_contains_char()
{
  XSmallstr s;
  x_smallstr_clear(&s);
  x_smallstr_append_cstr(&s, "xyz");
  ASSERT_TRUE(x_smallstr_contains_char(&s, 'y'));
  ASSERT_TRUE(!x_smallstr_contains_char(&s, 'a'));
  return 0;
}

int test_x_smallstr_join_basic()
{
  XSlice parts[3];
  parts[0] = x_slice("red");
  parts[1] = x_slice("green");
  parts[2] = x_slice("blue");

  XSmallstr dst;
  x_smallstr_clear(&dst);

  x_smallstr_join(&dst, parts, 3, x_slice(","));
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&dst), "red,green,blue") == 0);
  return 0;
}

int test_x_smallstr_join_empty_sep_and_zero()
{
  XSmallstr dst;
  x_smallstr_clear(&dst);

  size_t n = x_smallstr_join(&dst, NULL, 0, x_slice(""));
  ASSERT_TRUE(n == 0);
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&dst), "") == 0);
  return 0;
}

int test_x_smallstr_join_truncate()
{
  // make long entries to force truncation
  XSlice parts[4] = {
    x_slice_from_cstr("AAAAA AAAAA AAAAA AAAAA"),
    x_slice_from_cstr("BBBBB BBBBB BBBBB BBBBB"),
    x_slice_from_cstr("CCCCC CCCCC CCCCC CCCCC"),
    x_slice_from_cstr("DDDDD DDDDD DDDDD DDDDD"),
  };

  XSmallstr dst;
  x_smallstr_clear(&dst);
  x_smallstr_join(&dst, parts, 4, x_slice("|"));

  ASSERT_TRUE(dst.length <= X_SMALLSTR_MAX_LENGTH);
  ASSERT_TRUE(dst.buf[ dst.length ] == '\0');
  ASSERT_TRUE(strncmp(x_smallstr_cstr(&dst), "AAAAA", 5) == 0);
  return 0;
}

int test_x_smallstr_capacity_value()
{
  ASSERT_TRUE(x_smallstr_capacity() == X_SMALLSTR_MAX_LENGTH);
  return 0;
}

int test_x_smallstr_is_empty_cases()
{
  XSmallstr a;
  x_smallstr_clear(&a);
  ASSERT_TRUE(x_smallstr_is_empty(&a));

  x_smallstr_append_cstr(&a, "x");
  ASSERT_TRUE(!x_smallstr_is_empty(&a));

  ASSERT_TRUE(x_smallstr_is_empty(NULL));
  return 0;
}

int test_x_smallstr_try_append_cstr_ok_and_count()
{
  XSmallstr s;
  x_smallstr_clear(&s);

  size_t appended = 0;
  bool ok = x_smallstr_try_append_cstr(&s, "hi", &appended);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(appended == 2);
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&s), "hi") == 0);
  return 0;
}

int test_x_smallstr_append_slice_truncate()
{
  XSmallstr s;
  x_smallstr_clear(&s);

  // Fill leaving just 1 free byte for \0
  size_t keep = X_SMALLSTR_MAX_LENGTH - 1; 
  for (size_t i = 0; i < keep; ++i) { s.buf[i] = 'x'; }
  s.length = keep; 
  s.buf[s.length] = '\0';

  XSlice tail = x_slice("TAIL"); // 4 bytes
  size_t prev = s.length;

  size_t before_free = X_SMALLSTR_MAX_LENGTH - prev;
  size_t expected_added = (tail.length <= before_free) ? tail.length : before_free;

  x_smallstr_append_slice(&s, tail);

  ASSERT_TRUE(s.length == prev + expected_added);
  ASSERT_TRUE(s.buf[s.length] == '\0');
  return 0;
}

int test_x_smallstr_try_append_cstr_truncate()
{
  XSmallstr s;
  x_smallstr_clear(&s);

  // Fill until only 5 bytes remain (plus the terminator slot already guaranteed)
  size_t free_bytes = 5;
  size_t fill = (X_SMALLSTR_MAX_LENGTH > free_bytes) ? (X_SMALLSTR_MAX_LENGTH - free_bytes) : 0u;
  for (size_t i = 0; i < fill; ++i) s.buf[i] = 'x';
  s.length = fill;
  s.buf[s.length] = '\0';

  // Append a 10-byte literal so truncation is forced: only 5 can be written 
  const char *src = "ABCDEFGHIJ"; // 10 bytes 
  size_t before = s.length;

  size_t appended = 0;
  bool ok = x_smallstr_try_append_cstr(&s, src, &appended);

  ASSERT_TRUE(ok);                               // partial write counts as success
  ASSERT_TRUE(appended == free_bytes);           // actually appended bytes 
  ASSERT_TRUE(s.length == before + free_bytes);  // grew by exactly what fit 
  ASSERT_TRUE(s.buf[s.length] == '\0');          // invariant: NUL-terminated 
  return 0;
}

int test_x_smallstr_try_append_cstr_null_input()
{
  XSmallstr s;
  x_smallstr_clear(&s);

  size_t appended = 1234;
  bool ok = x_smallstr_try_append_cstr(&s, NULL, &appended);
  ASSERT_TRUE(!ok);
  ASSERT_TRUE(appended == 0);
  ASSERT_TRUE(strcmp(x_smallstr_cstr(&s), "") == 0);
  return 0;
}

int main()
{
  x_set_locale(NULL);

  STDXTestCase tests[] =
  {
    X_TEST(test_x_slice_utf8_ends_with_cstr),
    X_TEST(test_x_slice_utf8_starts_with_cstr),
    X_TEST(test_x_slice_utf8_next_token),
    X_TEST(test_x_slice_utf8_find_cp),
    X_TEST(test_x_slice_utf8_rfind),
    X_TEST(test_x_slice_utf8_split_at),

    X_TEST(test_wsmallstr_functions),
    X_TEST(test_xwslice_basic),
    X_TEST(test_xwslice_trim),
    X_TEST(test_xwslice_substr),

    X_TEST(test_chinese_wcstr),
    X_TEST(test_arabic_wcstr),
    X_TEST(test_conversion_utf8_and_back),

    X_TEST(test_cstr_wcstr_conversions),
    X_TEST(test_wcstr_starts_ends_with),
    X_TEST(test_wcstr_cicmp),
    X_TEST(test_wcstr_starts_ends_with_ci),

    X_TEST(test_str_starts_with),
    X_TEST(test_str_ends_with),
    X_TEST(test_x_smallstr_basic),
    X_TEST(test_x_smallstr_truncation),
    X_TEST(test_x_smallstr_format),
    X_TEST(test_x_smallstr_clear),
    X_TEST(test_str_hash),

    X_TEST(test_x_slice_empty),
    X_TEST(test_x_slice_eq_and_cmp),
    X_TEST(test_x_slice_ci_eq_and_cmp),
    X_TEST(test_x_slice_substr),
    X_TEST(test_x_slice_trim),
    X_TEST(test_x_slice_find_and_rfind),
    X_TEST(test_x_slice_split_at),
    X_TEST(test_x_wslice_next_token),

    X_TEST(test_x_slice_from_cstr_basic),
    X_TEST(test_x_slice_from_smallstr_basic),
    X_TEST(test_x_smallstr_append_slice_normal),
    X_TEST(test_x_smallstr_append_slice_truncate),
    X_TEST(test_x_smallstr_append_n_basic),
    X_TEST(test_x_smallstr_appendf_basic),
    X_TEST(test_x_smallstr_appendf_truncate),
    X_TEST(test_x_slice_contains_char),
    X_TEST(test_x_slice_contains_utf8),
    X_TEST(test_x_smallstr_contains_char),
    X_TEST(test_x_smallstr_join_basic),
    X_TEST(test_x_smallstr_join_empty_sep_and_zero),
    X_TEST(test_x_smallstr_join_truncate),
    X_TEST(test_x_smallstr_capacity_value),
    X_TEST(test_x_smallstr_is_empty_cases),
    X_TEST(test_x_smallstr_try_append_cstr_ok_and_count),
    X_TEST(test_x_smallstr_try_append_cstr_truncate),
    X_TEST(test_x_smallstr_try_append_cstr_null_input)
  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}
