/*
 * STDX - Lightweight String Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * To compile the implementation define X_IMPL_STRING
 * in **one** source file before including this header.
 *
 * Notes:
 *  This header provides:
 *   - C string helpers: case-insensitive prefix/suffix matching
 *   - XSmallstr: fixed-capacity, stack-allocated strings
 *   - XStrview: immutable, non-owning views into C strings
 *   - Tokenization and trimming
 *   - UTF-8-aware string length calculation
 *   - Fast substring and search operations
 *   - Case-sensitive and case-insensitive comparisons
 */

#ifndef X_STRING_H
#define X_STRING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifndef X_STRING_API
#define X_STRING_API
#endif

#define X_STRING_VERSION_MAJOR 1
#define X_STRING_VERSION_MINOR 0
#define X_STRING_VERSION_PATCH 0
#define X_STRING_VERSION (X_STRING_VERSION_MAJOR * 10000 + X_STRING_VERSION_MINOR * 100 + X_STRING_VERSION_PATCH)

#ifndef X_SMALLSTR_MAX_LENGTH
#define X_SMALLSTR_MAX_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct 
  {
    const char* data;
    size_t length;
  } XStrview;

  typedef struct
  {
    const wchar_t* data;
    size_t length; // In wide characters
  } XWStrview;

  typedef struct 
  {
    char buf[X_SMALLSTR_MAX_LENGTH + 1];
    size_t length;
  } XSmallstr;

  typedef struct
  {
    wchar_t buf[X_SMALLSTR_MAX_LENGTH + 1];
    size_t length; // In wide characters
  } XWSmallstr;

  X_STRING_API void      x_set_locale(const char* locale);
  X_STRING_API // C-string (null-terminated char) functions
    X_STRING_API uint32_t  x_cstr_hash(const char* str);
  X_STRING_API char*     x_cstr_str(const char *haystack, const char *needle);
  X_STRING_API bool      x_cstr_ends_with(const char* str, const char* suffix);
  X_STRING_API bool      x_cstr_starts_with(const char* str, const char* prefix);
  X_STRING_API bool      x_cstr_ends_with_ci(const char* str, const char* suffix);
  X_STRING_API bool      x_cstr_starts_with_ci(const char* str, const char* prefix);

  // Wide C-string (wchar_t) functions
  X_STRING_API bool      x_wcstr_starts_with(const wchar_t* str, const wchar_t* prefix);
  X_STRING_API bool      x_wcstr_ends_with(const wchar_t* str, const wchar_t* suffix);
  X_STRING_API bool      x_wcstr_starts_with_case(const wchar_t* str, const wchar_t* prefix);
  X_STRING_API bool      x_wcstr_ends_with_case(const wchar_t* str, const wchar_t* suffix);
  X_STRING_API int32_t   x_wcstr_casecmp(const wchar_t* a, const wchar_t* b);

  // UTF-8 and wide string conversions and utility functions
  X_STRING_API size_t    x_cstr_to_wcstr(const char* src, wchar_t* dest, size_t max);
  X_STRING_API size_t    x_wcstr_to_utf8(const wchar_t* wide, char* utf8, size_t max);
  X_STRING_API size_t    x_wcstr_to_cstr(const wchar_t* src, char* dest, size_t max);
  X_STRING_API size_t    x_utf8_to_wcstr(const char* utf8, wchar_t* wide, size_t max);

  //UTF-8 string manipulation
  X_STRING_API size_t    x_utf8_strlen(const char* utf8);
  X_STRING_API int32_t   x_utf8_strcmp(const char* a, const char* b);
  X_STRING_API char*     x_utf8_tolower(char* dest, const char* src);
  X_STRING_API char*     x_utf8_toupper(char* dest, const char* src);
  X_STRING_API bool      x_utf8_is_single_char(const char* s);
  X_STRING_API int32_t   x_utf8_decode(const char* ptr, const char* end, size_t* out_len);
  X_STRING_API int32_t   x_utf8_codepoint_length(unsigned char first_byte);

  //XSmallstr - A small fixed-size string abstraction with various utility functions.
  X_STRING_API int32_t   x_smallstr_init(XSmallstr* s, const char* str);
  X_STRING_API size_t    x_smallstr_format(XSmallstr* s, const char* fmt, ...);
  X_STRING_API size_t    x_smallstr_from_cstr(XSmallstr* s, const char* cstr);
  X_STRING_API size_t    x_smallstr_from_strview(XStrview sv, XSmallstr* out);
  X_STRING_API XStrview  x_smallstr_to_strview(const XSmallstr* s);
  X_STRING_API size_t    x_smallstr_length(const XSmallstr* s);
  X_STRING_API void      x_smallstr_clear(XSmallstr* s);
  X_STRING_API size_t    x_smallstr_append_cstr(XSmallstr* s, const char* cstr);
  X_STRING_API size_t    x_smallstr_append_char(XSmallstr* s, char c);
  X_STRING_API size_t    x_smallstr_substring(const XSmallstr* s, size_t start, size_t len, XSmallstr* out);
  X_STRING_API void      x_smallstr_trim_left(XSmallstr* s);
  X_STRING_API void      x_smallstr_trim_right(XSmallstr* s);
  X_STRING_API void      x_smallstr_trim(XSmallstr* s);
  X_STRING_API int32_t   x_smallstr_cmp_case(const XSmallstr* a, const XSmallstr* b);
  X_STRING_API int32_t   x_smallstr_replace_all(XSmallstr* s, const char* find, const char* replace);
  X_STRING_API size_t    x_smallstr_utf8_len(const XSmallstr* s);
  X_STRING_API int32_t   x_smallstr_cmp(const XSmallstr* a, const XSmallstr* b);
  X_STRING_API int32_t   x_smallstr_cmp_cstr(const XSmallstr* a, const char* b);
  X_STRING_API const char* x_smallstr_cstr(const XSmallstr* s);
  X_STRING_API int32_t   x_smallstr_utf8_substring(const XSmallstr* s, size_t start_cp, size_t len_cp, XSmallstr* out);
  X_STRING_API void      x_smallstr_utf8_trim_left(XSmallstr* s);
  X_STRING_API void      x_smallstr_utf8_trim_right(XSmallstr* s);
  X_STRING_API void      x_smallstr_utf8_trim(XSmallstr* s);

  // XWSmallstr
  // Wide-char version of small string abstraction.

  X_STRING_API int32_t   x_wsmallstr_init(XWSmallstr* s, const wchar_t* src);
  X_STRING_API int32_t   x_wsmallstr_cmp(const XWSmallstr* a, const XWSmallstr* b);
  X_STRING_API int32_t   x_wsmallstr_cmp_cstr(const XWSmallstr* a, const wchar_t* b);
  X_STRING_API int32_t   x_wsmallstr_cmp_case(const XWSmallstr* a, const XWSmallstr* b);
  X_STRING_API size_t    x_wsmallstr_length(const XWSmallstr* s);
  X_STRING_API void      x_wsmallstr_clear(XWSmallstr* s);
  X_STRING_API int32_t   x_wsmallstr_append(XWSmallstr* s, const wchar_t* tail);
  X_STRING_API int32_t   x_wsmallstr_substring(const XWSmallstr* s, size_t start, size_t len, XWSmallstr* out);
  X_STRING_API void      x_wsmallstr_trim(XWSmallstr* s);
  X_STRING_API int32_t   x_wsmallstr_find(const XWSmallstr* s, wchar_t c);


  // XStrview (string views on char)*
  // Non-owning string views and operations.

