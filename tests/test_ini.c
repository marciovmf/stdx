#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_INI
#include <stdx_ini.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static int s_eq(const char *a, const char *b)
{
  if (!a && !b) return 1;
  if (!a || !b) return 0;
  return strcmp(a, b) == 0;
}


int test_ini_parse_basic()
{
  const char *txt =
    "[server]\n"
    "port = 8080\n"
    "docroot = \"files\"\n"
    "list_dirs = true";

  XIni ini;
  XIniError iniError;
  ASSERT_TRUE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));

  // typed getters
  ASSERT_TRUE(x_ini_get_i32(&ini, "server", "port", 80) == 8080);
  ASSERT_TRUE(s_eq(x_ini_get(&ini, "server", "docroot", NULL), "files"));
  ASSERT_TRUE(x_ini_get_bool(&ini, "server", "list_dirs", false) == true);

  // defaults
  ASSERT_TRUE(x_ini_get(&ini, "server", "missing", "DEF") && s_eq(x_ini_get(&ini, "server", "missing", "DEF"), "DEF"));
  ASSERT_TRUE(x_ini_get_bool(&ini, "server", "missing_bool", true) == true);

  x_ini_free(&ini);
  return 0;
}

int test_ini_global_section_and_lookup()
{
  const char *txt =
    "appname = stdx\n"
    "[db]\n"
    "host = localhost\n";

  XIni ini;
  XIniError iniError;
  ASSERT_TRUE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));

  /* global section is "" */
  ASSERT_TRUE(s_eq(x_ini_get(&ini, "", "appname", NULL), "stdx"));
  ASSERT_TRUE(s_eq(x_ini_get(&ini, "db", "host", NULL), "localhost"));
  ASSERT_TRUE(x_ini_get(&ini, "nope", "key", "fallback") && s_eq(x_ini_get(&ini, "nope", "key", "fallback"), "fallback"));

  x_ini_free(&ini);
  return 0;
}

int test_ini_last_definition_wins()
{
  const char *txt =
    "[opt]\n"
    "level = 1\n"
    "level = 2\n"
    "level = 3\n";

  XIni ini;
  XIniError iniError;
  ASSERT_TRUE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));

  ASSERT_TRUE(s_eq(x_ini_get(&ini, "opt", "level", NULL), "3"));
  ASSERT_TRUE(x_ini_get_i32(&ini, "opt", "level", 0) == 3);

  x_ini_free(&ini);
  return 0;
}

int test_ini_inline_comments_and_whitespace()
{
  const char *txt =
    "[paths]\n"
    "dir = /tmp ; trailing comment\n"
    "log = /var/log  # hash comment\n"
    "quote = \"keep;#inside\"  ; outer comment only\n";

  XIni ini;
  XIniError iniError;
  ASSERT_TRUE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));

  ASSERT_TRUE(s_eq(x_ini_get(&ini, "paths", "dir", NULL), "/tmp"));
  ASSERT_TRUE(s_eq(x_ini_get(&ini, "paths", "log", NULL), "/var/log"));
  ASSERT_TRUE(s_eq(x_ini_get(&ini, "paths", "quote", NULL), "keep;#inside"));

  x_ini_free(&ini);
  return 0;
}

int test_ini_bool_variants()
{
  const char *txt =
    "[b]\n"
    "t1 = true\n"
    "t2 = yes\n"
    "t3 = on\n"
    "t4 = 1\n"
    "f1 = false\n"
    "f2 = no\n"
    "f3 = off\n"
    "f4 = 0\n";

  XIni ini;
  XIniError iniError;
  ASSERT_TRUE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));

  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "t1", false) == true);
  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "t2", false) == true);
  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "t3", false) == true);
  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "t4", false) == true);

  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "f1", true) == false);
  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "f2", true) == false);
  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "f3", true) == false);
  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "f4", true) == false);

  // unknown -> default
  ASSERT_TRUE(x_ini_get_bool(&ini, "b", "unknown", true) == true);

  x_ini_free(&ini);
  return 0;
}

