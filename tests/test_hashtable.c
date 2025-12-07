#include <stdx_common.h>
#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_HASHTABLE
#include <stdx_hashtable.h>
#include <stdx_arena.h>
#include <stdint.h>
#include <string.h>

#define ref_i(a) X_VALUE_PTR(int32_t, (a))
#define ref_f(a) X_VALUE_PTR(float, (a))
#define ref_d(a) X_VALUE_PTR(double, (a))
#define ref_point(a) X_VALUE_PTR(Point, (a))

int test_x_hashtable_rehash_ints(void)
{
  XHashtable* ht = x_hashtable_create(int32_t, char*);

  for (int i = 0; i < 100; i++)
  {
    char buf[32];
    sprintf(buf, "entry_%d", i);
    ASSERT_TRUE(x_hashtable_set(ht, &i,	 &buf));
  }

char* out;
  ASSERT_TRUE(x_hashtable_get(ht, ref_i(42), &out));
  ASSERT_TRUE(strcmp(out, "entry_42") == 0);
  return 0;
}

int test_x_hashtable_rehash_string(void)
{
  XHashtable* ht = x_hashtable_create(char*, char*);

  x_hashtable_set(ht, "zip",	 "zip");
  x_hashtable_set(ht, "xul",	  "application/vnd.mozilla.xul+xml");
  x_hashtable_set(ht, "xml",	  "text/xml");
  x_hashtable_set(ht, "xlsx",   "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
  x_hashtable_set(ht, "xls",	  "application/vnd.ms-excel");
  x_hashtable_set(ht, "xhtml",	"application/xhtml+xml");
  x_hashtable_set(ht, "woff2",  "font/woff2");
  x_hashtable_set(ht, "woff",   "font/woff");
  x_hashtable_set(ht, "webp",   "image/webp");
  x_hashtable_set(ht, "webm",   "video/webm");
  x_hashtable_set(ht, "weba",   "audio/webm");
  x_hashtable_set(ht, "wav",    "audio/wav");
  x_hashtable_set(ht, "vsd",    "application/vnd.visio");
  x_hashtable_set(ht, "txt",    "text/plain");
  x_hashtable_set(ht, "ttf",    "font/ttf");
  x_hashtable_set(ht, "ts",     "video/mp2t");
  x_hashtable_set(ht, "tiff",   "image/tiff");
  x_hashtable_set(ht, "tif",    "image/tiff");
  x_hashtable_set(ht, "tar",    "application/x-tar");
  x_hashtable_set(ht, "swf",    "application/x-shockwave-flash");
  x_hashtable_set(ht, "svg",    "image/svg+xml");
  x_hashtable_set(ht, "sh",     "application/x-sh");
  x_hashtable_set(ht, "rtf",    "application/rtf");
  x_hashtable_set(ht, "rar",    "application/vnd.rar");
  x_hashtable_set(ht, "pptx",   "application/vnd.openxmlformats-officedocument.presentationml.presentation");
  x_hashtable_set(ht, "ppt",    "application/vnd.ms-powerpoint");
  x_hashtable_set(ht, "png",    "image/png");
  x_hashtable_set(ht, "php",    "application/x-httpd-php");
  x_hashtable_set(ht, "pdf",    "application/pdf");
  x_hashtable_set(ht, "otf",    "font/otf");
  x_hashtable_set(ht, "opus",   "audio/opus");
  x_hashtable_set(ht, "ogx",    "application/ogg");
  x_hashtable_set(ht, "ogv",    "video/ogg");
  x_hashtable_set(ht, "oga",    "audio/ogg");
  x_hashtable_set(ht, "odt",    "application/vnd.oasis.opendocument.text");
  x_hashtable_set(ht, "ods",    "application/vnd.oasis.opendocument.spreadsheet");
  x_hashtable_set(ht, "odp",    "application/vnd.oasis.opendocument.presentation");
  x_hashtable_set(ht, "mpkg",   "application/vnd.apple.installer+xml");
  x_hashtable_set(ht, "mpeg",   "video/mpeg");
  x_hashtable_set(ht, "mp3",    "audio/mpeg");
  x_hashtable_set(ht, "mjs",    "text/javascript");
  x_hashtable_set(ht, "midi",   "audio/x-midi");
  x_hashtable_set(ht, "mid",    "audio/x-midi");
  x_hashtable_set(ht, "jsonld", "application/ld+json");
  x_hashtable_set(ht, "json",   "application/json");
  x_hashtable_set(ht, "js",     "text/javascript");
  x_hashtable_set(ht, "jpg",    "image/jpeg");
  x_hashtable_set(ht, "jpeg",   "image/jpeg");
  x_hashtable_set(ht, "jar",    "application/java-archive");
  x_hashtable_set(ht, "ics",    "text/calendar");
  x_hashtable_set(ht, "ico",    "image/vnd.microsoft.icon");
  x_hashtable_set(ht, "html",   "text/html");
  x_hashtable_set(ht, "htm",    "text/html");
  x_hashtable_set(ht, "gz",     "application/gzip");
  x_hashtable_set(ht, "gif",    "image/gif");
  x_hashtable_set(ht, "epub",   "application/epub+zip");
  x_hashtable_set(ht, "eot",    "application/vnd.ms-fontobject");
  x_hashtable_set(ht, "docx",   "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
  x_hashtable_set(ht, "doc",    "application/msword");
  x_hashtable_set(ht, "csv",    "text/csv");
  x_hashtable_set(ht, "css",    "text/css");
  x_hashtable_set(ht, "csh",    "application/x-csh");
  x_hashtable_set(ht, "bz2",    "application/x-bzip2");
  x_hashtable_set(ht, "bz",     "application/x-bzip");
  x_hashtable_set(ht, "bmp",    "OS/2 Bitmap Graphics	image/bmp");
  x_hashtable_set(ht, "bin",    "application/octet-stream");
  x_hashtable_set(ht, "azw",    "vnd.amazon.ebook");
  x_hashtable_set(ht, "avi",    "video/x-msvideo");
  x_hashtable_set(ht, "arc",    "application/x-freearc");
  x_hashtable_set(ht, "abw",    "application/x-abiword");
  x_hashtable_set(ht, "aac",    "audio/aac");
  x_hashtable_set(ht, "7z",     "application/x-7z-compressed");
  x_hashtable_set(ht, "3gp",	  "video/3gpp");
  x_hashtable_set(ht, "3g2",	  "video/3gpp2");


  const char* out;
  ASSERT_TRUE(x_hashtable_get(ht, "jpg", &out));
  ASSERT_TRUE(strcmp(out, "image/jpeg") == 0);

  ASSERT_TRUE(x_hashtable_get(ht, "svg", &out));
  ASSERT_TRUE(strcmp(out, "image/svg+xml") == 0);

  ASSERT_TRUE(x_hashtable_get(ht, "pptx", &out));
  ASSERT_TRUE(strcmp(out, "application/vnd.openxmlformats-officedocument.presentationml.presentation") == 0);

  return 0;

}

int test_str_key_str_val(void)
{
  XHashtable* ht = x_hashtable_create(char*, char*);

  const char* k = "foo";
  const char* v = "bar";
  x_hashtable_set(ht, k, v);
  ASSERT_TRUE(x_hashtable_count(ht) == 1);

  x_hashtable_set(ht, "html", "text/html");
  ASSERT_TRUE(x_hashtable_count(ht) == 2);

  {
    const char* out;
    ASSERT_TRUE(x_hashtable_get(ht, "html", &out));
    ASSERT_TRUE(strncmp(out, "text/html", 9) == 0);
  }

  {
    const char* out;
    ASSERT_TRUE(x_hashtable_get(ht, k, &out));
    ASSERT_TRUE(strncmp(out, v, strlen(v)) == 0);
  }


  x_hashtable_destroy(ht);
  return 0;
}

int test_str_key_copy_val(void)
{

  XHashtable* ht = x_hashtable_create(char*, int32_t);

  x_hashtable_set(ht, "FIVE",   ref_i(5));
  x_hashtable_set(ht, "SIX",    ref_i(6));
  x_hashtable_set(ht, "SEVEN",  ref_i(7));
  x_hashtable_set(ht, "EIGHT",  ref_i(8));

  ASSERT_TRUE(x_hashtable_has(ht, "SIX"));
  ASSERT_FALSE(x_hashtable_has(ht, "NINE"));
  ASSERT_TRUE(x_hashtable_count(ht) == 4);

  ASSERT_TRUE(x_hashtable_remove(ht, "SEVEN"));
  ASSERT_TRUE(x_hashtable_count(ht) == 3);

  uint32_t out;
  ASSERT_TRUE(x_hashtable_get(ht, "EIGHT", &out));
  ASSERT_TRUE(out == 8);
  ASSERT_FALSE(x_hashtable_get(ht, "SEVEN", &out));
  ASSERT_TRUE(x_hashtable_get(ht, "FIVE", &out));
  ASSERT_TRUE(out == 5);
  ASSERT_TRUE(x_hashtable_get(ht, "SIX", &out));
  ASSERT_TRUE(out == 6);


  x_hashtable_destroy(ht);
  return 0;
}

int test_copy_key_str_val(void)
{
  XHashtable* ht = x_hashtable_create(float, char*);

  x_hashtable_set(ht, ref_f(5.000f), "FIVE");
  x_hashtable_set(ht, ref_f(6.000f), "SIX");
  x_hashtable_set(ht, ref_f(7.000f), "SEVEN");
  x_hashtable_set(ht, ref_f(8.000f), "EIGHT");

  ASSERT_TRUE(x_hashtable_has(ht, ref_f(5.000f)));
  ASSERT_FALSE(x_hashtable_has(ht, ref_f(9.000f)));
  ASSERT_TRUE(x_hashtable_count(ht) == 4);

  ASSERT_TRUE(x_hashtable_remove(ht, ref_f(7.000f)));
  ASSERT_TRUE(x_hashtable_count(ht) == 3);


  char* out;
  ASSERT_TRUE(x_hashtable_get(ht, ref_f(8.000f), &out));
  ASSERT_TRUE(strcmp(out, "EIGHT") == 0);
  ASSERT_FALSE(x_hashtable_get(ht, ref_f(7.000f), &out));
  ASSERT_TRUE(x_hashtable_get(ht, ref_f(5.000f), &out));
  ASSERT_TRUE(strcmp(out, "FIVE") == 0);
  ASSERT_TRUE(x_hashtable_get(ht, ref_f(6.000f), &out));
  ASSERT_TRUE(strcmp(out, "SIX") == 0);


  x_hashtable_destroy(ht);
  return 0;
}

int test_copy_key_copy_val(void)
{
  XHashtable* ht = x_hashtable_create_full(
      sizeof(double),
      sizeof(float),
      x_hashtable_hash_bytes,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL);

  x_hashtable_set(ht, ref_d(5.000), ref_f(10.000f));
  x_hashtable_set(ht, ref_d(6.000), ref_f(12.000f));
  x_hashtable_set(ht, ref_d(7.000), ref_f(14.000f));
  x_hashtable_set(ht, ref_d(8.000), ref_f(16.000f));

  ASSERT_TRUE(x_hashtable_has(ht, ref_d(5)));
  ASSERT_FALSE(x_hashtable_has(ht, ref_d(9)));
  ASSERT_TRUE(x_hashtable_count(ht) == 4);

  ASSERT_TRUE(x_hashtable_remove(ht, ref_d(7)));
  ASSERT_TRUE(x_hashtable_count(ht) == 3);

  float out;
  ASSERT_FALSE(x_hashtable_get(ht, ref_d(7), &out));
  ASSERT_TRUE(x_hashtable_get(ht, ref_d(8), &out));
  ASSERT_TRUE(out == 16.0f);
  ASSERT_TRUE(x_hashtable_get(ht, ref_d(5), &out));
  ASSERT_TRUE(out == 10.0f);
  ASSERT_TRUE(x_hashtable_get(ht, ref_d(6), &out));
  ASSERT_TRUE(out == 12.0f);

  x_hashtable_destroy(ht);
  return 0;
}

int test_int_key_struct_value(void)
{

  typedef struct 
  {
    float x, y;
  } Point;


  XHashtable* ht = x_hashtable_create(int, Point);

  Point p1 = {5.0f, 10.0f};
  x_hashtable_set(ht, ref_i(5), &p1);

  Point p2 = (Point){.x = 6.0f, .y = 12.0f};
  x_hashtable_set(ht, ref_i(6), &p2);

  Point p3 = (Point){.x = 7.0f, .y = 14.0f};
  x_hashtable_set(ht, ref_i(7), &p3);

  Point p4 = (Point){.x = 8.0f, .y = 16.0f};
  x_hashtable_set(ht, ref_i(8), &p4);

  ASSERT_TRUE(x_hashtable_has(ht, ref_i(5)));
  ASSERT_FALSE(x_hashtable_has(ht, ref_i(9)));
  ASSERT_TRUE(x_hashtable_count(ht) == 4);

  ASSERT_TRUE(x_hashtable_remove(ht, ref_i(7)));
  ASSERT_TRUE(x_hashtable_count(ht) == 3);

  Point out;
  ASSERT_FALSE(x_hashtable_get(ht, ref_i(7), &out));

  ASSERT_TRUE(x_hashtable_get(ht, ref_i(8), &out));
  ASSERT_TRUE(memcmp(&out, &p4, sizeof(Point)) == 0);

  ASSERT_TRUE(x_hashtable_get(ht, ref_i(5), &out));
  ASSERT_TRUE(memcmp(&out, &p1, sizeof(Point)) == 0);

  ASSERT_TRUE(x_hashtable_get(ht, ref_i(6), &out));
  ASSERT_TRUE(memcmp(&out, &p2, sizeof(Point)) == 0);

  x_hashtable_destroy(ht);
  return 0;
}

int test_pointers_as_keys(void)
{
  // pointer to this funciton
  int (*this_func)(void) = test_pointers_as_keys;

  XHashtable* ht = x_hashtable_create(int*, char*);
  ASSERT_TRUE(x_hashtable_set(ht, this_func, "Hello, World!"));
  ASSERT_TRUE(x_hashtable_set(ht, this_func, "test_pointers_as_keys()"));
  char* out;
  ASSERT_TRUE(x_hashtable_get(ht, test_pointers_as_keys, &out));
  ASSERT_TRUE(strcmp(out, "test_pointers_as_keys()") == 0);
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_pointers_as_keys),
    X_TEST(test_str_key_str_val),
    X_TEST(test_str_key_copy_val),
    X_TEST(test_copy_key_str_val),
    X_TEST(test_copy_key_copy_val),
    X_TEST(test_int_key_struct_value),
    X_TEST(test_x_hashtable_rehash_string),
    X_TEST(test_x_hashtable_rehash_ints)
  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]));
}