#define     x_strview_empty() ((XStrview){ .data = 0, .length = 0 })
#define     x_strview_is_empty(sv) ((sv).length == 0)
#define     x_strview_init(cstr, len) ((XStrview){ .data = (cstr), .length = (len) })
#define     x_strview(cstr)           (x_strview_init( (cstr), strlen(cstr) ))

  X_STRING_API bool      x_strview_eq(XStrview a, XStrview b);
  X_STRING_API bool      x_strview_eq_cstr(XStrview a, const char* b);
  X_STRING_API bool      x_strview_eq_case(XStrview a, XStrview b);
  X_STRING_API int32_t   x_strview_cmp(XStrview a, XStrview b);
  X_STRING_API int32_t   x_strview_cmp_case(XStrview a, XStrview b);
  X_STRING_API XStrview  x_strview_substr(XStrview sv, size_t start, size_t len);
  X_STRING_API XStrview  x_strview_trim_left(XStrview sv);
  X_STRING_API XStrview  x_strview_trim_right(XStrview sv);
  X_STRING_API XStrview  x_strview_trim(XStrview sv);
  X_STRING_API int32_t   x_strview_find(XStrview sv, char c);
  X_STRING_API int32_t   x_strview_find_white_space(XStrview sv);
  X_STRING_API int32_t   x_strview_rfind(XStrview sv, char c);
  X_STRING_API bool      x_strview_split_at(XStrview sv, char delim, XStrview* left, XStrview* right);
  X_STRING_API bool      x_strview_split_at_white_space(XStrview sv, XStrview* left, XStrview* right);
  X_STRING_API bool      x_strview_next_token_white_space(XStrview* input, XStrview* token);
  X_STRING_API bool      x_strview_next_token(XStrview* input, char delim, XStrview* token);
  X_STRING_API bool      x_strview_starts_with_cstr(XStrview sv, const char* prefix);
  X_STRING_API bool      x_strview_ends_with_cstr(XStrview sv, const char* prefix);

  X_STRING_API XStrview  x_strview_utf8_substr(XStrview sv, size_t char_start, size_t char_len);
  X_STRING_API XStrview  x_strview_utf8_trim_left(XStrview sv);
  X_STRING_API XStrview  x_strview_utf8_trim_right(XStrview sv);
  X_STRING_API XStrview  x_strview_utf8_trim(XStrview sv);
  X_STRING_API int32_t   x_strview_utf8_find(XStrview sv, uint32_t codepoint);
  X_STRING_API int32_t   x_strview_utf8_rfind(XStrview sv, uint32_t codepoint);
  X_STRING_API bool      x_strview_utf8_split_at(XStrview sv, uint32_t delim, XStrview* left, XStrview* right);
  X_STRING_API bool      x_strview_utf8_next_token(XStrview* input, uint32_t codepoint, XStrview* token);
  X_STRING_API bool      x_strview_utf8_starts_with_cstr(XStrview sv, const char* prefix);
  X_STRING_API bool      x_strview_utf8_ends_with_cstr(XStrview sv, const char* prefix);

  // XWStrview (string views on wchar_t)*
  // Non-owning wide string views and operations.

#define x_wstrview_init(cstr, len) ((XWStrview){ .data = (cstr), .length = (len) })
#define x_wstrview(cstr)           (x_wstrview_init( (cstr), wcslen(cstr) ))

  X_STRING_API bool      x_wstrview_empty(XWStrview sv);
  X_STRING_API bool      x_wstrview_eq(XWStrview a, XWStrview b);
  X_STRING_API int32_t   x_wstrview_cmp(XWStrview a, XWStrview b);
  X_STRING_API XWStrview x_wstrview_substr(XWStrview sv, size_t start, size_t len);
  X_STRING_API XWStrview x_wstrview_trim_left(XWStrview sv);
  X_STRING_API XWStrview x_wstrview_trim_right(XWStrview sv);
  X_STRING_API XWStrview x_wstrview_trim(XWStrview sv);
  X_STRING_API bool      x_wstrview_split_at(XWStrview sv, uint32_t delim, XWStrview* left, XWStrview* right);
  X_STRING_API bool      x_wstrview_next_token(XWStrview* input, wchar_t delim, XWStrview* token);

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_STRING

