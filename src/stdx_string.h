/*
 * STDX - Lightweight String Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * This module provides efficient string handling tools for C:
 *   - C string helpers: case-insensitive prefix/suffix matching
 *   - XSmallstr: fixed-capacity, stack-allocated strings
 *   - XStrview: immutable, non-owning views into C strings
 *   - Tokenization and trimming
 *   - UTF-8-aware string length calculation
 *   - Fast substring and search operations
 *   - Case-sensitive and case-insensitive comparisons
 *
 * Useful for environments where dynamic memory is avoided.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_STRING
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "stdx_string.h"
 */
#ifndef STDX_STRING_H
#define STDX_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif

#define STDX_STRING_VERSION_MAJOR 1
#define STDX_STRING_VERSION_MINOR 0
#define STDX_STRING_VERSION_PATCH 0

#define STDX_STRING_VERSION (STDX_STRING_VERSION_MAJOR * 10000 + STDX_STRING_VERSION_MINOR * 100 + STDX_STRING_VERSION_PATCH)

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef STDX_SMALLSTR_MAX_LENGTH
#define STDX_SMALLSTR_MAX_LENGTH 256
#endif

  typedef struct 
  {
    const int8_t* data;
    size_t length;
  } XStrview;

  typedef struct
  {
    const wchar_t* data;
    size_t length; // In wide characters
  } XWStrview;

  typedef struct 
  {
    int8_t buf[STDX_SMALLSTR_MAX_LENGTH + 1];
    size_t length;
  } XSmallstr;

  typedef struct
  {
    wchar_t buf[STDX_SMALLSTR_MAX_LENGTH + 1];
    size_t length; // In wide characters
  } XWSmallstr;

  typedef struct 
  {
    const XSmallstr* s;
    size_t pos;
    int8_t delimiter;
  } XSmallstrTokenIterator;

  typedef struct
  {
    const XWSmallstr* s;
    size_t pos;
    wchar_t delimiter;
  } XWSmallstrTokenIterator;


  // ---------------------------------------------------------------------------
  // C string utilities
  // ---------------------------------------------------------------------------

  int8_t*     x_cstr_str(const int8_t *haystack, const int8_t *needle);
  bool      x_cstr_ends_with(const int8_t* str, const int8_t* suffix);
  bool      x_cstr_starts_with(const int8_t* str, const int8_t* prefix);
  bool      x_cstr_ends_with_ci(const int8_t* str, const int8_t* suffix);
  bool      x_cstr_starts_with_ci(const int8_t* str, const int8_t* prefix);

  bool      x_wcstr_starts_with(const wchar_t* str, const wchar_t* prefix);
  bool      x_wcstr_ends_with(const wchar_t* str, const wchar_t* suffix);
  bool      x_wcstr_starts_with_ci(const wchar_t* str, const wchar_t* prefix);
  bool      x_wcstr_ends_with_ci(const wchar_t* str, const wchar_t* suffix);
  int       x_wcstr_casecmp(const wchar_t* a, const wchar_t* b);

  size_t    x_cstr_to_wcstr(const int8_t* src, wchar_t* dest, size_t max);
  size_t    x_wcstr_to_utf8(const wchar_t* wide, int8_t* utf8, size_t max);
  size_t    x_wcstr_to_cstr(const wchar_t* src, int8_t* dest, size_t max);
  size_t    x_utf8_to_wcstr(const int8_t* utf8, wchar_t* wide, size_t max);

  size_t    x_utf8_strlen(const int8_t* utf8);
  int       x_utf8_strcmp(const int8_t* a, const int8_t* b);
  int8_t*     x_utf8_tolower(int8_t* dest, const int8_t* src);
  int8_t*     x_utf8_toupper(int8_t* dest, const int8_t* src);


  // ---------------------------------------------------------------------------
  // XSmallstr
  // ---------------------------------------------------------------------------

  int       x_smallstr_init(XSmallstr* s, const int8_t* str);
  size_t    x_smallstr_format(XSmallstr* s, const int8_t* fmt, ...);
  size_t    x_smallstr_from_cstr(XSmallstr* s, const int8_t* cstr);
  size_t    x_smallstr_from_strview(XStrview sv, XSmallstr* out);
  XStrview  x_smallstr_to_strview(const XSmallstr* s);
  size_t    x_smallstr_length(const XSmallstr* s);
  void      x_smallstr_clear(XSmallstr* s);
  size_t    x_smallstr_append_cstr(XSmallstr* s, const int8_t* cstr);
  size_t    x_smallstr_append_char(XSmallstr* s, int8_t c);
  size_t    x_smallstr_substring(const XSmallstr* s, size_t start, size_t len, XSmallstr* out);
  int       x_smallstr_find(const XSmallstr* s, int8_t c);
  int       x_smallstr_rfind(const XSmallstr* s, int8_t c);
  int       x_smallstr_split_at(const XSmallstr* s, int8_t delim, XSmallstr* left, XSmallstr* right);
  int       x_smallstr_next_token(XSmallstr* input, int8_t delim, XSmallstr* token);
  void      x_smallstr_trim_left(XSmallstr* s);
  void      x_smallstr_trim_right(XSmallstr* s);
  void      x_smallstr_trim(XSmallstr* s);
  int       x_smallstr_compare_case_insensitive(const XSmallstr* a, const XSmallstr* b);
  int       x_smallstr_replace_all(XSmallstr* s, const int8_t* find, const int8_t* replace);
  void      x_smallstr_token_iter_init(XSmallstrTokenIterator* iter, const XSmallstr* s, int8_t delimiter);
  int       x_smallstr_token_iter_next(XSmallstrTokenIterator* iter, XSmallstr* token);
  size_t    x_smallstr_utf8_len(const XSmallstr* s);
  int       x_smallstr_cmp(const XSmallstr* a, const XSmallstr* b);
  int       x_smallstr_cmp_cstr(const XSmallstr* a, const int8_t* b);
  const int8_t* x_smallstr_cstr(const XSmallstr* s);

  int       x_smallstr_utf8_substring(const XSmallstr* s, size_t start_cp, size_t len_cp, XSmallstr* out);
  void      x_smallstr_utf8_trim_left(XSmallstr* s);
  void      x_smallstr_utf8_trim_right(XSmallstr* s);
  void      x_smallstr_utf8_trim(XSmallstr* s);
  int       x_wsmallstr_from_wcstr(XWSmallstr* s, const wchar_t* src);
  int       x_wsmallstr_cmp(const XWSmallstr* a, const XWSmallstr* b);
  int       x_wsmallstr_casecmp(const XWSmallstr* a, const XWSmallstr* b);
  size_t    x_wsmallstr_len(const XWSmallstr* s);
  void      x_wsmallstr_clear(XWSmallstr* s);
  int       x_wsmallstr_append(XWSmallstr* s, const wchar_t* tail);
  int       x_wsmallstr_substring(const XWSmallstr* s, size_t start, size_t len, XWSmallstr* out);
  void      x_wsmallstr_trim(XWSmallstr* s);
  void      x_wsmallstr_token_iter_init(XWSmallstrTokenIterator* iter, const XWSmallstr* s, wchar_t delimiter);
  int       x_wsmallstr_token_iter_next(XWSmallstrTokenIterator* iter, XWSmallstr* token);

  // ---------------------------------------------------------------------------
  // Stringview
  // ---------------------------------------------------------------------------

