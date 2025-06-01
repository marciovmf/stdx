/*
 * XStrBuilder - Dynamic String Builder for C
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a simple interface for constructing strings efficiently:
 *   - Dynamic growth as data is appended
 *   - Append strings, characters, substrings, and formatted text
 *   - Convert to null-terminated C string
 *   - Clear or destroy the builder when done
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_STRINGBUILDER
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "strbuilder.h"
 */

#ifndef STDX_STRINGBUILDER_H
#define STDX_STRINGBUILDER_H

#ifdef __cplusplus
extern "C"
{
#endif

#define STDX_STRINGBUILDER_VERSION_MAJOR 1
#define STDX_STRINGBUILDER_VERSION_MINOR 0
#define STDX_STRINGBUILDER_VERSION_PATCH 0

#define STDX_STRINGBUILDER_VERSION (STDX_STRINGBUILDER_VERSION_MAJOR * 10000 + STDX_STRINGBUILDER_VERSION_MINOR * 100 + STDX_STRINGBUILDER_VERSION_PATCH)

#ifndef STRBUILDER_STACK_BUFFER_SIZE
#define STRBUILDER_STACK_BUFFER_SIZE 255
#endif

  typedef struct XStrBuilder_t XStrBuilder;
  typedef struct XWStrBuilder_t XWStrBuilder;

#include <wchar.h>

  XStrBuilder*x_strbuilder_create();
  void         x_strbuilder_append(XStrBuilder *sb, const char *str);
  void         x_strbuilder_append_char(XStrBuilder* sb, char c);
  void         x_strbuilder_append_format(XStrBuilder *sb, const char *format, ...);
  void         x_strbuilder_append_substring(XStrBuilder *sb, const char *start, size_t length);
  char*        x_strbuilder_to_string(const XStrBuilder *sb);
  void         x_strbuilder_destroy(XStrBuilder *sb);
  void         x_strbuilder_clear(XStrBuilder *sb);
  size_t       x_strbuilder_length(XStrBuilder *sb);
  void         x_strbuilder_append_utf8_substring(XStrBuilder* sb, const char* utf8, size_t start_cp, size_t len_cp); // UTF-8-aware append
  size_t       x_strbuilder_utf8_charlen(const XStrBuilder* sb); // UTF-8 aware length

  XWStrBuilder* x_wstrbuilder_create();
  void          x_wstrbuilder_append(XWStrBuilder* sb, const wchar_t* str);
  void          x_wstrbuilder_append_char(XWStrBuilder* sb, wchar_t c);
  void          x_wstrbuilder_append_format(XWStrBuilder* sb, const wchar_t* format, ...);
  void          x_wstrbuilder_clear(XWStrBuilder* sb);
  void          x_wstrbuilder_destroy(XWStrBuilder* sb);
  wchar_t*      x_wstrbuilder_to_string(const XWStrBuilder* sb);
  size_t        x_wstrbuilder_length(const XWStrBuilder* sb);

#ifdef STDX_IMPLEMENTATION_STRINGBUILDER

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

  struct XStrBuilder_t
  {
    char *data;
    size_t capacity;
    size_t length;
  };

  struct XWStrBuilder_t
  {
    wchar_t* data;
    size_t capacity;
    size_t length;
  };

  static size_t utf8_advance_index(const char* s, size_t maxlen, size_t count)
  {
    size_t i = 0, cp = 0;
    while (i < maxlen && cp < count)
    {
      unsigned char c = (unsigned char)s[i];
      if ((c & 0x80) == 0x00) i += 1;
      else if ((c & 0xE0) == 0xC0) i += 2;
      else if ((c & 0xF0) == 0xE0) i += 3;
      else if ((c & 0xF8) == 0xF0) i += 4;
      else break;
      cp++;
    }
    return i;
  }

  XStrBuilder* x_strbuilder_create()
  {
    XStrBuilder* sb = (XStrBuilder*)  malloc(sizeof(XStrBuilder));
    sb->capacity = 16; // Initial capacity
    sb->data = (char *) malloc(sb->capacity * sizeof(char));
    sb->length = 0;
    sb->data[0] = '\0'; // Null-terminate the string
    return sb;
  }

  void x_strbuilder_append(XStrBuilder *sb, const char *str)
  {
    size_t strLen = strlen(str);
    size_t newLength = sb->length + strLen;

    if (newLength + 1 > sb->capacity)
    {
      while (newLength + 1 > sb->capacity)
      {
        sb->capacity *= 2; // Double the capacity
      }
      sb->data = (char *)realloc(sb->data, sb->capacity * sizeof(char));
    }

    strcat(sb->data, str);
    sb->length = newLength;
  }

  void x_strbuilder_append_format(XStrBuilder *sb, const char *format, ...)
  {
    va_list args;
    va_start(args, format);

    // Stack-allocated buffer for small strings
    char stack_buffer[STRBUILDER_STACK_BUFFER_SIZE];
    static const int STACK_BUFFER_SIZE = STRBUILDER_STACK_BUFFER_SIZE;

    // Determine the size needed for the formatted string
    size_t needed = vsnprintf(stack_buffer, STACK_BUFFER_SIZE, format, args);

    if (needed < STACK_BUFFER_SIZE)
    {
      // The formatted string fits within the stack buffer
      x_strbuilder_append(sb, stack_buffer);
    }
    else
    {
      // Allocate memory for the formatted string
      char *formatted_str = (char *)malloc((needed + 1) * sizeof(char));

      // Format the string
      vsnprintf(formatted_str, needed + 1, format, args);

      // Append the formatted string to the builder
      x_strbuilder_append(sb, formatted_str);

      // Free memory
      free(formatted_str);
    }

    va_end(args);
  }

  void x_strbuilder_append_char(XStrBuilder* sb, char c)
  {
    char s[2] =
    {c, 0};
    x_strbuilder_append(sb, s);
  }

  void x_strbuilder_append_substring(XStrBuilder *sb, const char *start, size_t length)
  {
    x_strbuilder_append_format(sb, "%.*s", length, start);
  }

  char* x_strbuilder_to_string(const XStrBuilder *sb)
  {
    return sb->data;
  }

  void x_strbuilder_destroy(XStrBuilder *sb)
  {
    free(sb->data);
    sb->data = NULL;
    sb->capacity = 0;
    sb->length = 0;
    free(sb);
  }

  void x_strbuilder_clear(XStrBuilder *sb)
  {
    if (sb->length > 0)

    {
      sb->data[0] = 0;
      sb->length = 0;
    }
  }

  size_t x_strbuilder_length(XStrBuilder *sb)
  {
    return sb->length;
  }

  void x_strbuilder_append_utf8_substring(XStrBuilder* sb, const char* utf8, size_t start_cp, size_t len_cp)
  {
    size_t byte_start = utf8_advance_index(utf8, strlen(utf8), start_cp);
    size_t byte_len = utf8_advance_index(utf8 + byte_start, strlen(utf8 + byte_start), len_cp);
    x_strbuilder_append_substring(sb, utf8 + byte_start, byte_len);
  }

  size_t x_strbuilder_utf8_charlen(const XStrBuilder* sb)
  {
    size_t i = 0, count = 0;
    const char* s = sb->data;
    while (*s)
    {
      unsigned char c = (unsigned char)*s;
      if ((c & 0x80) == 0x00) s += 1;
      else if ((c & 0xE0) == 0xC0) s += 2;
      else if ((c & 0xF0) == 0xE0) s += 3;
      else if ((c & 0xF8) == 0xF0) s += 4;
      else break;
      count++;
    }
    return count;
  }

  XWStrBuilder* x_wstrbuilder_create()
  {
    XWStrBuilder* sb = (XWStrBuilder*)malloc(sizeof(XWStrBuilder));
    sb->capacity = 16;
    sb->length = 0;
    sb->data = (wchar_t*)malloc(sb->capacity * sizeof(wchar_t));
    sb->data[0] = L'\0';
    return sb;
  }

  void x_wstrbuilder_grow(XWStrBuilder* sb, size_t needed_len)
  {
    if (sb->length + needed_len + 1 > sb->capacity)
    {
      while (sb->length + needed_len + 1 > sb->capacity)
      {
        sb->capacity *= 2;
      }
      sb->data = (wchar_t*)realloc(sb->data, sb->capacity * sizeof(wchar_t));
    }
  }

  void x_wstrbuilder_append(XWStrBuilder* sb, const wchar_t* str)
  {
    size_t len = wcslen(str);
    x_wstrbuilder_grow(sb, len);
    wcscat(sb->data, str);
    sb->length += len;
  }

  void x_wstrbuilder_append_char(XWStrBuilder* sb, wchar_t c)
  {
    wchar_t buf[2] =
    {c, 0};
    x_wstrbuilder_append(sb, buf);
  }

  void x_wstrbuilder_append_format(XWStrBuilder* sb, const wchar_t* format, ...)
  {
    va_list args;
    va_start(args, format);

    wchar_t tmp[512];
    vswprintf(tmp, 512, format, args);
    x_wstrbuilder_append(sb, tmp);

    va_end(args);
  }

  void x_wstrbuilder_clear(XWStrBuilder* sb)
  {
    sb->length = 0;
    sb->data[0] = L'\0';
  }

  void x_wstrbuilder_destroy(XWStrBuilder* sb)
  {
    free(sb->data);
    free(sb);
  }

  wchar_t* x_wstrbuilder_to_string(const XWStrBuilder* sb)
  {
    return sb->data;
  }

  size_t x_wstrbuilder_length(const XWStrBuilder* sb)
  {
    return sb->length;
  }

#endif // STDX_IMPLEMENTATION_STRINGBUILDER

#ifdef __cplusplus
}
#endif
#endif // STDX_STRINGBUILDER_H