#include <ctype.h>   /* tolower */
#include <stddef.h>  /* size_t  */
#include <string.h>  /* strlen, memcmp, strncasecmp (POSIX) */
#include <stdint.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

#ifdef __cplusplus
extern "C" {
#endif

  static bool s_is_unicode_whitespace(uint32_t cp)
  {
    return (cp == 0x09 || cp == 0x0A || cp == 0x0B || cp == 0x0C || cp == 0x0D || cp == 0x20 ||
        cp == 0x85 || cp == 0xA0 || cp == 0x1680 ||
        (cp >= 0x2000 && cp <= 0x200A) ||
        cp == 0x2028 || cp == 0x2029 || cp == 0x202F || cp == 0x205F || cp == 0x3000);
  }

  static int32_t utf8_decode(const char** ptr, const char* end)
  {
    if (*ptr >= end) return -1;  // no input left
    const unsigned char* s = (const unsigned char*)(*ptr);

    uint32_t cp;
    if (s[0] < 0x80) {
      cp = s[0];
      *ptr += 1;
    } else if ((s[0] & 0xE0) == 0xC0 && s + 1 < (const unsigned char*)end &&
        (s[1] & 0xC0) == 0x80) {
      cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
      *ptr += 2;
    } else if ((s[0] & 0xF0) == 0xE0 && s + 2 < (const unsigned char*)end &&
        (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
      cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
      *ptr += 3;
    } else if ((s[0] & 0xF8) == 0xF0 && s + 3 < (const unsigned char*)end &&
        (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 &&
        (s[3] & 0xC0) == 0x80) {
      cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
        ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
      *ptr += 4;
    } else {
      *ptr += 1;
      return 0xFFFD; // Unicode replacement character
    }
    return cp;
  }

  static size_t utf8_advance(const char* s, size_t len, size_t chars)
  {
    size_t i = 0, count = 0;
    while (i < len && count < chars)
    {
      char c = (char)s[i];
      if ((c & 0x80) == 0x00) i += 1;
      else if ((c & 0xE0) == 0xC0) i += 2;
      else if ((c & 0xF0) == 0xE0) i += 3;
      else if ((c & 0xF8) == 0xF0) i += 4;
      else break;
      count++;
    }
    return i;
  }

  X_STRING_API char* x_cstr_str(const char *haystack, const char *needle)
  {
    if (!*needle)
      return (char *)haystack;

    for (; *haystack; haystack++)
    {
      const char *h = haystack;
      const char *n = needle;

      while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n))
      {
        h++;
        n++;
      }

      if (!*n)  // Reached end of needle => match
        return (char *)haystack;
    }
    return NULL;
  }

  X_STRING_API bool x_cstr_ends_with(const char* str, const char* suffix)
  {
    const char* found = strstr(str, suffix);
    if (found == NULL || found + strlen(suffix) != str + strlen(str))


    {
      return false;
    }
    return true;
  }

  X_STRING_API bool x_cstr_starts_with(const char* str, const char* prefix)
  {
    const char* found = strstr(str, prefix);
    bool match = (found == str);
    if (match && prefix[0] == 0 && str[0] != 0)
      match = false;

    return match;
  }

  X_STRING_API bool x_cstr_starts_with_ci(const char* str, const char* prefix)
  {
    const char* found = x_cstr_str(str, prefix);
    bool match = (found == str);
    if (match && prefix[0] == 0 && str[0] != 0)
      match = false;

    return match;
  }

  X_STRING_API bool x_wcstr_starts_with(const wchar_t* str, const wchar_t* prefix)
  {
    size_t plen = wcslen(prefix);
    return wcsncmp(str, prefix, plen) == 0;
  }

  X_STRING_API bool x_wcstr_ends_with(const wchar_t* str, const wchar_t* suffix)
  {
    size_t slen = wcslen(str);
    size_t suf_len = wcslen(suffix);
    if (slen < suf_len) return false;
    return wcscmp(str + slen - suf_len, suffix) == 0;
  }

  X_STRING_API bool x_wcstr_starts_with_case(const wchar_t* str, const wchar_t* prefix)
  {
    while (*prefix && *str)
    {
      if (towlower(*str++) != towlower(*prefix++))
        return false;
    }
    return *prefix == 0;
  }

  X_STRING_API bool x_wcstr_ends_with_case(const wchar_t* str, const wchar_t* suffix)
  {
    size_t slen = wcslen(str);
    size_t suflen = wcslen(suffix);
    if (slen < suflen) return false;

    const wchar_t* str_tail = str + slen - suflen;
    while (*suffix)
    {
      if (towlower(*str_tail++) != towlower(*suffix++))
        return false;
    }
    return true;
  }

  X_STRING_API int32_t  x_wcstr_casecmp(const wchar_t* a, const wchar_t* b)
  {
    while (*a && *b)
    {
      wint_t ca = towlower(*a++);
      wint_t cb = towlower(*b++);
      if (ca != cb) return ca - cb;
    }
    return towlower(*a) - towlower(*b);
  }

  // Ensure locale is set before using this
  X_STRING_API size_t x_cstr_to_wcstr(const char* src, wchar_t* dest, size_t max)
  {
    if (!src || !dest || max == 0) return 0;
    mbstate_t state =
    {0};
    size_t len = mbsrtowcs(dest, &src, max - 1, &state);
    if (len == (size_t)-1)
    {
      dest[0] = L'\0';
      return 0;
    }
    dest[len] = L'\0';
    return len;
  }

  X_STRING_API size_t x_wcstr_to_utf8(const wchar_t* wide, char* utf8, size_t max)
  {
    mbstate_t ps = {0};
    return wcsrtombs(utf8, (const wchar_t**)&wide, max, &ps);
  }

  X_STRING_API size_t x_wcstr_to_cstr(const wchar_t* src, char* dest, size_t max)
  {
    if (!src || !dest || max == 0) return 0;
    mbstate_t state =
    {0};
    size_t len = wcsrtombs(dest, &src, max - 1, &state);
    if (len == (size_t)-1)
    {
      dest[0] = '\0';
      return 0;
    }
    dest[len] = '\0';
    return len;
  }

  X_STRING_API bool x_cstr_ends_with_ci(const char* str, const char* suffix)
  {
    const char* found = x_cstr_str(str, suffix);
    if (found == NULL || found + strlen(suffix) != str + strlen(str))
    {
      return false;
    }
    return true;
  }

  X_STRING_API void x_set_locale(const char* locale)
  {
    setlocale(LC_ALL, locale ? locale : "");
  }

  X_STRING_API size_t x_utf8_strlen(const char* utf8)
  {
    size_t count = 0;
    while (*utf8)
    {
      char c = (char)*utf8;
      if ((c & 0x80) == 0x00) utf8 += 1;
      else if ((c & 0xE0) == 0xC0) utf8 += 2;
      else if ((c & 0xF0) == 0xE0) utf8 += 3;
      else if ((c & 0xF8) == 0xF0) utf8 += 4;
      else break;
      count++;
    }
    return count;
  }

  X_STRING_API int32_t  x_utf8_strcmp(const char* a, const char* b)
  {
    if (setlocale(LC_ALL, NULL) != NULL) return strcoll(a, b);
    return strcmp(a, b); // no locale set, fallback to pure ascii comparisons
  }

  X_STRING_API char* x_utf8_tolower(char* dest, const char* src)
  {
    wchar_t wbuf[512];
    x_utf8_to_wcstr(src, wbuf, 512);
    for (size_t i = 0; wbuf[i]; ++i)
      wbuf[i] = towlower(wbuf[i]);
    x_wcstr_to_utf8(wbuf, dest, 512);
    return dest;
  }

  X_STRING_API char* x_utf8_toupper(char* dest, const char* src)
  {
    wchar_t wbuf[512];
    x_utf8_to_wcstr(src, wbuf, 512);
    for (size_t i = 0; wbuf[i]; ++i)
      wbuf[i] = towupper(wbuf[i]);
    x_wcstr_to_utf8(wbuf, dest, 512);
    return dest;
  }

  X_STRING_API int32_t x_utf8_codepoint_length(unsigned char first_byte)
  {
    if (first_byte < 0x80)                return 1;
    else if ((first_byte & 0xE0) == 0xC0) return 2;
    else if ((first_byte & 0xF0) == 0xE0) return 3;
    else if ((first_byte & 0xF8) == 0xF0) return 4;
    else return -1; // Invalid UTF-8 start byte
  }

  X_STRING_API size_t x_utf8_to_wcstr(const char* utf8, wchar_t* wide, size_t max)
  {
    mbstate_t ps = {0};
    return mbsrtowcs(wide, &utf8, max, &ps);
  }

  X_STRING_API int32_t x_utf8_decode(const char* ptr, const char* end, size_t* out_len)
  {
    int32_t codepoint = utf8_decode(&ptr, end);
    if (codepoint && out_len)
      *out_len = x_utf8_codepoint_length(codepoint);
    return codepoint;
  }

  X_STRING_API bool x_utf8_is_single_char(const char* s)
  {
    const char* end = s + strlen(s);
    const char* p = s;
    uint32_t cp = utf8_decode(&p, end);
    return *p == '\0' && cp != 0;
  }

  X_STRING_API size_t x_wide_to_utf8(const wchar_t* wide, char* utf8, size_t max)
  {
    mbstate_t ps =
    {0};
    return wcsrtombs(utf8, (const wchar_t**)&wide, max, &ps);
  }

  X_STRING_API uint32_t x_cstr_hash(const char* str)
  {
    uint32_t hash = 2166136261u; // FNV offset basis
    while (*str)
    {
      hash ^= (unsigned int) *str++;
      hash *= 16777619u; // FNV prime
    }
    return hash;
  }

  X_STRING_API size_t smallstr(XSmallstr* smallString, const char* str)
  {
    size_t length = strlen(str);
#if defined(_DEBUG) || defined(DEBUG)
#if defined(X_x_smallstr_ENABLE_DBG_MESSAGES)
    if (length >= X_SMALLSTR_MAX_LENGTH)
    {
      log_print_debug("The string length (%d) exceeds the maximum size of a XSmallstr (%d)", length, X_SMALLSTR_MAX_LENGTH);
    }
#endif
#endif
    strncpy((char *) &smallString->buf, str, X_SMALLSTR_MAX_LENGTH - 1);
    smallString->buf[X_SMALLSTR_MAX_LENGTH - 1] = 0;
    smallString->length = strlen(smallString->buf);
    return smallString->length;
  }

  X_STRING_API size_t x_smallstr_format(XSmallstr* smallString, const char* fmt, ...)
  {
    va_list argList;
    va_start(argList, fmt);
    smallString->length = vsnprintf((char*) &smallString->buf, X_SMALLSTR_MAX_LENGTH-1, fmt, argList);
    va_end(argList);

    bool success = smallString->length > 0 && smallString->length < X_SMALLSTR_MAX_LENGTH;
    return success;
  }

  X_STRING_API size_t x_smallstr_from_cstr(XSmallstr* s, const char* cstr)
  {
    size_t len = strlen(cstr);
    if (len > X_SMALLSTR_MAX_LENGTH)
      return -1;
    memcpy(s->buf, cstr, len);
    s->buf[len] = '\0';
    s->length = len;
    return len;
  }

  X_STRING_API const char* x_smallstr_cstr(const XSmallstr* s)
  {
    ((char*)s->buf)[s->length] = '\0';
    return (const char*)s->buf;
  }

  X_STRING_API size_t x_smallstr_length(const XSmallstr* smallString)
  {
    return smallString->length;
  }

  X_STRING_API void x_smallstr_clear(XSmallstr* smallString)
  {
    memset(smallString->buf, 0, X_SMALLSTR_MAX_LENGTH * sizeof(char));
    smallString->length = 0;
  }

  X_STRING_API size_t x_smallstr_append_cstr(XSmallstr* s, const char* cstr)
  {
    size_t len = strlen(cstr);
    if (s->length + len > X_SMALLSTR_MAX_LENGTH) return -1;
    memcpy(&s->buf[s->length], cstr, len);
    s->length += len;
    s->buf[s->length] = '\0';
    return s->length;
  }

  X_STRING_API size_t x_smallstr_append_char(XSmallstr* s, char c)
  {
    if (s->length + 1 > X_SMALLSTR_MAX_LENGTH) return -1;
    s->buf[s->length++] = (char)c;
    s->buf[s->length] = '\0';
    return s->length;
  }

  X_STRING_API size_t x_smallstr_substring(const XSmallstr* s, size_t start, size_t len, XSmallstr* out)
  {
    if (start > s->length || start + len > s->length) return -1;
    out->length = len;
    memcpy(out->buf, &s->buf[start], len);
    out->buf[len] = '\0';
    return len;
  }

  X_STRING_API void x_smallstr_trim_left(XSmallstr* s)
  {
    size_t i = 0;
    while (i < s->length && isspace(s->buf[i])) i++;
    if (i > 0)
    {
      memmove(s->buf, &s->buf[i], s->length - i);
      s->length -= i;
      s->buf[s->length] = '\0';
    }
  }

  X_STRING_API void x_smallstr_trim_right(XSmallstr* s)
  {
    while (s->length > 0 && isspace(s->buf[s->length - 1]))
    {
      s->length--;
    }
    s->buf[s->length] = '\0';
  }

  X_STRING_API void x_smallstr_trim(XSmallstr* s)
  {
    x_smallstr_trim_right(s);
    x_smallstr_trim_left(s);
  }

  X_STRING_API int32_t x_smallstr_cmp_case(const XSmallstr* a, const XSmallstr* b)
  {
    if (a->length != b->length) return 0;
    for (size_t i = 0; i < a->length; i++)
    {
      if (tolower(a->buf[i]) != tolower(b->buf[i])) return 0;
    }
    return 1;
  }

  X_STRING_API int32_t x_smallstr_replace_all(XSmallstr* s, const char* find, const char* replace)
  {
    XSmallstr result;
    x_smallstr_clear(&result);

    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);
    size_t i = 0;

    while (i < s->length)
    {
      if (i + find_len <= s->length && memcmp(&s->buf[i], find, find_len) == 0)
      {
        if (result.length + replace_len > X_SMALLSTR_MAX_LENGTH) return -1;
        memcpy(&result.buf[result.length], replace, replace_len);
        result.length += replace_len;
        i += find_len;
      }
      else
      {
        if (result.length + 1 > X_SMALLSTR_MAX_LENGTH) return -1;
        result.buf[result.length++] = s->buf[i++];
      }
    }

    result.buf[result.length] = '\0';
    *s = result;
    return 0;
  }

  X_STRING_API size_t x_smallstr_utf8_len(const XSmallstr* s)
  {
    size_t count = 0;
    for (size_t i = 0; i < s->length;)
    {
      char c = s->buf[i];
      if ((c & 0x80) == 0x00) i += 1;
      else if ((c & 0xE0) == 0xC0) i += 2;
      else if ((c & 0xF0) == 0xE0) i += 3;
      else if ((c & 0xF8) == 0xF0) i += 4;
      else return count; // invalid byte
      count++;
    }
    return count;
  }

  X_STRING_API size_t x_smallstr_from_strview(XStrview sv, XSmallstr* out)
  {
    x_smallstr_clear(out);
    if (sv.length > X_SMALLSTR_MAX_LENGTH) return -1;
    memcpy(out->buf, sv.data, sv.length);
    out->buf[sv.length] = '\0';
    out->length = sv.length;
    return out->length;
  }

  X_STRING_API int32_t x_smallstr_cmp(const XSmallstr* a, const XSmallstr* b)
  {
    return strncmp(a->buf, b->buf, b->length);
  }

  X_STRING_API int32_t x_smallstr_cmp_cstr(const XSmallstr* a, const char* b)
  {
    return strncmp(a->buf, b, a->length);
  }

  X_STRING_API int32_t x_smallstr_find(const XSmallstr* s, char c)
  {
    for (size_t i = 0; i < s->length; ++i)
    {
      if (s->buf[i] == c) return (int)i;
    }
    return -1;
  }

  X_STRING_API int32_t x_smallstr_rfind(const XSmallstr* s, char c)
  {
    for (size_t i = s->length; i > 0; --i)
    {
      if (s->buf[i - 1] == c) return (int)(i - 1);
    }
    return -1;
  }

  X_STRING_API int32_t x_smallstr_split_at(const XSmallstr* s, char delim, XSmallstr* left, XSmallstr* right)
  {
    int32_t pos = x_smallstr_find(s, delim);
    if (pos < 0) return 0;
    x_smallstr_substring(s, 0, pos, left);
    x_smallstr_substring(s, pos + 1, s->length - pos - 1, right);
    return 1;
  }

  X_STRING_API XStrview x_smallstr_to_strview(const XSmallstr* s)
  {
    return x_strview_init(s->buf, s->length);
  }

  X_STRING_API int32_t x_smallstr_utf8_substring(const XSmallstr* s, size_t start_cp, size_t len_cp, XSmallstr* out)
  {
    size_t start = utf8_advance(s->buf, s->length, start_cp);
    size_t len = utf8_advance(s->buf + start, s->length - start, len_cp);
    if (start + len > s->length) return -1;

    out->length = len;
    memcpy(out->buf, s->buf + start, len);
    out->buf[len] = '\0';
    return 0;
  }

  X_STRING_API void x_smallstr_utf8_trim_left(XSmallstr* s)
  {
    const char* start = s->buf;
    const char* end = s->buf + s->length;

    while (start < end)
    {
      const char* prev = start;
      uint32_t cp = utf8_decode(&start, end);
      if (!s_is_unicode_whitespace(cp))
      {
        size_t offset = prev - s->buf;
        memmove(s->buf, prev, end - prev);
        s->length = end - prev;
        s->buf[s->length] = '\0';
        return;
      }
    }
    x_smallstr_clear(s);
  }

  X_STRING_API void x_smallstr_utf8_trim_right(XSmallstr* s)
  {
    const char* start = s->buf;
    const char* end = s->buf + s->length;
    const char* p = end;

    while (p > start)
    {
      const char* prev = p;
      do { --p; } while (p > start && ((*p & 0xC0) == 0x80));
      const char* decode_from = p;
      uint32_t cp = utf8_decode(&decode_from, end);
      if (!s_is_unicode_whitespace(cp))
      {
        size_t new_len = prev - s->buf;
        s->length = new_len;
        s->buf[new_len] = '\0';
        return;
      }
    }
    x_smallstr_clear(s);
  }

  X_STRING_API void x_smallstr_utf8_trim(XSmallstr* s)
  {
    x_smallstr_utf8_trim_right(s);
    x_smallstr_utf8_trim_left(s);
  }

  X_STRING_API int32_t x_wsmallstr_from_wcstr(XWSmallstr* s, const wchar_t* src)
  {
    size_t len = wcslen(src);
    if (len > X_SMALLSTR_MAX_LENGTH) return -1;
    wcsncpy(s->buf, src, X_SMALLSTR_MAX_LENGTH);
    s->buf[len] = L'\0';
    s->length = len;
    return 0;
  }

  X_STRING_API int32_t x_wsmallstr_cmp(const XWSmallstr* a, const XWSmallstr* b)
  {
    return wcsncmp(a->buf, b->buf, a->length);
  }

  X_STRING_API int32_t x_wsmallstr_cmp_cstr(const XWSmallstr* a, const wchar_t* b)
  {
    return wcsncmp(a->buf, b, wcslen(b));
  }

  X_STRING_API int32_t x_wsmallstr_cmp_case(const XWSmallstr* a, const XWSmallstr* b)
  {
    if (a->length != b->length) return 1;
    for (size_t i = 0; i < a->length; ++i)
    {
      if (towlower(a->buf[i]) != towlower(b->buf[i])) return 1;
    }
    return 0;
  }

  X_STRING_API size_t x_wsmallstr_length(const XWSmallstr* s)
  {
    return s->length;
  }

  X_STRING_API void x_wsmallstr_clear(XWSmallstr* s)
  {
    s->length = 0;
    s->buf[0] = L'\0';
  }

  X_STRING_API int32_t x_wsmallstr_append(XWSmallstr* s, const wchar_t* tail)
  {
    size_t tail_len = wcslen(tail);
    if (s->length + tail_len > X_SMALLSTR_MAX_LENGTH)
      return -1;

    wcsncpy(&s->buf[s->length], tail, tail_len);
    s->length += tail_len;
    s->buf[s->length] = L'\0';
    return 0;
  }

  X_STRING_API int32_t x_wsmallstr_substring(const XWSmallstr* s, size_t start, size_t len, XWSmallstr* out)
  {
    if (start > s->length || (start + len) > s->length)
      return -1;

    wcsncpy(out->buf, &s->buf[start], len);
    out->buf[len] = L'\0';
    out->length = len;
    return 0;
  }

  X_STRING_API void x_wsmallstr_trim(XWSmallstr* s)
  {
    size_t start = 0;
    size_t end = s->length;

    // Find first non-space
    while (start < end && iswspace(s->buf[start]))
      start++;

    // Find last non-space
    while (end > start && iswspace(s->buf[end - 1]))
      end--;

    size_t new_len = end - start;

    if (start > 0 && new_len > 0)
      wmemmove(s->buf, s->buf + start, new_len);

    s->length = new_len;
    s->buf[new_len] = L'\0';
  }

  X_STRING_API int32_t x_wsmallstr_find(const XWSmallstr* s, wchar_t c)
  {
    for (size_t i = 0; i < s->length; ++i)
    {
      if (s->buf[i] == c)
        return (int32_t)i;
    }
    return -1;
  }

  X_STRING_API int32_t x_wsmallstr_next_token(XWSmallstr* input, wchar_t delim, XWSmallstr* token)
  {
    XWSmallstr temp;
    int32_t found = 0;
    int32_t pos = x_wsmallstr_find(input, delim);
    if (pos >= 0)
    {
      x_wsmallstr_substring(input, 0, pos, token);
      x_wsmallstr_substring(input, pos + 1, input->length - pos - 1, &temp);
      *input = temp;
      found = 1;
    }
    else if (input->length > 0)
    {
      *token = *input;
      x_wsmallstr_clear(input);
      found = 1;
    }
    return found;
  }

  X_STRING_API bool x_strview_eq(XStrview a, XStrview b)
  {
    return a.length == b.length && (memcmp(a.data, b.data, a.length) == 0);
  }

  X_STRING_API bool x_strview_eq_cstr(XStrview a, const char* b)
  {
    return x_strview_eq(a, x_strview(b));
  }

  X_STRING_API int32_t x_strview_cmp(XStrview a, XStrview b)
  {
    size_t min_len = a.length < b.length ? a.length : b.length;
    int32_t r = memcmp(a.data, b.data, min_len);
    if (r != 0) return r;
    return (int)(a.length - b.length);
  }

  X_STRING_API bool x_strview_eq_case(XStrview a, XStrview b)
  {
    if (a.length != b.length) return 0;
    for (size_t i = 0; i < a.length; i++)
    {
      if (tolower((unsigned char)a.data[i]) != tolower((unsigned char)b.data[i]))
        return false;
    }
    return true;
  }

  X_STRING_API int32_t x_strview_cmp_case(XStrview a, XStrview b)
  {
    size_t min_len = a.length < b.length ? a.length : b.length;
    for (size_t i = 0; i < min_len; i++)
    {
      int32_t ca = tolower((unsigned char)a.data[i]);
      int32_t cb = tolower((unsigned char)b.data[i]);
      if (ca != cb) return ca - cb;
    }
    return (int)(a.length - b.length);
  }

  X_STRING_API XStrview x_strview_substr(XStrview sv, size_t start, size_t len)
  {
    if (start > sv.length) start = sv.length;
    if (start + len > sv.length) len = sv.length - start;
    return x_strview_init(sv.data + start, len);
  }

  X_STRING_API XStrview x_strview_trim_left(XStrview sv)
  {
    size_t i = 0;
    while (i < sv.length && (unsigned char)sv.data[i] <= ' ') i++;
    return x_strview_substr(sv, i, sv.length - i);
  }

  X_STRING_API XStrview x_strview_trim_right(XStrview sv)
  {
    size_t i = sv.length;
    while (i > 0 && (unsigned char)sv.data[i - 1] <= ' ') i--;
    return x_strview_substr(sv, 0, i);
  }

  X_STRING_API XStrview x_strview_trim(XStrview sv)
  {
    return x_strview_trim_right(x_strview_trim_left(sv));
  }

  X_STRING_API int32_t x_strview_find(XStrview sv, char c)
  {
    for (size_t i = 0; i < sv.length; i++)
    {
      if (sv.data[i] == c) return (int)i;
    }
    return -1;
  }

  X_STRING_API int32_t x_strview_find_white_space(XStrview sv)
  {
    for (size_t i = 0; i < sv.length; i++)
    {
      if ( sv.data[i] == ' ' ||
          sv.data[i] == '\t' ||
          sv.data[i] == '\r'
         ) return (int)i;
    }
    return -1;
  }

  X_STRING_API int32_t x_strview_rfind(XStrview sv, char c)
  {
    for (size_t i = sv.length; i > 0; i--)
    {
      if (sv.data[i - 1] == c) return (int)(i - 1);
    }
    return -1;
  }

  X_STRING_API bool x_strview_split_at(XStrview sv, char delim, XStrview* left, XStrview* right)
  {
    int32_t pos = x_strview_find(sv, delim);
    if (pos < 0) return false;
    if (left) *left = x_strview_substr(sv, 0, pos);
    if (right) *right = x_strview_substr(sv, pos + 1, sv.length - pos - 1);
    return true;
  }

  X_STRING_API bool x_strview_split_at_white_space(XStrview sv, XStrview* left, XStrview* right)
  {
    int32_t pos = x_strview_find_white_space(sv);
    if (pos < 0) return false;
    if (left) *left = x_strview_substr(sv, 0, pos);
    if (right) *right = x_strview_substr(sv, pos + 1, sv.length - pos - 1);
    return true;
  }

  X_STRING_API bool x_strview_next_token_white_space(XStrview* input, XStrview* token)
  {
    XStrview rest;
    if (x_strview_split_at_white_space(*input, token, &rest))
    {
      *input = rest;
      return true;
    }
    else if (input->length > 0)
    {
      *token = *input;
      *input = (XStrview){0};
      return true;
    }

    token->data = NULL;
    token->length = 0;
    return false;
  }

  X_STRING_API bool x_strview_next_token(XStrview* input, char delim, XStrview* token)
  {
    XStrview rest;
    if (x_strview_split_at(*input, delim, token, &rest))
    {
      *input = rest;
      return true;
    }
    else if (input->length > 0)
    {
      *token = *input;
      *input = (XStrview){0};
      return true;
    }

    token->data = NULL;
    token->length = 0;
    return false;
  }

  X_STRING_API bool x_strview_starts_with_cstr(XStrview sv, const char* prefix)
  {
    if (prefix == NULL || sv.length == 0 || sv.data == NULL)
      return false;
    size_t prefix_len = strlen(prefix);
    if (prefix_len > sv.length)
      return false;

    return strncmp(sv.data, prefix, prefix_len) == 0;
  }

  X_STRING_API bool x_strview_ends_with_cstr(XStrview sv, const char* suffix)
  {
    if (suffix == NULL || sv.length == 0 || sv.data == NULL)
      return false;
    size_t suffix_len = strlen(suffix);
    if (suffix_len > sv.length)
      return false;

    return strncmp(sv.data + sv.length - suffix_len, suffix, suffix_len) == 0;
  }

  X_STRING_API bool x_wstrview_empty(XWStrview sv)
  {
    return sv.length == 0;
  }

  X_STRING_API bool x_wstrview_eq(XWStrview a, XWStrview b)
  {
    return a.length == b.length && wcsncmp(a.data, b.data, a.length) == 0;
  }

  X_STRING_API int32_t x_wstrview_cmp(XWStrview a, XWStrview b)
  {
    size_t min = a.length < b.length ? a.length : b.length;
    int32_t result = wcsncmp(a.data, b.data, min);
    return result != 0 ? result : (int)(a.length - b.length);
  }

  X_STRING_API XWStrview x_wstrview_substr(XWStrview sv, size_t start, size_t len)
  {
    if (start > sv.length) start = sv.length;
    if (start + len > sv.length) len = sv.length - start;
    return (XWStrview){ sv.data + start, len };
  }

  X_STRING_API XWStrview x_wstrview_trim_left(XWStrview sv)
  {
    size_t i = 0;
    while (i < sv.length && iswspace(sv.data[i])) i++;
    return x_wstrview_substr(sv, i, sv.length - i);
  }

  X_STRING_API XWStrview x_wstrview_trim_right(XWStrview sv)
  {
    size_t i = sv.length;
    while (i > 0 && iswspace(sv.data[i - 1])) i--;
    return x_wstrview_substr(sv, 0, i);
  }

  X_STRING_API XWStrview x_wstrview_trim(XWStrview sv)
  {
    return x_wstrview_trim_right(x_wstrview_trim_left(sv));
  }

  X_STRING_API bool x_wstrview_split_at(XWStrview sv, uint32_t delim, XWStrview* left, XWStrview* right)
  {
    for (size_t i = 0; i < sv.length; ++i)
    {
      if ((uint32_t)sv.data[i] == delim)
      {
        if (left) {
          left->data = sv.data;
          left->length = i;
        }
        if (right)
        {
          right->data = sv.data + i + 1;
          right->length = sv.length - i - 1;
        }
        return true;
      }
    }
    return false;
  }

  X_STRING_API bool x_wstrview_next_token(XWStrview* input, wchar_t delim, XWStrview* token)
  {
    XWStrview left, right;
    if (x_wstrview_split_at(*input, (uint32_t)delim, &left, &right))
    {
      *token = left;
      *input = right;
      return true;
    }
    else if (input->length > 0)
    {
      *token = *input;
      input->data += input->length;
      input->length = 0;
      return true;
    }

    token->data = NULL;
    token->length = 0;
    return false;
  }

  X_STRING_API XStrview x_strview_utf8_substr(XStrview sv, size_t char_start, size_t char_len)
  {
    size_t byte_start = utf8_advance(sv.data, sv.length, char_start);
    size_t byte_end = utf8_advance(sv.data + byte_start, sv.length - byte_start, char_len);
    return x_strview_init(sv.data + byte_start, byte_end );
  }

  X_STRING_API XStrview x_strview_utf8_trim_left(XStrview sv)
  {
    const char* start = sv.data;
    const char* end = sv.data + sv.length;
    while (start < end)
    {
      const char* prev = start;
      uint32_t cp = utf8_decode(&start, end);
      if (!s_is_unicode_whitespace(cp))
      {
        return (XStrview){ prev, (size_t)(end - prev) };
      }
    }
    return x_strview_init(end, 0);
  }

  X_STRING_API XStrview x_strview_utf8_trim_right(XStrview sv)
  {
    const char* start = sv.data;
    const char* end = sv.data + sv.length;
    const char* p = end;
    while (p > start)
    {
      const char* prev = p;
      // Step back 1 byte to guess start of char
      do { --p; } while (p > start && ((*p & 0xC0) == 0x80));
      const char* decode_from = p;
      uint32_t cp = utf8_decode(&decode_from, end);
      if (!s_is_unicode_whitespace(cp))
      {
        return x_strview_init(sv.data, (size_t)(prev - sv.data));
      }
    }
    return x_strview_init(sv.data + sv.length, 0 );
  }

  X_STRING_API XStrview x_strview_utf8_trim(XStrview sv)
  {
    return x_strview_utf8_trim_right(x_strview_utf8_trim_left(sv));
  }

  X_STRING_API int32_t x_strview_utf8_find(XStrview sv, uint32_t codepoint)
  {
    const char* ptr = sv.data;
    const char* end = sv.data + sv.length;

    while (ptr < end)
    {
      const char* start = ptr;
      uint32_t cp = utf8_decode(&ptr, end);
      if (cp == codepoint)
        return (int)(start - sv.data);
    }
    return -1;
  }

  X_STRING_API int32_t x_strview_utf8_rfind(XStrview sv, uint32_t codepoint)
  {
    const char* ptr = sv.data;
    const char* end = sv.data + sv.length;
    const char* last_match = NULL;

    while (ptr < end)
    {
      const char* current = ptr;
      uint32_t cp = utf8_decode(&ptr, end);
      if (cp == codepoint)
        last_match = current;
    }

    return last_match ? (int)(last_match - sv.data) : -1;
  }

  X_STRING_API bool x_strview_utf8_split_at(XStrview sv, uint32_t delim, XStrview* left, XStrview* right)
  {
    const char* ptr = sv.data;
    const char* end = sv.data + sv.length;

    while (ptr < end) {
      const char* codepoint_start = ptr;
      int32_t cp = utf8_decode(&ptr, end);

      if (cp == (int32_t)delim) {
        size_t left_len = codepoint_start - sv.data;
        size_t right_len = end - ptr;
        *left  = x_strview_init(sv.data, left_len);
        *right = x_strview_init(ptr, right_len);
        return true;
      }
    }

    return false;
  }

  X_STRING_API bool x_strview_utf8_next_token(XStrview* input, uint32_t delim, XStrview* token)
  {
    XStrview rest;
    if (x_strview_utf8_split_at(*input, delim, token, &rest)) {
      *input = rest;
      return true;
    }
    if (input->length > 0) {
      *token = *input;
      *input = (XStrview){0};
      return true;
    }

    token->data = NULL;
    token->length = 0;
    return false;
  }

  X_STRING_API bool x_strview_utf8_starts_with_cstr(XStrview sv, const char* prefix)
  {
    if (!sv.data || !prefix) return false;
    size_t prefix_len = strlen(prefix);
    if (prefix_len > sv.length) return false;
    return memcmp(sv.data, prefix, prefix_len) == 0;
  }

  X_STRING_API bool x_strview_utf8_ends_with_cstr(XStrview sv, const char* suffix)
  {
    if (!sv.data || !suffix) return false;
    size_t suffix_len = strlen(suffix);
    if (suffix_len > sv.length) return false;
    return memcmp(sv.data + sv.length - suffix_len, suffix, suffix_len) == 0;
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_STRING
#endif // X_STRING_H