#define     x_strview(cstr) ((XStrview){ .data = (cstr), .length = strlen(cstr) })

  bool      x_strview_empty(XStrview sv);
  bool      x_strview_eq(XStrview a, XStrview b);
  bool      x_strview_eq_cstr(XStrview a, const int8_t* b);
  int       x_strview_cmp(XStrview a, XStrview b);
  bool      x_strview_case_eq(XStrview a, XStrview b);
  int       x_strview_case_cmp(XStrview a, XStrview b);
  XStrview  x_strview_substr(XStrview sv, size_t start, size_t len);
  XStrview  x_strview_trim_left(XStrview sv);
  XStrview  x_strview_trim_right(XStrview sv);
  XStrview  x_strview_trim(XStrview sv);
  int       x_strview_find(XStrview sv, int8_t c);
  int       x_strview_rfind(XStrview sv, int8_t c);
  bool      x_strview_split_at(XStrview sv, int8_t delim, XStrview* left, XStrview* right);
  bool      x_strview_next_token(XStrview* input, int8_t delim, XStrview* token);

#define     x_wstrview(wcs) ((XWStrview){ .data = (wcs), .length = wcslen(wcs) })

  // Basics
  bool      x_wstrview_empty(XWStrview sv);
  bool      x_wstrview_eq(XWStrview a, XWStrview b);
  int       x_wstrview_cmp(XWStrview a, XWStrview b);
  XWStrview x_wstrview_substr(XWStrview sv, size_t start, size_t len);
  XWStrview x_wstrview_trim_left(XWStrview sv);
  XWStrview x_wstrview_trim_right(XWStrview sv);
  XWStrview x_wstrview_trim(XWStrview sv);
  XStrview  x_strview_utf8_substr(XStrview sv, size_t char_start, size_t char_len);
  XStrview  x_strview_utf8_trim_left(XStrview sv);
  XStrview  x_strview_utf8_trim_right(XStrview sv);
  XStrview  x_strview_utf8_trim(XStrview sv);