int test_ini_iteration_helpers()
{
  const char *txt =
    "rootkey = r\n"
    "[s1]\n"
    "a = 1\n"
    "b = 2\n"
    "[s2]\n"
    "c = 3\n";

  XIni ini;
  XIniError iniError;
  ASSERT_TRUE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));

  // sections: "" (global), "s1", "s2"
  ASSERT_TRUE(x_ini_section_count(&ini) == 3);
  ASSERT_TRUE(s_eq(x_ini_section_name(&ini, 0), ""));
  ASSERT_TRUE(s_eq(x_ini_section_name(&ini, 1), "s1"));
  ASSERT_TRUE(s_eq(x_ini_section_name(&ini, 2), "s2"));

  // key counts per section (linear scan inside helpers)
  ASSERT_TRUE(x_ini_key_count(&ini, 0) == 1);
  ASSERT_TRUE(x_ini_key_count(&ini, 1) == 2);
  ASSERT_TRUE(x_ini_key_count(&ini, 2) == 1);

  /* enumerate names/values by index */
  ASSERT_TRUE(s_eq(x_ini_key_name(&ini, 1, 0), "a"));
  ASSERT_TRUE(s_eq(x_ini_value_at(&ini, 1, 0), "1"));
  ASSERT_TRUE(s_eq(x_ini_key_name(&ini, 1, 1), "b"));
  ASSERT_TRUE(s_eq(x_ini_value_at(&ini, 1, 1), "2"));
  ASSERT_TRUE(s_eq(x_ini_key_name(&ini, 2, 0), "c"));
  ASSERT_TRUE(s_eq(x_ini_value_at(&ini, 2, 0), "3"));

  x_ini_free(&ini);
  return 0;
}

int test_ini_malformed_1()
{
  char *txt =
    "[server\n #missing ']'"
    "port = 8080\n"
    "docroot = \"files\"\n"
    "list_dirs = true\n";


  XIni ini;
  XIniError iniError;
  ASSERT_FALSE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));
  ASSERT_EQ(iniError.code, XINI_ERR_EXPECT_RBRACKET);
  ASSERT_EQ(iniError.line, 1);
  ASSERT_EQ(iniError.column, 8);
  return 0;
}

int test_ini_malformed_2()
{
  char *txt =
    "[server]\n"
    "port = 8080\n"
    "docroot = \"files\"\n"
    "list_dirs true  # Missing '=' sign";

  XIni ini;
  XIniError iniError;
  ASSERT_FALSE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));
  ASSERT_EQ(iniError.code, XINI_ERR_EXPECT_EQUALS);
  ASSERT_EQ(iniError.line, 4);
  return 0;
}

int test_ini_malformed_3()
{
  char *txt =
    "[server]\n"
    "port = 8080\n"
    "docroot = \"files\n # unterminated quoted string"
    "list_dirs = true\n";

  XIni ini;
  XIniError iniError;
  ASSERT_FALSE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));
  ASSERT_EQ(iniError.code, XINI_ERR_UNTERMINATED_STRING);
  ASSERT_EQ(iniError.line, 3);
  ASSERT_EQ(iniError.column, 17);
  return 0;
}

int test_ini_numeric_parsing()
{
  const char *txt =
    "[n]\n"
    "i = -123\n"
    "f = 3.5\n"
    "hex = 0x10\n";

  XIni ini;
  XIniError iniError;
  ASSERT_TRUE(x_ini_load_mem(txt, strlen(txt), &ini, &iniError));
  ASSERT_TRUE(x_ini_get_i32(&ini, "n", "i", 0) == -123);
  ASSERT_TRUE(x_ini_get_f32(&ini, "n", "f", 0.0f) > 3.49f && x_ini_get_f32(&ini, "n", "f", 0.0f) < 3.51f);
  // strtol with base 0 handles 0x prefix
  ASSERT_TRUE(x_ini_get_i32(&ini, "n", "hex", 0) == 16);

  x_ini_free(&ini);
  return 0;
}

int main(void)
{
  STDXTestCase tests[] =
  {
    X_TEST(test_ini_parse_basic),
    X_TEST(test_ini_global_section_and_lookup),
    X_TEST(test_ini_last_definition_wins),
    X_TEST(test_ini_inline_comments_and_whitespace),
    X_TEST(test_ini_bool_variants),
    X_TEST(test_ini_iteration_helpers),
    X_TEST(test_ini_malformed_1),
    X_TEST(test_ini_malformed_2),
    X_TEST(test_ini_malformed_3),
    X_TEST(test_ini_numeric_parsing),
  };

  return stdx_run_tests(tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