#ifdef STDX_IMPLEMENTATION_STRING

#include <ctype.h>   // tolower
#include <stddef.h>  // size_t
#include <string.h>  // strlen, memcmp, strncasecmp (POSIX)
#include <ctype.h>   // tolower
#include <stdint.h>
#include <wchar.h>
#include <wctype.h>
#include <wchar.h>
#include <locale.h>
#include <errno.h>

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

  static bool is_unicode_whitespace(uint32_t cp)
  {
    return (cp == 0x09 || cp == 0x0A || cp == 0x0B || cp == 0x0C || cp == 0x0D || cp == 0x20 ||
        cp == 0x85 || cp == 0xA0 || cp == 0x1680 ||
        (cp >= 0x2000 && cp <= 0x200A) ||
        cp == 0x2028 || cp == 0x2029 || cp == 0x202F || cp == 0x205F || cp == 0x3000);
  }

  static uint32_t utf8_decode(const char** s, const int8_t* end)
  {
    const uint8_t* p = (const uint8_t*)(*s);
    if (p >= (const unsigned char*)end) return 0;

    uint32_t cp = 0;
    if (*p < 0x80)
    {
      cp = *p++;
    } else if ((*p & 0xE0) == 0xC0) {
      cp = ((*p & 0x1F) << 6) | (p[1] & 0x3F); p += 2;
    } else if ((*p & 0xF0) == 0xE0) {
      cp = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3;
    } else if ((*p & 0xF8) == 0xF0) {
      cp = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); p += 4;
    } else {
      p++; // Invalid byte, skip
    }
    *s = (const char*)p;
    return cp;
  }

  static size_t utf8_advance(const int8_t* s, size_t len, size_t chars)
  {
    size_t i = 0, count = 0;
    while (i < len && count < chars)
    {
      uint8_t c = (uint8_t)s[i];
      if ((c & 0x80) == 0x00) i += 1;
      else if ((c & 0xE0) == 0xC0) i += 2;
      else if ((c & 0xF0) == 0xE0) i += 3;
      else if ((c & 0xF8) == 0xF0) i += 4;
      else break;
      count++;
    }
    return i;
  }

  int8_t* x_cstr_str(const int8_t *haystack, const int8_t *needle)
  {
    if (!*needle)
      return (int8_t *)haystack;

    for (; *haystack; haystack++)
    {
      const int8_t *h = haystack;
      const int8_t *n = needle;

      while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n))

      {
        h++;
        n++;
      }

      if (!*n)  // Reached end of needle => match
        return (int8_t *)haystack;
    }
    return NULL;
  }

  bool x_cstr_ends_with(const int8_t* str, const int8_t* suffix)
  {
    const int8_t* found = strstr(str, suffix);
    if (found == NULL || found + strlen(suffix) != str + strlen(str))


    {
      return false;
    }
    return true;
  }

  bool x_cstr_starts_with(const int8_t* str, const int8_t* prefix)
  {
    const int8_t* found = strstr(str, prefix);
    bool match = (found == str);
    if (match && prefix[0] == 0 && str[0] != 0)
      match = false;

    return match;
  }

  bool x_cstr_starts_with_ci(const int8_t* str, const int8_t* prefix)
  {
    const int8_t* found = x_cstr_str(str, prefix);
    bool match = (found == str);
    if (match && prefix[0] == 0 && str[0] != 0)
      match = false;

    return match;
  }

  bool x_wcstr_starts_with(const wchar_t* str, const wchar_t* prefix)
  {
    size_t plen = wcslen(prefix);
    return wcsncmp(str, prefix, plen) == 0;
  }

  bool x_wcstr_ends_with(const wchar_t* str, const wchar_t* suffix)
  {
    size_t slen = wcslen(str);
    size_t suf_len = wcslen(suffix);
    if (slen < suf_len) return false;
    return wcscmp(str + slen - suf_len, suffix) == 0;
  }

  bool x_wcstr_starts_with_ci(const wchar_t* str, const wchar_t* prefix)
  {
    while (*prefix && *str)

    {
      if (towlower(*str++) != towlower(*prefix++))
        return false;
    }
    return *prefix == 0;
  }

  bool x_wcstr_ends_with_ci(const wchar_t* str, const wchar_t* suffix)
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

  int x_wcstr_casecmp(const wchar_t* a, const wchar_t* b)
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
  size_t x_cstr_to_wcstr(const int8_t* src, wchar_t* dest, size_t max)
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

  size_t x_wcstr_to_utf8(const wchar_t* wide, int8_t* utf8, size_t max)
  {
    mbstate_t ps =
    {0};
    return wcsrtombs(utf8, (const wchar_t**)&wide, max, &ps);
  }

  size_t x_wcstr_to_cstr(const wchar_t* src, int8_t* dest, size_t max)
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

  bool x_cstr_ends_with_ci(const int8_t* str, const int8_t* suffix)
  {
    const int8_t* found = x_cstr_str(str, suffix);
    if (found == NULL || found + strlen(suffix) != str + strlen(str))

    {
      return false;
    }
    return true;
  }

  void x_utf8_set_locale(void)
  {
    // Set locale once at startup (user responsibility)
    setlocale(LC_ALL, "");
  }

  size_t x_utf8_strlen(const int8_t* utf8)
  {
    size_t count = 0;
    while (*utf8)
    {
      uint8_t c = (uint8_t)*utf8;
      if ((c & 0x80) == 0x00) utf8 += 1;
      else if ((c & 0xE0) == 0xC0) utf8 += 2;
      else if ((c & 0xF0) == 0xE0) utf8 += 3;
      else if ((c & 0xF8) == 0xF0) utf8 += 4;
      else break;
      count++;
    }
    return count;
  }

  int x_utf8_strcmp(const int8_t* a, const int8_t* b)
  {
    if (setlocale(LC_ALL, NULL) != NULL) return strcoll(a, b);
    return strcmp(a, b); // no locale set, fallback to pure ascii comparisons
  }

  int8_t* x_utf8_tolower(int8_t* dest, const int8_t* src)
  {
    wchar_t wbuf[512];
    x_utf8_to_wcstr(src, wbuf, 512);
    for (size_t i = 0; wbuf[i]; ++i)
      wbuf[i] = towlower(wbuf[i]);
    x_wcstr_to_utf8(wbuf, dest, 512);
    return dest;
  }

  int8_t* x_utf8_toupper(int8_t* dest, const int8_t* src)
  {
    wchar_t wbuf[512];
    x_utf8_to_wcstr(src, wbuf, 512);
    for (size_t i = 0; wbuf[i]; ++i)
      wbuf[i] = towupper(wbuf[i]);
    x_wcstr_to_utf8(wbuf, dest, 512);
    return dest;
  }

  size_t x_utf8_to_wcstr(const int8_t* utf8, wchar_t* wide, size_t max)
  {
    mbstate_t ps =
    {0};
    return mbsrtowcs(wide, &utf8, max, &ps);
  }

  size_t x_wide_to_utf8(const wchar_t* wide, int8_t* utf8, size_t max)
  {
    mbstate_t ps =
    {0};
    return wcsrtombs(utf8, (const wchar_t**)&wide, max, &ps);
  }

  unsigned int str_hash(const int8_t* str)
  {
    unsigned int hash = 2166136261u; // FNV offset basis
    while (*str)

    {
      hash ^= (unsigned int) *str++;
      hash *= 16777619u; // FNV prime
    }
    return hash;
  }

  size_t smallstr(XSmallstr* smallString, const int8_t* str)
  {
    size_t length = strlen(str);
#if defined(_DEBUG) || defined(DEBUG)
#if defined(STDX_x_smallstr_ENABLE_DBG_MESSAGES)
    if (length >= STDX_SMALLSTR_MAX_LENGTH)

    {
      log_print_debug("The string length (%d) exceeds the maximum size of a XSmallstr (%d)", length, STDX_SMALLSTR_MAX_LENGTH);
    }
#endif
#endif
    strncpy((int8_t *) &smallString->buf, str, STDX_SMALLSTR_MAX_LENGTH - 1);
    smallString->buf[STDX_SMALLSTR_MAX_LENGTH - 1] = 0;
    smallString->length = strlen(smallString->buf);
    return smallString->length;
  }

  size_t x_smallstr_format(XSmallstr* smallString, const int8_t* fmt, ...)
  {
    va_list argList;
    va_start(argList, fmt);
    smallString->length = vsnprintf((char*) &smallString->buf, STDX_SMALLSTR_MAX_LENGTH-1, fmt, argList);
    va_end(argList);

    bool success = smallString->length > 0 && smallString->length < STDX_SMALLSTR_MAX_LENGTH;
    return success;
  }

  size_t x_smallstr_from_cstr(XSmallstr* s, const int8_t* cstr)
  {
    size_t len = strlen(cstr);
    if (len > STDX_SMALLSTR_MAX_LENGTH)
      return -1;
    memcpy(s->buf, cstr, len);
    s->buf[len] = '\0';
    s->length = len;
    return len;
  }

  const int8_t* x_smallstr_cstr(const XSmallstr* s)
  {
    ((char*)s->buf)[s->length] = '\0';
    return (const char*)s->buf;
  }

  size_t x_smallstr_length(const XSmallstr* smallString)
  {
    return smallString->length;
  }

  void x_smallstr_clear(XSmallstr* smallString)
  {
    memset(smallString->buf, 0, STDX_SMALLSTR_MAX_LENGTH * sizeof(char));
    smallString->length = 0;
  }

  size_t x_smallstr_append_cstr(XSmallstr* s, const int8_t* cstr)
  {
    size_t len = strlen(cstr);
    if (s->length + len > STDX_SMALLSTR_MAX_LENGTH) return -1;
    memcpy(&s->buf[s->length], cstr, len);
    s->length += len;
    s->buf[s->length] = '\0';
    return s->length;
  }

  size_t x_smallstr_append_char(XSmallstr* s, int8_t c)
  {
    if (s->length + 1 > STDX_SMALLSTR_MAX_LENGTH) return -1;
    s->buf[s->length++] = (uint8_t)c;
    s->buf[s->length] = '\0';
    return s->length;
  }

  size_t x_smallstr_substring(const XSmallstr* s, size_t start, size_t len, XSmallstr* out)
  {
    if (start > s->length || start + len > s->length) return -1;
    out->length = len;
    memcpy(out->buf, &s->buf[start], len);
    out->buf[len] = '\0';
    return len;
  }

  void x_smallstr_trim_left(XSmallstr* s)
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

  void x_smallstr_trim_right(XSmallstr* s)
  {
    while (s->length > 0 && isspace(s->buf[s->length - 1]))

    {
      s->length--;
    }
    s->buf[s->length] = '\0';
  }

  void x_smallstr_trim(XSmallstr* s)
  {
    x_smallstr_trim_right(s);
    x_smallstr_trim_left(s);
  }

  int x_smallstr_compare_case_insensitive(const XSmallstr* a, const XSmallstr* b)
  {
    if (a->length != b->length) return 0;
    for (size_t i = 0; i < a->length; i++)

    {
      if (tolower(a->buf[i]) != tolower(b->buf[i])) return 0;
    }
    return 1;
  }

  int x_smallstr_replace_all(XSmallstr* s, const int8_t* find, const int8_t* replace)
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
        if (result.length + replace_len > STDX_SMALLSTR_MAX_LENGTH) return -1;
        memcpy(&result.buf[result.length], replace, replace_len);
        result.length += replace_len;
        i += find_len;
      } else

      {
        if (result.length + 1 > STDX_SMALLSTR_MAX_LENGTH) return -1;
        result.buf[result.length++] = s->buf[i++];
      }
    }

    result.buf[result.length] = '\0';
    *s = result;
    return 0;
  }

  void x_smallstr_token_iter_init(XSmallstrTokenIterator* iter, const XSmallstr* s, int8_t delimiter)
  {
    iter->s = s;
    iter->pos = 0;
    iter->delimiter = delimiter;
  }

  int x_smallstr_token_iter_next(XSmallstrTokenIterator* iter, XSmallstr* token)
  {
    if (iter->pos >= iter->s->length) return 0;

    size_t start = iter->pos;
    while (iter->pos < iter->s->length && iter->s->buf[iter->pos] != (uint8_t)iter->delimiter)

    {
      iter->pos++;
    }

    x_smallstr_substring(iter->s, start, iter->pos - start, token);

    if (iter->pos < iter->s->length) iter->pos++; // Skip delimiter

    return 1;
  }

  size_t x_smallstr_utf8_len(const XSmallstr* s)
  {
    size_t count = 0;
    for (size_t i = 0; i < s->length;)

    {
      uint8_t c = s->buf[i];
      if ((c & 0x80) == 0x00) i += 1;
      else if ((c & 0xE0) == 0xC0) i += 2;
      else if ((c & 0xF0) == 0xE0) i += 3;
      else if ((c & 0xF8) == 0xF0) i += 4;
      else return count; // invalid byte
      count++;
    }
    return count;
  }

  size_t x_smallstr_from_strview(XStrview sv, XSmallstr* out)
  {
    x_smallstr_clear(out);
    if (sv.length > STDX_SMALLSTR_MAX_LENGTH) return -1;
    memcpy(out->buf, sv.data, sv.length);
    out->buf[sv.length] = '\0';
    out->length = sv.length;
    return out->length;
  }

  int x_smallstr_cmp(const XSmallstr* a, const XSmallstr* b)
  {
    return strncmp(a->buf, b->buf, b->length);
  }

  int x_smallstr_cmp_cstr(const XSmallstr* a, const int8_t* b)
  {
    return strncmp(a->buf, b, a->length);
  }

  int x_smallstr_find(const XSmallstr* s, int8_t c)
  {
    for (size_t i = 0; i < s->length; ++i)

    {
      if (s->buf[i] == c) return (int)i;
    }
    return -1;
  }

  int x_smallstr_rfind(const XSmallstr* s, int8_t c)
  {
    for (size_t i = s->length; i > 0; --i)

    {
      if (s->buf[i - 1] == c) return (int)(i - 1);
    }
    return -1;
  }

  int x_smallstr_split_at(const XSmallstr* s, int8_t delim, XSmallstr* left, XSmallstr* right)
  {
    int pos = x_smallstr_find(s, delim);
    if (pos < 0) return 0;
    x_smallstr_substring(s, 0, pos, left);
    x_smallstr_substring(s, pos + 1, s->length - pos - 1, right);
    return 1;
  }

  int x_smallstr_next_token(XSmallstr* input, int8_t delim, XSmallstr* token)
  {
    XSmallstr temp;
    int found = 0;
    int pos = x_smallstr_find(input, delim);
    if (pos >= 0)

    {
      x_smallstr_substring(input, 0, pos, token);
      x_smallstr_substring(input, pos + 1, input->length - pos - 1, &temp);
      *input = temp;
      found = 1;
    }
    else if (input->length > 0)

    {
      *token = *input;
      x_smallstr_clear(input);
      found = 1;
    }
    return found;
  }

  XStrview x_smallstr_to_strview(const XSmallstr* s)
  {
    return (XStrview){ s->buf, s->length };
  }

  int x_smallstr_utf8_substring(const XSmallstr* s, size_t start_cp, size_t len_cp, XSmallstr* out)
  {
    size_t start = utf8_advance(s->buf, s->length, start_cp);
    size_t len = utf8_advance(s->buf + start, s->length - start, len_cp);
    if (start + len > s->length) return -1;

    out->length = len;
    memcpy(out->buf, s->buf + start, len);
    out->buf[len] = '\0';
    return 0;
  }

  void x_smallstr_utf8_trim_left(XSmallstr* s)
  {
    const int8_t* start = s->buf;
    const int8_t* end = s->buf + s->length;

    while (start < end)
    {
      const int8_t* prev = start;
      uint32_t cp = utf8_decode(&start, end);
      if (!is_unicode_whitespace(cp))
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

  void x_smallstr_utf8_trim_right(XSmallstr* s)
  {
    const int8_t* start = s->buf;
    const int8_t* end = s->buf + s->length;
    const int8_t* p = end;

    while (p > start)
    {
      const int8_t* prev = p;
      do { --p; } while (p > start && ((*p & 0xC0) == 0x80));
      const int8_t* decode_from = p;
      uint32_t cp = utf8_decode(&decode_from, end);
      if (!is_unicode_whitespace(cp))
      {
        size_t new_len = prev - s->buf;
        s->length = new_len;
        s->buf[new_len] = '\0';
        return;
      }
    }
    x_smallstr_clear(s);
  }

  void x_smallstr_utf8_trim(XSmallstr* s)
  {
    x_smallstr_utf8_trim_right(s);
    x_smallstr_utf8_trim_left(s);
  }

  int x_wsmallstr_from_wcstr(XWSmallstr* s, const wchar_t* src)
  {
    size_t len = wcslen(src);
    if (len > STDX_SMALLSTR_MAX_LENGTH) return -1;
    wcsncpy(s->buf, src, STDX_SMALLSTR_MAX_LENGTH);
    s->buf[len] = L'\0';
    s->length = len;
    return 0;
  }

  int x_wsmallstr_cmp(const XWSmallstr* a, const XWSmallstr* b)
  {
    return wcsncmp(a->buf, b->buf, a->length);
  }

  int x_wsmallstr_casecmp(const XWSmallstr* a, const XWSmallstr* b)
  {
    if (a->length != b->length) return 1;
    for (size_t i = 0; i < a->length; ++i)
    {
      if (towlower(a->buf[i]) != towlower(b->buf[i])) return 1;
    }
    return 0;
  }


  size_t x_wsmallstr_len(const XWSmallstr* s)
  {
    return s->length;
  }

  void x_wsmallstr_clear(XWSmallstr* s)
  {
    s->length = 0;
    s->buf[0] = L'\0';
  }

  int x_wsmallstr_append(XWSmallstr* s, const wchar_t* tail)
  {
    size_t tail_len = wcslen(tail);
    if (s->length + tail_len > STDX_SMALLSTR_MAX_LENGTH)
      return -1;

    wcsncpy(&s->buf[s->length], tail, tail_len);
    s->length += tail_len;
    s->buf[s->length] = L'\0';
    return 0;
  }

  int x_wsmallstr_substring(const XWSmallstr* s, size_t start, size_t len, XWSmallstr* out)
  {
    if (start > s->length || (start + len) > s->length)
      return -1;

    wcsncpy(out->buf, &s->buf[start], len);
    out->buf[len] = L'\0';
    out->length = len;
    return 0;
  }

  void x_wsmallstr_trim(XWSmallstr* s)
  {
    // Left trim
    size_t i = 0;
    while (i < s->length && iswspace(s->buf[i]))
      i++;

    if (i > 0)
    {
      size_t new_len = s->length - i;
      wmemmove(s->buf, &s->buf[i], new_len);
      s->length = new_len;
      s->buf[s->length] = L'\0';
    }

    // Right trim
    while (s->length > 0 && iswspace(s->buf[s->length - 1]))
      s->length--;

    s->buf[s->length] = L'\0';
  }

  void x_wsmallstr_token_iter_init(XWSmallstrTokenIterator* iter, const XWSmallstr* s, wchar_t delimiter)
  {
    iter->s = s;
    iter->pos = 0;
    iter->delimiter = delimiter;
  }

  int x_wsmallstr_token_iter_next(XWSmallstrTokenIterator* iter, XWSmallstr* token)
  {
    if (iter->pos >= iter->s->length)
      return 0;

    size_t start = iter->pos;
    while (iter->pos < iter->s->length && iter->s->buf[iter->pos] != iter->delimiter)
      iter->pos++;

    size_t len = iter->pos - start;
    x_wsmallstr_substring(iter->s, start, len, token);

    if (iter->pos < iter->s->length)
      iter->pos++; // Skip delimiter

    return 1;
  }

  inline bool x_strview_empty(XStrview sv)
  {
    return sv.length == 0;
  }

  bool x_strview_eq(XStrview a, XStrview b)
  {
    return a.length == b.length && (memcmp(a.data, b.data, a.length) == 0);
  }

  bool x_strview_eq_cstr(XStrview a, const int8_t* b)
  {
    return x_strview_eq(a, x_strview(b));
  }

  int x_strview_cmp(XStrview a, XStrview b)
  {
    size_t min_len = a.length < b.length ? a.length : b.length;
    int r = memcmp(a.data, b.data, min_len);
    if (r != 0) return r;
    return (int)(a.length - b.length);
  }

  bool x_strview_case_eq(XStrview a, XStrview b)
  {
    if (a.length != b.length) return 0;
    for (size_t i = 0; i < a.length; i++)


    {
      if (tolower((unsigned char)a.data[i]) != tolower((unsigned char)b.data[i]))
        return false;
    }
    return true;
  }

  int x_strview_case_cmp(XStrview a, XStrview b)
  {
    size_t min_len = a.length < b.length ? a.length : b.length;
    for (size_t i = 0; i < min_len; i++)


    {
      int ca = tolower((unsigned char)a.data[i]);
      int cb = tolower((unsigned char)b.data[i]);
      if (ca != cb) return ca - cb;
    }
    return (int)(a.length - b.length);
  }

  XStrview x_strview_substr(XStrview sv, size_t start, size_t len)
  {
    if (start > sv.length) start = sv.length;
    if (start + len > sv.length) len = sv.length - start;
    return (XStrview){ sv.data + start, len };
  }

  XStrview x_strview_trim_left(XStrview sv)
  {
    size_t i = 0;
    while (i < sv.length && (unsigned char)sv.data[i] <= ' ') i++;
    return x_strview_substr(sv, i, sv.length - i);
  }

  XStrview x_strview_trim_right(XStrview sv)
  {
    size_t i = sv.length;
    while (i > 0 && (unsigned char)sv.data[i - 1] <= ' ') i--;
    return x_strview_substr(sv, 0, i);
  }

  XStrview x_strview_trim(XStrview sv)
  {
    return x_strview_trim_right(x_strview_trim_left(sv));
  }

  int x_strview_find(XStrview sv, int8_t c)
  {
    for (size_t i = 0; i < sv.length; i++)

    {
      if (sv.data[i] == c) return (int)i;
    }
    return -1;
  }

  int x_strview_rfind(XStrview sv, int8_t c)
  {
    for (size_t i = sv.length; i > 0; i--)

    {
      if (sv.data[i - 1] == c) return (int)(i - 1);
    }
    return -1;
  }

  bool x_strview_split_at(XStrview sv, int8_t delim, XStrview* left, XStrview* right)
  {
    int pos = x_strview_find(sv, delim);
    if (pos < 0) return false;
    *left = x_strview_substr(sv, 0, pos);
    *right = x_strview_substr(sv, pos + 1, sv.length - pos - 1);
    return true;
  }

  // Yields the next token before `delim` and advances input
  bool x_strview_next_token(XStrview* input, int8_t delim, XStrview* token)
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
      return 1;
    }
    return false;
  }

  bool x_wstrview_empty(XWStrview sv)
  {
    return sv.length == 0;
  }

  bool x_wstrview_eq(XWStrview a, XWStrview b)
  {
    return a.length == b.length && wcsncmp(a.data, b.data, a.length) == 0;
  }

  int x_wstrview_cmp(XWStrview a, XWStrview b)
  {
    size_t min = a.length < b.length ? a.length : b.length;
    int result = wcsncmp(a.data, b.data, min);
    return result != 0 ? result : (int)(a.length - b.length);
  }

  XWStrview x_wstrview_substr(XWStrview sv, size_t start, size_t len)
  {
    if (start > sv.length) start = sv.length;
    if (start + len > sv.length) len = sv.length - start;
    return (XWStrview){ sv.data + start, len };
  }

  XWStrview x_wstrview_trim_left(XWStrview sv)
  {
    size_t i = 0;
    while (i < sv.length && iswspace(sv.data[i])) i++;
    return x_wstrview_substr(sv, i, sv.length - i);
  }

  XWStrview x_wstrview_trim_right(XWStrview sv)
  {
    size_t i = sv.length;
    while (i > 0 && iswspace(sv.data[i - 1])) i--;
    return x_wstrview_substr(sv, 0, i);
  }

  XWStrview x_wstrview_trim(XWStrview sv)
  {
    return x_wstrview_trim_right(x_wstrview_trim_left(sv));
  }

  XStrview x_strview_utf8_substr(XStrview sv, size_t char_start, size_t char_len)
  {
    size_t byte_start = utf8_advance(sv.data, sv.length, char_start);
    size_t byte_end = utf8_advance(sv.data + byte_start, sv.length - byte_start, char_len);
    return (XStrview){
      .data = sv.data + byte_start,
        .length = byte_end
    };
  }

  XStrview x_strview_utf8_trim_left(XStrview sv)
  {
    const int8_t* start = sv.data;
    const int8_t* end = sv.data + sv.length;
    while (start < end)
    {
      const int8_t* prev = start;
      uint32_t cp = utf8_decode(&start, end);
      if (!is_unicode_whitespace(cp))
      {
        return (XStrview){ prev, (size_t)(end - prev) };
      }
    }
    return (XStrview){ end, 0 }; // All whitespace
  }

  XStrview x_strview_utf8_trim_right(XStrview sv)
  {
    const int8_t* start = sv.data;
    const int8_t* end = sv.data + sv.length;
    const int8_t* p = end;
    while (p > start)
    {
      const int8_t* prev = p;
      // Step back 1 byte to guess start of char
      do { --p; } while (p > start && ((*p & 0xC0) == 0x80));
      const int8_t* decode_from = p;
      uint32_t cp = utf8_decode(&decode_from, end);
      if (!is_unicode_whitespace(cp))
      {
        return (XStrview){ sv.data, (size_t)(prev - sv.data) };
      }
    }
    return (XStrview){ sv.data + sv.length, 0 };
  }

  XStrview x_strview_utf8_trim(XStrview sv)
  {
    return x_strview_utf8_trim_right(x_strview_utf8_trim_left(sv));
  }

#endif // STDX_IMPLEMENTATION_STRING

#ifdef __cplusplus
}
#endif

#endif // STDX_STRING_H

