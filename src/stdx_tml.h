/** 
 * STDX - Tree Markup Language (TML)
 * Part of the STDX General Purpose C Library by marciovmf
 * License: MIT
 * https://github.com/marciovmf/stdx
 *
 * To compile the implementation define X_IMPL_TML
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * `X_ARRAY_ALLOC` / `X_ARRAY_REALLOC` / `X_ARRAY_FREE` before including.
 *
 *  TML is a minimal, indentation-based hierarchical data format.
 *  It borrows the readability of YAML but is parsed in two phases,
 *  with a single arena allocation.
 *
 *   Notes:
 *   - Nodes are stored in document order.
 *   - Direct children are linked with first_child / next_sibling because direct
 *   child nodes are not necessarily contiguous in document order.
 *   - Entries are key:value pairs. They are never anonymous.
 *   - Anonymous nodes have name.size == 0 and name.data == NULL.
 *   - Strings are stored as views but are also NUL-terminated in string_data.
 * 
 *  Syntax overview:
 *  - Indentation defines hierarchy. The first indented line sets the unit (2 or 4 spaces).
 *  - Tabs are invalid.
 *  - Lines ending with ':' define a new section (node).
 *  - Lists are defined with '-' as anonymous entries:
 *  - Triple quotes (""") define multi-line strings.
 *      The first newline after """ and the last before """ are ignored.
 *  - Comma-separated lists are parsed once into typed arrays (i64/f64/str).
 *  - Comments start with '#'.
 * 
 *  The TML parser performs:
 *  - Single memory allocation (arena-style).
 *  - Linear-time parsing with no dynamic allocations.
 *  - Typed key/value decoding (bool, int, float, string, array).
 * 
 * Example:
 *
 * level:
 * name: "Caverns"
 * seed: 12345
 * enabled: true
 * objects:
 * - position: 1.0, 2.0, 0.0
 * scale: 1.0, 1.0, 1.0
 * - position: 7.0, 0.0, 0.0
 * scale: 1.0, 1.0, 1.0
 */

#ifndef STDX_TML_H
#define STDX_TML_H

#include <errno.h>
#include <stddef.h> // size_t
#include <stdint.h> // uint32_t uint64_t uint8_t
#include <string.h>

#ifndef X_TML_API
#define X_TML_API
#endif

#define X_TML_VERSION_MAJOR 1
#define X_TML_VERSION_MINOR 0
#define X_TML_VERSION_PATCH 0
#define X_TML_VERSION (X_TML_VERSION_MAJOR * 10000 + X_TML_VERSION_MINOR * 100 + X_TML_VERSION_PATCH)

#ifdef __cplusplus
extern "C" {
#endif

  typedef uint8_t  u8;
  typedef uint32_t u32;
  typedef uint64_t u64;
  typedef int64_t  i64;
  typedef double   f64;

  typedef struct TMLString
  {
    char const* data;
    u32 size;
  } TMLString;

  typedef enum TMLValueType
  {
    TML_VALUE_BOOL,
    TML_VALUE_I64,
    TML_VALUE_F64,
    TML_VALUE_STRING,
    TML_VALUE_ARRAY_I64,
    TML_VALUE_ARRAY_F64,
    TML_VALUE_ARRAY_STRING
  } TMLValueType;

  typedef struct TMLArray
  {
    u32 first;
    u32 count;
  } TMLArray;

  typedef struct TMLI64Slice
  {
    i64 const* data;
    u32 count;
  } TMLI64Slice;

  typedef struct TMLF64Slice
  {
    f64 const* data;
    u32 count;
  } TMLF64Slice;

  typedef struct TMLStringSlice
  {
    TMLString const* data;
    u32 count;
  } TMLStringSlice;
  

  typedef struct TMLEntry
  {
    TMLString name;
    TMLValueType type;
    u32 parent;
    u32 next_entry;

    union
    {
      u8 boolean;
      i64 integer;
      f64 number;
      TMLString string;
      TMLArray array;
    };
  } TMLEntry;

  typedef struct TMLNode
  {
    TMLString name;

    u32 parent;
    u32 next_sibling;

    u32 first_child;
    u32 last_child;
    u32 child_count;

    u32 first_entry;
    u32 last_entry;
    u32 entry_count;
  } TMLNode;

  typedef struct TMLDocument
  {
    void* memory;
    u64 memory_size;

    TMLNode* nodes;
    TMLEntry* entries;

    char* string_data;
    i64* array_i64;
    f64* array_f64;
    TMLString* array_string;

    u32 node_count;
    u32 entry_count;
    u32 string_size;
    u32 array_i64_count;
    u32 array_f64_count;
    u32 array_string_count;

    u32 first_root_node;
    u32 last_root_node;
    u32 root_node_count;
  } TMLDocument;

  typedef struct TMLParseResult
  {
    int ok;
    char error[256];
    u32 line;
    u32 column;
    TMLDocument* document;
  } TMLParseResult;

  X_TML_API void tml_document_free(TMLDocument* doc);
  X_TML_API TMLParseResult tml_parse(char const* source);
  X_TML_API TMLNode const* tml_node_at(TMLDocument const* doc, u32 index);
  X_TML_API TMLEntry const* tml_entry_at(TMLDocument const* doc, u32 index);
  X_TML_API TMLNode const* tml_root_node_at(TMLDocument const* doc, u32 index);
  X_TML_API TMLNode const* tml_node_child_at(TMLDocument const* doc, TMLNode const* node, u32 index);
  X_TML_API TMLEntry const* tml_node_entry_at(TMLDocument const* doc, TMLNode const* node, u32 index);
  X_TML_API TMLNode const* tml_node_find_child(TMLDocument const* doc, TMLNode const* node, char const* name);
  X_TML_API TMLEntry const* tml_node_find_entry(TMLDocument const* doc, TMLNode const* node, char const* name);


  X_TML_API int tml_entry_get_bool(TMLEntry const* entry, u8* out_value);
  X_TML_API int tml_entry_get_i64(TMLEntry const* entry, i64* out_value);
  X_TML_API int tml_entry_get_f64(TMLEntry const* entry, f64* out_value);
  X_TML_API int tml_entry_get_string(TMLEntry const* entry, TMLString* out_value);
  X_TML_API int tml_entry_get_i64_array(TMLDocument const* doc, TMLEntry const* entry, TMLI64Slice* out_value);
  X_TML_API int tml_entry_get_f64_array(TMLDocument const* doc, TMLEntry const* entry, TMLF64Slice* out_value);
  X_TML_API int tml_entry_get_string_array(TMLDocument const* doc, TMLEntry const* entry, TMLStringSlice* out_value);

  X_TML_API int tml_node_get_bool(TMLDocument const* doc, TMLNode const* node, char const* name, u8* out_value);
  X_TML_API int tml_node_get_i64(TMLDocument const* doc, TMLNode const* node, char const* name, i64* out_value);
  X_TML_API int tml_node_get_f64(TMLDocument const* doc, TMLNode const* node, char const* name, f64* out_value);
  X_TML_API int tml_node_get_string(TMLDocument const* doc, TMLNode const* node, char const* name, TMLString* out_value);
  X_TML_API int tml_node_get_i64_array(TMLDocument const* doc, TMLNode const* node, char const* name, TMLI64Slice* out_value);
  X_TML_API int tml_node_get_f64_array(TMLDocument const* doc, TMLNode const* node, char const* name, TMLF64Slice* out_value);
  X_TML_API int tml_node_get_string_array(TMLDocument const* doc, TMLNode const* node, char const* name, TMLStringSlice* out_value);

  X_TML_API TMLNode const* tml_path_find_node(TMLDocument const* doc, char const* path);
  X_TML_API TMLEntry const* tml_path_find_entry(TMLDocument const* doc, char const* path);
  X_TML_API int tml_path_get_bool(TMLDocument const* doc, char const* path, u8* out_value);
  X_TML_API int tml_path_get_i64(TMLDocument const* doc, char const* path, i64* out_value);
  X_TML_API int tml_path_get_f64(TMLDocument const* doc, char const* path, f64* out_value);
  X_TML_API int tml_path_get_string(TMLDocument const* doc, char const* path, TMLString* out_value);
  X_TML_API int tml_path_get_i64_array(TMLDocument const* doc, char const* path, TMLI64Slice* out_value);
  X_TML_API int tml_path_get_f64_array(TMLDocument const* doc, char const* path, TMLF64Slice* out_value);
  X_TML_API int tml_path_get_string_array(TMLDocument const* doc, char const* path, TMLStringSlice* out_value);



#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_TML

#ifdef __cplusplus
extern "C" {
#endif


#ifndef X_TML_ALLOC
  /**
   * @brief Internal macro for allocating memory.
   * To override how this header allocates memory, define this macro with a
   * different implementation before including this header.
   * @param sz  The size of memory to alloc.
   */
#define X_TML_ALLOC(sz) calloc(1, sz)
#endif

#ifndef X_TML_FREE
  /**
   * @brief Internal macro for freeing memory.
   * To override how this header frees memory, define this macro with a
   * different implementation before including this header.
   * @param p  The address of memory region to free.
   */
#define X_TML_FREE(p) free(p)
#endif

#define TML_INVALID_INDEX ((uint32_t)0xFFFFFFFFu)

  typedef struct TMLStats
  {
    u32 node_count;
    u32 entry_count;
    u32 string_size;
    u32 array_i64_count;
    u32 array_f64_count;
    u32 array_string_count;
  } TMLStats;

  typedef enum TMLParseMode
  {
    TML_PARSE_COUNT,
    TML_PARSE_WRITE
  } TMLParseMode;

  typedef struct TMLScope
  {
    u32 node;
    u32 indent;
  } TMLScope;

  typedef struct TMLParser
  {
    char const* source;
    char const* cursor;
    char const* line_start;

    u32 line;
    u32 indent_unit;

    TMLParseMode mode;
    TMLStats stats;
    TMLDocument* doc;

    u32 node_cursor;
    u32 entry_cursor;
    u32 string_cursor;
    u32 array_i64_cursor;
    u32 array_f64_cursor;
    u32 array_string_cursor;

    TMLScope scopes[256];
    u32 scope_count;

    char error[256];
    u32 error_line;
    u32 error_column;
  } TMLParser;

  static u64 tml_align_forward_u64(u64 value, u64 align)
  {
    u64 mask;

    mask = align - 1u;
    return (value + mask) & ~mask;
  }

  static int tml_string_equals_cstr(TMLString s, char const* cstr)
  {
    size_t len;

    len = strlen(cstr);

    if (s.size != (u32)len)
    {
      return 0;
    }

    return memcmp(s.data, cstr, len) == 0;
  }

  static int tml_set_error(TMLParser* p, char const* msg)
  {
    u32 col;

    col = (u32)(p->cursor - p->line_start) + 1u;

    snprintf(p->error, sizeof(p->error), "%s", msg);
    p->error_line = p->line;
    p->error_column = col;

    return 0;
  }

  static int is_identifier_start(char c)
  {
    return isalpha((unsigned char)c) || c == '_';
  }

  static int is_identifier_char(char c)
  {
    return isalnum((unsigned char)c) || c == '_';
  }

  static void tml_skip_spaces(TMLParser* p)
  {
    while (*p->cursor == ' ')
    {
      p->cursor += 1;
    }
  }

  static void tml_skip_comment(TMLParser* p)
  {
    if (*p->cursor == '#')
    {
      while (*p->cursor != '\0' && *p->cursor != '\n' && *p->cursor != '\r')
      {
        p->cursor += 1;
      }
    }
  }

  static int tml_at_line_end_after_space_or_comment(TMLParser* p)
  {
    char const* save;

    save = p->cursor;

    while (*save == ' ')
    {
      save += 1;
    }

    if (*save == '#')
    {
      while (*save != '\0' && *save != '\n' && *save != '\r')
      {
        save += 1;
      }
    }

    return *save == '\0' || *save == '\n' || *save == '\r';
  }

  static int tml_consume_line_end(TMLParser* p)
  {
    tml_skip_spaces(p);
    tml_skip_comment(p);

    if (*p->cursor == '\r')
    {
      p->cursor += 1;

      if (*p->cursor == '\n')
      {
        p->cursor += 1;
      }

      p->line += 1;
      p->line_start = p->cursor;
      return 1;
    }

    if (*p->cursor == '\n')
    {
      p->cursor += 1;
      p->line += 1;
      p->line_start = p->cursor;
      return 1;
    }

    if (*p->cursor == '\0')
    {
      return 1;
    }

    return tml_set_error(p, "expected end of line");
  }

  static TMLString tml_read_identifier_view(TMLParser* p)
  {
    TMLString result;
    char const* begin;

    result.data = NULL;
    result.size = 0;

    if (!is_identifier_start(*p->cursor))
    {
      return result;
    }

    begin = p->cursor;
    p->cursor += 1;

    while (is_identifier_char(*p->cursor))
    {
      p->cursor += 1;
    }

    result.data = begin;
    result.size = (u32)(p->cursor - begin);

    return result;
  }

  static int identifier_reserved(TMLString id)
  {
    return tml_string_equals_cstr(id, "true") || tml_string_equals_cstr(id, "false");
  }

  static TMLString tml_emit_string(TMLParser* p, char const* data, u32 size)
  {
    TMLString result;

    result.data = NULL;
    result.size = size;

    if (p->mode == TML_PARSE_COUNT)
    {
      p->stats.string_size += size + 1u;
      return result;
    }

    result.data = p->doc->string_data + p->string_cursor;

    if (size > 0)
    {
      memcpy(p->doc->string_data + p->string_cursor, data, size);
    }

    p->doc->string_data[p->string_cursor + size] = '\0';
    p->string_cursor += size + 1u;

    return result;
  }

  static TMLString tml_emit_identifier(TMLParser* p, TMLString id)
  {
    return tml_emit_string(p, id.data, id.size);
  }

  static u32 tml_emit_node(TMLParser* p, TMLNode* node)
  {
    u32 index;

    if (p->mode == TML_PARSE_COUNT)
    {
      p->stats.node_count += 1u;
      return p->stats.node_count - 1u;
    }

    index = p->node_cursor;
    p->doc->nodes[index] = *node;
    p->node_cursor += 1u;

    return index;
  }

  static u32 tml_emit_entry(TMLParser* p, TMLEntry* entry)
  {
    u32 index;

    if (p->mode == TML_PARSE_COUNT)
    {
      p->stats.entry_count += 1u;
      return p->stats.entry_count - 1u;
    }

    index = p->entry_cursor;
    p->doc->entries[index] = *entry;
    p->entry_cursor += 1u;

    return index;
  }

  static TMLArray tml_emit_array_i64(TMLParser* p, u32 count)
  {
    TMLArray array;

    array.first = 0;
    array.count = count;

    if (p->mode == TML_PARSE_COUNT)
    {
      p->stats.array_i64_count += count;
      return array;
    }

    array.first = p->array_i64_cursor;
    p->array_i64_cursor += count;

    return array;
  }

  static TMLArray tml_emit_array_f64(TMLParser* p, u32 count)
  {
    TMLArray array;

    array.first = 0;
    array.count = count;

    if (p->mode == TML_PARSE_COUNT)
    {
      p->stats.array_f64_count += count;
      return array;
    }

    array.first = p->array_f64_cursor;
    p->array_f64_cursor += count;

    return array;
  }

  static TMLArray tml_emit_array_string(TMLParser* p, u32 count)
  {
    TMLArray array;

    array.first = 0;
    array.count = count;

    if (p->mode == TML_PARSE_COUNT)
    {
      p->stats.array_string_count += count;
      return array;
    }

    array.first = p->array_string_cursor;
    p->array_string_cursor += count;

    return array;
  }

  static int tml_parse_string_to_buffer(TMLParser* p, char* dst, u32* out_size)
  {
    u32 size;

    if (*p->cursor != '"')
    {
      return tml_set_error(p, "expected string");
    }

    p->cursor += 1;
    size = 0;

    while (*p->cursor != '\0')
    {
      char c;

      c = *p->cursor;

      if (c == '"')
      {
        p->cursor += 1;
        *out_size = size;
        return 1;
      }

      if (c == '\\')
      {
        p->cursor += 1;
        c = *p->cursor;

        if (c == '\0')
        {
          return tml_set_error(p, "unfinished string escape");
        }

        if (c == 'n')
        {
          c = '\n';
        }
        else if (c == 'r')
        {
          c = '\r';
        }
        else if (c == 't')
        {
          c = '\t';
        }
        else if (c == '"')
        {
          c = '"';
        }
        else if (c == '\\')
        {
          c = '\\';
        }
        else
        {
          return tml_set_error(p, "unknown string escape");
        }

        if (dst != NULL)
        {
          dst[size] = c;
        }

        size += 1;
        p->cursor += 1;
        continue;
      }

      if (c == '\r')
      {
        if (dst != NULL)
        {
          dst[size] = '\n';
        }

        size += 1;
        p->cursor += 1;

        if (*p->cursor == '\n')
        {
          p->cursor += 1;
        }

        p->line += 1;
        p->line_start = p->cursor;
        continue;
      }

      if (c == '\n')
      {
        if (dst != NULL)
        {
          dst[size] = '\n';
        }

        size += 1;
        p->cursor += 1;
        p->line += 1;
        p->line_start = p->cursor;
        continue;
      }

      if (dst != NULL)
      {
        dst[size] = c;
      }

      size += 1;
      p->cursor += 1;
    }

    return tml_set_error(p, "unterminated string");
  }

  static TMLString tml_parse_and_emit_string(TMLParser* p)
  {
    char const* save_cursor;
    char const* save_line_start;
    u32 save_line;
    u32 size;
    TMLString result;
    char* dst;

    result.data = NULL;
    result.size = 0;

    save_cursor = p->cursor;
    save_line_start = p->line_start;
    save_line = p->line;

    if (!tml_parse_string_to_buffer(p, NULL, &size))
    {
      return result;
    }

    if (p->mode == TML_PARSE_COUNT)
    {
      return tml_emit_string(p, NULL, size);
    }

    dst = p->doc->string_data + p->string_cursor;

    p->cursor = save_cursor;
    p->line_start = save_line_start;
    p->line = save_line;

    if (!tml_parse_string_to_buffer(p, dst, &size))
    {
      return result;
    }

    result.data = dst;
    result.size = size;
    dst[size] = '\0';
    p->string_cursor += size + 1u;

    return result;
  }

  typedef enum TMLScalarKind
  {
    TML_SCALAR_BOOL,
    TML_SCALAR_I64,
    TML_SCALAR_F64,
    TML_SCALAR_STRING
  } TMLScalarKind;

  typedef struct TMLScalar
  {
    TMLScalarKind kind;
    union
    {
      u8 boolean;
      i64 integer;
      f64 number;
      TMLString string;
    };
  } TMLScalar;

  static int tml_parse_number(TMLParser* p, TMLScalar* out)
  {
    char const* begin = p->cursor;
    int is_float = 0;
    char* end = NULL;
    errno = 0;

    if (*p->cursor == '-' || *p->cursor == '+')
    {
      p->cursor += 1;
    }

    if (p->cursor[0] == '0' && (p->cursor[1] == 'x' || p->cursor[1] == 'X'))
    {
      long long value;

      p->cursor += 2;

      if (!isxdigit((unsigned char)*p->cursor))
      {
        return tml_set_error(p, "expected hexadecimal digit");
      }

      value = strtoll(begin, &end, 0);

      if (errno == ERANGE)
      {
        return tml_set_error(p, "integer out of range");
      }

      p->cursor = end;
      out->kind = TML_SCALAR_I64;
      out->integer = (i64)value;
      return 1;
    }

    if (!isdigit((unsigned char)*p->cursor))
    {
      return tml_set_error(p, "expected number");
    }

    while (isdigit((unsigned char)*p->cursor))
    {
      p->cursor += 1;
    }

    if (*p->cursor == '.')
    {
      is_float = 1;
      p->cursor += 1;

      while (isdigit((unsigned char)*p->cursor))
      {
        p->cursor += 1;
      }
    }

    if (*p->cursor == 'e' || *p->cursor == 'E')
    {
      is_float = 1;
      p->cursor += 1;

      if (*p->cursor == '-' || *p->cursor == '+')
      {
        p->cursor += 1;
      }

      if (!isdigit((unsigned char)*p->cursor))
      {
        return tml_set_error(p, "expected exponent digit");
      }

      while (isdigit((unsigned char)*p->cursor))
      {
        p->cursor += 1;
      }
    }

    if (is_float)
    {
      double value;

      value = strtod(begin, &end);

      if (errno == ERANGE)
      {
        return tml_set_error(p, "float out of range");
      }

      p->cursor = end;
      out->kind = TML_SCALAR_F64;
      out->number = value;
      return 1;
    }
    else
    {
      long long value;

      value = strtoll(begin, &end, 10);

      if (errno == ERANGE)
      {
        return tml_set_error(p, "integer out of range");
      }

      p->cursor = end;
      out->kind = TML_SCALAR_I64;
      out->integer = (i64)value;
      return 1;
    }
  }

  static int tml_parse_scalar(TMLParser* p, TMLScalar* out)
  {
    TMLString id;

    tml_skip_spaces(p);

    if (*p->cursor == '"')
    {
      out->kind = TML_SCALAR_STRING;
      out->string = tml_parse_and_emit_string(p);

      if (p->error[0] != '\0')
      {
        return 0;
      }

      return 1;
    }

    if (is_identifier_start(*p->cursor))
    {
      id = tml_read_identifier_view(p);

      if (tml_string_equals_cstr(id, "true"))
      {
        out->kind = TML_SCALAR_BOOL;
        out->boolean = 1;
        return 1;
      }

      if (tml_string_equals_cstr(id, "false"))
      {
        out->kind = TML_SCALAR_BOOL;
        out->boolean = 0;
        return 1;
      }

      return tml_set_error(p, "unknown identifier value");
    }

    if (*p->cursor == '-' || *p->cursor == '+' || isdigit((unsigned char)*p->cursor))
    {
      return tml_parse_number(p, out);
    }

    return tml_set_error(p, "expected value");
  }

  static int tml_parse_scalar_no_emit(TMLParser* p, TMLScalar* out)
  {
    TMLString id;
    u32 string_size = 0;

    tml_skip_spaces(p);

    if (*p->cursor == '"')
    {
      out->kind = TML_SCALAR_STRING;

      if (!tml_parse_string_to_buffer(p, NULL, &string_size))
      {
        return 0;
      }

      out->string.data = NULL;
      out->string.size = string_size;
      return 1;
    }

    if (is_identifier_start(*p->cursor))
    {
      id = tml_read_identifier_view(p);

      if (tml_string_equals_cstr(id, "true"))
      {
        out->kind = TML_SCALAR_BOOL;
        out->boolean = 1;
        return 1;
      }

      if (tml_string_equals_cstr(id, "false"))
      {
        out->kind = TML_SCALAR_BOOL;
        out->boolean = 0;
        return 1;
      }

      return tml_set_error(p, "unknown identifier value");
    }

    if (*p->cursor == '-' || *p->cursor == '+' || isdigit((unsigned char)*p->cursor))
    {
      return tml_parse_number(p, out);
    }

    return tml_set_error(p, "expected value");
  }

  static int tml_next_is_comma_before_comment_or_eol(TMLParser* p)
  {
    char const* c = p->cursor;
    while (*c == ' ')
    {
      c += 1;
    }

    return *c == ',';
  }

  static int tml_consume_optional_comma(TMLParser* p, int* out_has_comma)
  {
    tml_skip_spaces(p);

    if (*p->cursor == ',')
    {
      p->cursor += 1;
      *out_has_comma = 1;
      return 1;
    }

    *out_has_comma = 0;
    return 1;
  }

  static int tml_skip_array_separator(TMLParser* p, int has_comma)
  {
    tml_skip_spaces(p);
    tml_skip_comment(p);

    if (*p->cursor == '\r')
    {
      p->cursor += 1;

      if (*p->cursor == '\n')
      {
        p->cursor += 1;
      }

      p->line += 1;
      p->line_start = p->cursor;

      if (has_comma)
      {
        while (*p->cursor == ' ')
        {
          p->cursor += 1;
        }
      }

      return 1;
    }

    if (*p->cursor == '\n')
    {
      p->cursor += 1;
      p->line += 1;
      p->line_start = p->cursor;

      if (has_comma)
      {
        while (*p->cursor == ' ')
        {
          p->cursor += 1;
        }
      }

      return 1;
    }

    if (*p->cursor == '\0')
    {
      if (has_comma)
      {
        return tml_set_error(p, "array cannot end after comma");
      }

      return 1;
    }

    if (has_comma)
    {
      return 1;
    }

    return tml_set_error(p, "expected end of array line");
  }

  static int tml_parse_array_count_only(TMLParser* p, TMLScalarKind first_kind, u32* out_count)
  {
    TMLScalar scalar;
    int has_comma;
    u32 count = 0;

    for (;;)
    {
      if (!tml_parse_scalar_no_emit(p, &scalar))
      {
        return 0;
      }

      if (scalar.kind != first_kind)
      {
        return tml_set_error(p, "array elements must be homogeneous");
      }

      count += 1;

      if (!tml_consume_optional_comma(p, &has_comma))
      {
        return 0;
      }

      if (!has_comma)
      {
        break;
      }

      if (!tml_skip_array_separator(p, has_comma))
      {
        return 0;
      }
    }

    *out_count = count;
    return 1;
  }

  static int tml_parse_array_fill_i64(TMLParser* p, TMLArray array)
  {
    TMLScalar scalar;
    int has_comma;
    u32 index = 0;

    for (;;)
    {
      if (!tml_parse_scalar(p, &scalar))
      {
        return 0;
      }

      if (scalar.kind != TML_SCALAR_I64)
      {
        return tml_set_error(p, "expected integer array element");
      }

      p->doc->array_i64[array.first + index] = scalar.integer;
      index += 1;

      if (!tml_consume_optional_comma(p, &has_comma))
      {
        return 0;
      }

      if (!has_comma)
      {
        break;
      }

      if (!tml_skip_array_separator(p, has_comma))
      {
        return 0;
      }
    }

    return 1;
  }

  static int tml_parse_array_fill_f64(TMLParser* p, TMLArray array)
  {
    TMLScalar scalar;
    int has_comma;
    u32 index = 0;

    for (;;)
    {
      if (!tml_parse_scalar(p, &scalar))
      {
        return 0;
      }

      if (scalar.kind != TML_SCALAR_F64)
      {
        return tml_set_error(p, "expected float array element");
      }

      p->doc->array_f64[array.first + index] = scalar.number;
      index += 1;

      if (!tml_consume_optional_comma(p, &has_comma))
      {
        return 0;
      }

      if (!has_comma)
      {
        break;
      }

      if (!tml_skip_array_separator(p, has_comma))
      {
        return 0;
      }
    }

    return 1;
  }

  static int tml_parse_array_fill_string(TMLParser* p, TMLArray array)
  {
    TMLScalar scalar;
    int has_comma;
    u32 index = 0;

    for (;;)
    {
      if (!tml_parse_scalar(p, &scalar))
      {
        return 0;
      }

      if (scalar.kind != TML_SCALAR_STRING)
      {
        return tml_set_error(p, "expected string array element");
      }

      if (p->mode == TML_PARSE_WRITE)
      {
        p->doc->array_string[array.first + index] = scalar.string;
      }

      index += 1;

      if (!tml_consume_optional_comma(p, &has_comma))
      {
        return 0;
      }

      if (!has_comma)
      {
        break;
      }

      if (!tml_skip_array_separator(p, has_comma))
      {
        return 0;
      }
    }

    return 1;
  }

  static int tml_validate_unique_name(TMLParser* p, u32 parent, TMLString name)
  {
    u32 node_index;
    u32 entry_index;

    if (p->mode != TML_PARSE_WRITE)
    {
      return 1;
    }

    if (name.size == 0)
    {
      return 1;
    }

    if (parent == TML_INVALID_INDEX)
    {
      node_index = p->doc->first_root_node;
    }
    else
    {
      node_index = p->doc->nodes[parent].first_child;
    }

    while (node_index != TML_INVALID_INDEX)
    {
      TMLNode* node;

      node = p->doc->nodes + node_index;

      if (node->name.size == name.size && node->name.size > 0)
      {
        if (memcmp(node->name.data, name.data, name.size) == 0)
        {
          return tml_set_error(p, "duplicate name in scope");
        }
      }

      node_index = node->next_sibling;
    }

    if (parent != TML_INVALID_INDEX)
    {
      entry_index = p->doc->nodes[parent].first_entry;

      while (entry_index != TML_INVALID_INDEX)
      {
        TMLEntry* entry;

        entry = p->doc->entries + entry_index;

        if (entry->name.size == name.size)
        {
          if (memcmp(entry->name.data, name.data, name.size) == 0)
          {
            return tml_set_error(p, "duplicate name in scope");
          }
        }

        entry_index = entry->next_entry;
      }
    }

    return 1;
  }

  static void tml_link_node(TMLParser* p, u32 parent, u32 node_index)
  {
    TMLNode* node = NULL;

    if (p->mode != TML_PARSE_WRITE)
    {
      return;
    }

    node = p->doc->nodes + node_index;

    if (parent == TML_INVALID_INDEX)
    {
      if (p->doc->last_root_node == TML_INVALID_INDEX)
      {
        p->doc->first_root_node = node_index;
        p->doc->last_root_node = node_index;
      }
      else
      {
        p->doc->nodes[p->doc->last_root_node].next_sibling = node_index;
        p->doc->last_root_node = node_index;
      }

      p->doc->root_node_count += 1u;
      return;
    }

    if (p->doc->nodes[parent].last_child == TML_INVALID_INDEX)
    {
      p->doc->nodes[parent].first_child = node_index;
      p->doc->nodes[parent].last_child = node_index;
    }
    else
    {
      p->doc->nodes[p->doc->nodes[parent].last_child].next_sibling = node_index;
      p->doc->nodes[parent].last_child = node_index;
    }

    p->doc->nodes[parent].child_count += 1u;
    node->parent = parent;
  }

  static void tml_link_entry(TMLParser* p, u32 parent, u32 entry_index)
  {
    if (p->mode != TML_PARSE_WRITE || parent == TML_INVALID_INDEX)
    {
      return;
    }

    if (p->doc->nodes[parent].last_entry == TML_INVALID_INDEX)
    {
      p->doc->nodes[parent].first_entry = entry_index;
      p->doc->nodes[parent].last_entry = entry_index;
    }
    else
    {
      p->doc->entries[p->doc->nodes[parent].last_entry].next_entry = entry_index;
      p->doc->nodes[parent].last_entry = entry_index;
    }

    p->doc->nodes[parent].entry_count += 1u;
  }

  static u32 tml_current_parent(TMLParser* p, u32 indent)
  {
    while (p->scope_count > 0)
    {
      if (p->scopes[p->scope_count - 1u].indent < indent)
      {
        break;
      }

      p->scope_count -= 1u;
    }

    if (p->scope_count == 0)
    {
      return TML_INVALID_INDEX;
    }

    u32 parent = p->scopes[p->scope_count - 1u].node;
    return parent;
  }

  static int tml_push_scope(TMLParser* p, u32 node, u32 indent)
  {
    if (p->scope_count >= 256u)
    {
      return tml_set_error(p, "maximum nesting depth exceeded");
    }

    p->scopes[p->scope_count].node = node;
    p->scopes[p->scope_count].indent = indent;
    p->scope_count += 1u;

    return 1;
  }

  static int tml_parse_entry_value(TMLParser* p, TMLEntry* entry)
  {
    u32 count = 0;
    TMLArray array = {0};

    char const* value_start = p->cursor;
    char const* save_line_start = p->line_start;
    u32 save_line = p->line;

    TMLScalar scalar;
    if (!tml_parse_scalar_no_emit(p, &scalar))
    {
      return 0;
    }

    TMLScalarKind kind = scalar.kind;
    int is_array = tml_next_is_comma_before_comment_or_eol(p);

    if (!is_array)
    {
      p->cursor = value_start;
      p->line_start = save_line_start;
      p->line = save_line;

      if (!tml_parse_scalar(p, &scalar))
      {
        return 0;
      }

      if (kind == TML_SCALAR_BOOL)
      {
        entry->type = TML_VALUE_BOOL;
        entry->boolean = scalar.boolean;
      }
      else if (kind == TML_SCALAR_I64)
      {
        entry->type = TML_VALUE_I64;
        entry->integer = scalar.integer;
      }
      else if (kind == TML_SCALAR_F64)
      {
        entry->type = TML_VALUE_F64;
        entry->number = scalar.number;
      }
      else
      {
        entry->type = TML_VALUE_STRING;
        entry->string = scalar.string;
      }

      return tml_consume_line_end(p);
    }

    p->cursor = value_start;
    p->line_start = save_line_start;
    p->line = save_line;

    if (!tml_parse_array_count_only(p, kind, &count))
    {
      return 0;
    }

    if (!tml_consume_line_end(p))
    {
      return 0;
    }

    if (kind == TML_SCALAR_I64)
    {
      entry->type = TML_VALUE_ARRAY_I64;
      array = tml_emit_array_i64(p, count);
    }
    else if (kind == TML_SCALAR_F64)
    {
      entry->type = TML_VALUE_ARRAY_F64;
      array = tml_emit_array_f64(p, count);
    }
    else if (kind == TML_SCALAR_STRING)
    {
      entry->type = TML_VALUE_ARRAY_STRING;
      array = tml_emit_array_string(p, count);
    }
    else
    {
      return tml_set_error(p, "bool arrays are not supported");
    }

    entry->array = array;

    if (p->mode == TML_PARSE_COUNT && kind == TML_SCALAR_STRING)
    {
      p->cursor = value_start;
      p->line_start = save_line_start;
      p->line = save_line;

      if (!tml_parse_array_fill_string(p, array))
      {
        return 0;
      }

      if (!tml_consume_line_end(p))
      {
        return 0;
      }
    }

    if (p->mode == TML_PARSE_WRITE)
    {
      p->cursor = value_start;
      p->line_start = save_line_start;
      p->line = save_line;

      if (kind == TML_SCALAR_I64)
      {
        if (!tml_parse_array_fill_i64(p, array))
        {
          return 0;
        }
      }
      else if (kind == TML_SCALAR_F64)
      {
        if (!tml_parse_array_fill_f64(p, array))
        {
          return 0;
        }
      }
      else
      {
        if (!tml_parse_array_fill_string(p, array))
        {
          return 0;
        }
      }

      if (!tml_consume_line_end(p))
      {
        return 0;
      }
    }

    return 1;
  }

  static int tml_parse_entry(TMLParser* p, u32 parent, TMLString name)
  {
    if (parent == TML_INVALID_INDEX)
    {
      return tml_set_error(p, "top-level entries are not allowed");
    }

    if (identifier_reserved(name))
    {
      return tml_set_error(p, "reserved identifier cannot be used as name");
    }

    if (!tml_validate_unique_name(p, parent, name))
    {
      return 0;
    }

    TMLEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.name = tml_emit_identifier(p, name);
    entry.parent = parent;
    entry.next_entry = TML_INVALID_INDEX;

    if (!tml_parse_entry_value(p, &entry))
    {
      return 0;
    }

    u32 entry_index = tml_emit_entry(p, &entry);
    tml_link_entry(p, parent, entry_index);

    return 1;
  }

  static int tml_parse_named_statement(TMLParser* p, u32 indent, u32 parent)
  {
    TMLString name = tml_read_identifier_view(p);

    if (name.size == 0)
    {
      return tml_set_error(p, "expected identifier");
    }

    if (identifier_reserved(name))
    {
      return tml_set_error(p, "reserved identifier cannot be used as name");
    }

    tml_skip_spaces(p);

    if (*p->cursor != ':')
    {
      return tml_set_error(p, "expected ':'");
    }

    p->cursor += 1;
    tml_skip_spaces(p);

    if (tml_at_line_end_after_space_or_comment(p))
    {
      if (!tml_validate_unique_name(p, parent, name))
      {
        return 0;
      }

      TMLNode node;
      memset(&node, 0, sizeof(node));
      node.name = tml_emit_identifier(p, name);
      node.parent = parent;
      node.next_sibling = TML_INVALID_INDEX;
      node.first_child = TML_INVALID_INDEX;
      node.last_child = TML_INVALID_INDEX;
      node.first_entry = TML_INVALID_INDEX;
      node.last_entry = TML_INVALID_INDEX;

      u32 node_index = tml_emit_node(p, &node);
      tml_link_node(p, parent, node_index);

      if (!tml_consume_line_end(p))
      {
        return 0;
      }

      return tml_push_scope(p, node_index, indent);
    }

    return tml_parse_entry(p, parent, name);
  }

  static int tml_parse_anonymous_node(TMLParser* p, u32 indent, u32 parent)
  {
    if (parent == TML_INVALID_INDEX)
    {
      return tml_set_error(p, "anonymous node cannot appear at top-level");
    }

    p->cursor += 1;

    if (*p->cursor != ' ')
    {
      return tml_set_error(p, "expected space after anonymous node marker");
    }

    tml_skip_spaces(p);

    TMLNode node;
    memset(&node, 0, sizeof(node));
    node.name.data = NULL;
    node.name.size = 0;
    node.parent = parent;
    node.next_sibling = TML_INVALID_INDEX;
    node.first_child = TML_INVALID_INDEX;
    node.last_child = TML_INVALID_INDEX;
    node.first_entry = TML_INVALID_INDEX;
    node.last_entry = TML_INVALID_INDEX;

    u32 node_index = tml_emit_node(p, &node);
    tml_link_node(p, parent, node_index);

    if (!tml_push_scope(p, node_index, indent))
    {
      return 0;
    }

    TMLString first_entry_name = tml_read_identifier_view(p);

    if (first_entry_name.size == 0)
    {
      return tml_set_error(p, "anonymous node must start with key:value entry");
    }

    tml_skip_spaces(p);

    if (*p->cursor != ':')
    {
      return tml_set_error(p, "expected ':' after anonymous node first entry");
    }

    p->cursor += 1;
    tml_skip_spaces(p);

    if (tml_at_line_end_after_space_or_comment(p))
    {
      return tml_set_error(p, "anonymous node first entry must have a value");
    }

    return tml_parse_entry(p, node_index, first_entry_name);
  }

  static int tml_read_indent(TMLParser* p, u32* out_indent, int* out_blank)
  {
    u32 indent = 0;
    *out_blank = 0;

    while (*p->cursor == ' ')
    {
      indent += 1u;
      p->cursor += 1;
    }

    if (*p->cursor == '\t')
    {
      return tml_set_error(p, "tabs are invalid for indentation");
    }

    if (*p->cursor == '#' || *p->cursor == '\r' || *p->cursor == '\n' || *p->cursor == '\0')
    {
      *out_indent = indent;
      *out_blank = 1;
      return 1;
    }

    if (indent > 0)
    {
      if (p->indent_unit == 0)
      {
        if (indent == 2u || indent == 4u)
        {
          p->indent_unit = indent;
        }
        else
        {
          return tml_set_error(p, "first indentation must be 2 or 4 spaces");
        }
      }
      else if ((indent % p->indent_unit) != 0u)
      {
        return tml_set_error(p, "indentation must be a multiple of the indent unit");
      }
    }

    *out_indent = indent;
    return 1;
  }

  static int tml_parse_document_internal(TMLParser* p)
  {
    while (*p->cursor != '\0')
    {
      u32 indent;
      u32 parent;
      int blank;

      if (!tml_read_indent(p, &indent, &blank))
      {
        return 0;
      }

      if (blank)
      {
        if (!tml_consume_line_end(p))
        {
          return 0;
        }

        continue;
      }

      parent = tml_current_parent(p, indent);

      if (indent > 0)
      {
        if (parent == TML_INVALID_INDEX)
        {
          return tml_set_error(p, "indented statement has no parent node");
        }

        if (p->indent_unit > 0)
        {
          if (p->scopes[p->scope_count - 1u].indent + p->indent_unit != indent)
          {
            return tml_set_error(p, "indentation jumped more than one level");
          }
        }
      }

      if (*p->cursor == '-')
      {
        if (!tml_parse_anonymous_node(p, indent, parent))
        {
          return 0;
        }
      }
      else
      {
        if (!tml_parse_named_statement(p, indent, parent))
        {
          return 0;
        }
      }
    }

    return 1;
  }

  static void tml_parser_init(TMLParser* p, char const* source, TMLParseMode mode, TMLDocument* doc)
  {
    memset(p, 0, sizeof(*p));
    p->source = source;
    p->cursor = source;
    p->line_start = source;
    p->line = 1;
    p->mode = mode;
    p->doc = doc;
  }

  static int tml_string_equals_range(TMLString string, char const* data, u32 size)
  {
    if (string.size != size)
    {
      return 0;
    }

    if (size == 0)
    {
      return 1;
    }

    return memcmp(string.data, data, size) == 0;
  }

  static int tml_path_segment_is_index(char const* data, u32 size, u32* out_index)
  {
    u32 index = 0;
    u32 i = 0;

    if (data == NULL || size == 0 || out_index == NULL)
    {
      return 0;
    }

    while (i < size)
    {
      char c = data[i];

      if (c < '0' || c > '9')
      {
        return 0;
      }

      index = (index * 10u) + (u32)(c - '0');
      i += 1u;
    }

    *out_index = index;
    return 1;
  }

  static TMLNode const* tml_root_node_find_child_range(TMLDocument const* doc, char const* name, u32 name_size)
  {
    u32 node_index = TML_INVALID_INDEX;

    if (doc == NULL || name == NULL)
    {
      return NULL;
    }

    node_index = doc->first_root_node;

    while (node_index != TML_INVALID_INDEX)
    {
      TMLNode const* child = doc->nodes + node_index;

      if (tml_string_equals_range(child->name, name, name_size))
      {
        return child;
      }

      node_index = child->next_sibling;
    }

    return NULL;
  }

  static TMLNode const* tml_root_node_child_at(TMLDocument const* doc, u32 index)
  {
    u32 node_index = TML_INVALID_INDEX;
    u32 i = 0;

    if (doc == NULL)
    {
      return NULL;
    }

    node_index = doc->first_root_node;

    while (node_index != TML_INVALID_INDEX)
    {
      if (i == index)
      {
        return doc->nodes + node_index;
      }

      node_index = doc->nodes[node_index].next_sibling;
      i += 1u;
    }

    return NULL;
  }

  static TMLNode const* tml_node_find_child_range(TMLDocument const* doc, TMLNode const* node, char const* name, u32 name_size)
  {
    u32 node_index = TML_INVALID_INDEX;

    if (doc == NULL || node == NULL || name == NULL)
    {
      return NULL;
    }

    node_index = node->first_child;

    while (node_index != TML_INVALID_INDEX)
    {
      TMLNode const* child = doc->nodes + node_index;

      if (tml_string_equals_range(child->name, name, name_size))
      {
        return child;
      }

      node_index = child->next_sibling;
    }

    return NULL;
  }

  static TMLEntry const* tml_node_find_entry_range(TMLDocument const* doc, TMLNode const* node, char const* name, u32 name_size)
  {
    u32 entry_index = TML_INVALID_INDEX;

    if (doc == NULL || node == NULL || name == NULL)
    {
      return NULL;
    }

    entry_index = node->first_entry;

    while (entry_index != TML_INVALID_INDEX)
    {
      TMLEntry const* entry = doc->entries + entry_index;

      if (tml_string_equals_range(entry->name, name, name_size))
      {
        return entry;
      }

      entry_index = entry->next_entry;
    }

    return NULL;
  }

  static TMLEntry const* tml_node_entry_at_internal(TMLDocument const* doc, TMLNode const* node, u32 index)
  {
    u32 entry_index = TML_INVALID_INDEX;
    u32 i = 0;

    if (doc == NULL || node == NULL)
    {
      return NULL;
    }

    entry_index = node->first_entry;

    while (entry_index != TML_INVALID_INDEX)
    {
      if (i == index)
      {
        return doc->entries + entry_index;
      }

      entry_index = doc->entries[entry_index].next_entry;
      i += 1u;
    }

    return NULL;
  }

  static TMLDocument* tml_document_alloc(TMLStats stats)
  {
    u64 offset;
    u64 size;
    char* memory;
    TMLDocument* doc;

    offset = sizeof(TMLDocument);

    offset = tml_align_forward_u64(offset, 8u);
    offset += sizeof(TMLNode) * stats.node_count;

    offset = tml_align_forward_u64(offset, 8u);
    offset += sizeof(TMLEntry) * stats.entry_count;

    offset = tml_align_forward_u64(offset, 8u);
    offset += sizeof(char) * stats.string_size;

    offset = tml_align_forward_u64(offset, 8u);
    offset += sizeof(i64) * stats.array_i64_count;

    offset = tml_align_forward_u64(offset, 8u);
    offset += sizeof(f64) * stats.array_f64_count;

    offset = tml_align_forward_u64(offset, 8u);
    offset += sizeof(TMLString) * stats.array_string_count;

    size = offset;
    memory = (char*)X_TML_ALLOC((size_t)size);

    if (memory == NULL)
    {
      return NULL;
    }

    doc = (TMLDocument*)memory;
    doc->memory = memory;
    doc->memory_size = size;

    offset = sizeof(TMLDocument);

    offset = tml_align_forward_u64(offset, 8u);
    doc->nodes = (TMLNode*)(memory + offset);
    offset += sizeof(TMLNode) * stats.node_count;

    offset = tml_align_forward_u64(offset, 8u);
    doc->entries = (TMLEntry*)(memory + offset);
    offset += sizeof(TMLEntry) * stats.entry_count;

    offset = tml_align_forward_u64(offset, 8u);
    doc->string_data = (char*)(memory + offset);
    offset += sizeof(char) * stats.string_size;

    offset = tml_align_forward_u64(offset, 8u);
    doc->array_i64 = (i64*)(memory + offset);
    offset += sizeof(i64) * stats.array_i64_count;

    offset = tml_align_forward_u64(offset, 8u);
    doc->array_f64 = (f64*)(memory + offset);
    offset += sizeof(f64) * stats.array_f64_count;

    offset = tml_align_forward_u64(offset, 8u);
    doc->array_string = (TMLString*)(memory + offset);

    doc->node_count = stats.node_count;
    doc->entry_count = stats.entry_count;
    doc->string_size = stats.string_size;
    doc->array_i64_count = stats.array_i64_count;
    doc->array_f64_count = stats.array_f64_count;
    doc->array_string_count = stats.array_string_count;
    doc->first_root_node = TML_INVALID_INDEX;
    doc->last_root_node = TML_INVALID_INDEX;

    return doc;
  }

  X_TML_API void tml_document_free(TMLDocument* doc)
  {
    if (doc != NULL)
    {
      X_TML_FREE(doc->memory);
    }
  }

  X_TML_API TMLParseResult tml_parse(char const* source)
  {
    TMLParser parser;
    TMLParseResult result;

    memset(&result, 0, sizeof(result));

    if (source == NULL)
    {
      result.ok = 0;
      snprintf(result.error, sizeof(result.error), "source is NULL");
      result.line = 0;
      result.column = 0;
      return result;
    }

    tml_parser_init(&parser, source, TML_PARSE_COUNT, NULL);

    if (!tml_parse_document_internal(&parser))
    {
      result.ok = 0;
      snprintf(result.error, sizeof(result.error), "%s", parser.error);
      result.line = parser.error_line;
      result.column = parser.error_column;
      return result;
    }

    TMLDocument* doc = tml_document_alloc(parser.stats);

    if (doc == NULL)
    {
      result.ok = 0;
      snprintf(result.error, sizeof(result.error), "out of memory");
      return result;
    }

    tml_parser_init(&parser, source, TML_PARSE_WRITE, doc);

    if (!tml_parse_document_internal(&parser))
    {
      result.ok = 0;
      snprintf(result.error, sizeof(result.error), "%s", parser.error);
      result.line = parser.error_line;
      result.column = parser.error_column;
      tml_document_free(doc);
      return result;
    }

    result.ok = 1;
    result.document = doc;
    return result;
  }

  X_TML_API TMLNode const* tml_node_at(TMLDocument const* doc, u32 index)
  {
    if (doc == NULL || index >= doc->node_count)
    {
      return NULL;
    }

    return doc->nodes + index;
  }

  X_TML_API TMLEntry const* tml_entry_at(TMLDocument const* doc, u32 index)
  {
    if (doc == NULL || index >= doc->entry_count)
    {
      return NULL;
    }

    return doc->entries + index;
  }

  X_TML_API TMLNode const* tml_root_node_at(TMLDocument const* doc, u32 index)
  {
    if (doc == NULL)
    {
      return NULL;
    }

    u32 node_index = doc->first_root_node;
    u32 i = 0;

    while (node_index != TML_INVALID_INDEX)
    {
      if (i == index)
      {
        return doc->nodes + node_index;
      }

      node_index = doc->nodes[node_index].next_sibling;
      i += 1u;
    }

    return NULL;
  }

  X_TML_API TMLNode const* tml_node_child_at(TMLDocument const* doc, TMLNode const* node, u32 index)
  {
    if (doc == NULL || node == NULL)
    {
      return NULL;
    }

    u32 node_index = node->first_child;
    u32 i = 0;

    while (node_index != TML_INVALID_INDEX)
    {
      if (i == index)
      {
        return doc->nodes + node_index;
      }

      node_index = doc->nodes[node_index].next_sibling;
      i += 1u;
    }

    return NULL;
  }

  X_TML_API TMLEntry const* tml_node_entry_at(TMLDocument const* doc, TMLNode const* node, u32 index)
  {
    if (doc == NULL || node == NULL)
    {
      return NULL;
    }

    u32 entry_index = node->first_entry;
    u32 i = 0;

    while (entry_index != TML_INVALID_INDEX)
    {
      if (i == index)
      {
        return doc->entries + entry_index;
      }

      entry_index = doc->entries[entry_index].next_entry;
      i += 1u;
    }

    return NULL;
  }

  X_TML_API TMLNode const* tml_node_find_child(TMLDocument const* doc, TMLNode const* node, char const* name)
  {
    if (doc == NULL || node == NULL || name == NULL)
    {
      return NULL;
    }

    size_t len = strlen(name);
    u32 node_index = node->first_child;

    while (node_index != TML_INVALID_INDEX)
    {
      TMLNode const* child;

      child = doc->nodes + node_index;

      if (child->name.size == (u32)len)
      {
        if (memcmp(child->name.data, name, len) == 0)
        {
          return child;
        }
      }

      node_index = child->next_sibling;
    }

    return NULL;
  }

  X_TML_API TMLEntry const* tml_node_find_entry(TMLDocument const* doc, TMLNode const* node, char const* name)
  {
    if (doc == NULL || node == NULL || name == NULL)
    {
      return NULL;
    }

    size_t len = strlen(name);
    u32 entry_index = node->first_entry;

    while (entry_index != TML_INVALID_INDEX)
    {
      TMLEntry const* entry;

      entry = doc->entries + entry_index;

      if (entry->name.size == (u32)len)
      {
        if (memcmp(entry->name.data, name, len) == 0)
        {
          return entry;
        }
      }

      entry_index = entry->next_entry;
    }

    return NULL;
  }

  X_TML_API int tml_entry_get_bool(TMLEntry const* entry, u8* out_value)
  {
    if (entry == NULL || out_value == NULL || entry->type != TML_VALUE_BOOL)
    {
      return 0;
    }

    *out_value = entry->boolean;
    return 1;
  }

  X_TML_API int tml_entry_get_i64(TMLEntry const* entry, i64* out_value)
  {
    if (entry == NULL || out_value == NULL || entry->type != TML_VALUE_I64)
    {
      return 0;
    }

    *out_value = entry->integer;
    return 1;
  }

  X_TML_API int tml_entry_get_f64(TMLEntry const* entry, f64* out_value)
  {
    if (entry == NULL || out_value == NULL || entry->type != TML_VALUE_F64)
    {
      return 0;
    }

    *out_value = entry->number;
    return 1;
  }

  X_TML_API int tml_entry_get_string(TMLEntry const* entry, TMLString* out_value)
  {
    if (entry == NULL || out_value == NULL || entry->type != TML_VALUE_STRING)
    {
      return 0;
    }

    *out_value = entry->string;
    return 1;
  }

  X_TML_API int tml_entry_get_i64_array(TMLDocument const* doc, TMLEntry const* entry, TMLI64Slice* out_value)
  {
    if (doc == NULL || entry == NULL || out_value == NULL || entry->type != TML_VALUE_ARRAY_I64)
    {
      return 0;
    }

    out_value->data = doc->array_i64 + entry->array.first;
    out_value->count = entry->array.count;
    return 1;
  }

  X_TML_API int tml_entry_get_f64_array(TMLDocument const* doc, TMLEntry const* entry, TMLF64Slice* out_value)
  {
    if (doc == NULL || entry == NULL || out_value == NULL || entry->type != TML_VALUE_ARRAY_F64)
    {
      return 0;
    }

    out_value->data = doc->array_f64 + entry->array.first;
    out_value->count = entry->array.count;
    return 1;
  }

  X_TML_API int tml_entry_get_string_array(TMLDocument const* doc, TMLEntry const* entry, TMLStringSlice* out_value)
  {
    if (doc == NULL || entry == NULL || out_value == NULL || entry->type != TML_VALUE_ARRAY_STRING)
    {
      return 0;
    }

    out_value->data = doc->array_string + entry->array.first;
    out_value->count = entry->array.count;
    return 1;
  }

  X_TML_API int tml_node_get_bool(TMLDocument const* doc, TMLNode const* node, char const* name, u8* out_value)
  {
    TMLEntry const* entry = tml_node_find_entry(doc, node, name);
    return tml_entry_get_bool(entry, out_value);
  }

  X_TML_API int tml_node_get_i64(TMLDocument const* doc, TMLNode const* node, char const* name, i64* out_value)
  {
    TMLEntry const* entry = tml_node_find_entry(doc, node, name);
    return tml_entry_get_i64(entry, out_value);
  }

  X_TML_API int tml_node_get_f64(TMLDocument const* doc, TMLNode const* node, char const* name, f64* out_value)
  {
    TMLEntry const* entry = tml_node_find_entry(doc, node, name);
    return tml_entry_get_f64(entry, out_value);
  }

  X_TML_API int tml_node_get_string(TMLDocument const* doc, TMLNode const* node, char const* name, TMLString* out_value)
  {
    TMLEntry const* entry = tml_node_find_entry(doc, node, name);
    return tml_entry_get_string(entry, out_value);
  }

  X_TML_API int tml_node_get_i64_array(TMLDocument const* doc, TMLNode const* node, char const* name, TMLI64Slice* out_value)
  {
    TMLEntry const* entry = tml_node_find_entry(doc, node, name);
    return tml_entry_get_i64_array(doc, entry, out_value);
  }

  X_TML_API int tml_node_get_f64_array(TMLDocument const* doc, TMLNode const* node, char const* name, TMLF64Slice* out_value)
  {
    TMLEntry const* entry = tml_node_find_entry(doc, node, name);
    return tml_entry_get_f64_array(doc, entry, out_value);
  }

  X_TML_API int tml_node_get_string_array(TMLDocument const* doc, TMLNode const* node, char const* name, TMLStringSlice* out_value)
  {
    TMLEntry const* entry = tml_node_find_entry(doc, node, name);
    return tml_entry_get_string_array(doc, entry, out_value);
  }

  X_TML_API TMLNode const* tml_path_find_node(TMLDocument const* doc, char const* path)
  {
    TMLNode const* node = NULL;
    char const* segment = path;

    if (doc == NULL || path == NULL || *path == '\0')
    {
      return NULL;
    }

    while (*segment != '\0')
    {
      char const* end = segment;
      u32 segment_size = 0;
      u32 index = 0;

      while (*end != '\0' && *end != '.')
      {
        end += 1;
      }

      segment_size = (u32)(end - segment);

      if (segment_size == 0)
      {
        return NULL;
      }

      if (tml_path_segment_is_index(segment, segment_size, &index))
      {
        if (node == NULL)
        {
          node = tml_root_node_child_at(doc, index);
        }
        else
        {
          node = tml_node_child_at(doc, node, index);
        }
      }
      else
      {
        if (node == NULL)
        {
          node = tml_root_node_find_child_range(doc, segment, segment_size);
        }
        else
        {
          node = tml_node_find_child_range(doc, node, segment, segment_size);
        }
      }

      if (node == NULL)
      {
        return NULL;
      }

      if (*end == '\0')
      {
        return node;
      }

      segment = end + 1;
    }

    return node;
  }

  X_TML_API TMLEntry const* tml_path_find_entry(TMLDocument const* doc, char const* path)
  {
    TMLNode const* node = NULL;
    char const* segment = path;

    if (doc == NULL || path == NULL || *path == '\0')
    {
      return NULL;
    }

    while (*segment != '\0')
    {
      char const* end = segment;
      u32 segment_size = 0;
      u32 index = 0;
      int is_last = 0;

      while (*end != '\0' && *end != '.')
      {
        end += 1;
      }

      segment_size = (u32)(end - segment);
      is_last = *end == '\0';

      if (segment_size == 0)
      {
        return NULL;
      }

      if (is_last)
      {
        if (node == NULL)
        {
          return NULL;
        }

        if (tml_path_segment_is_index(segment, segment_size, &index))
        {
          return tml_node_entry_at_internal(doc, node, index);
        }

        return tml_node_find_entry_range(doc, node, segment, segment_size);
      }

      if (tml_path_segment_is_index(segment, segment_size, &index))
      {
        if (node == NULL)
        {
          node = tml_root_node_child_at(doc, index);
        }
        else
        {
          node = tml_node_child_at(doc, node, index);
        }
      }
      else
      {
        if (node == NULL)
        {
          node = tml_root_node_find_child_range(doc, segment, segment_size);
        }
        else
        {
          node = tml_node_find_child_range(doc, node, segment, segment_size);
        }
      }

      if (node == NULL)
      {
        return NULL;
      }

      segment = end + 1;
    }

    return NULL;
  }

  X_TML_API int tml_path_get_bool(TMLDocument const* doc, char const* path, u8* out_value)
  {
    TMLEntry const* entry = tml_path_find_entry(doc, path);
    return tml_entry_get_bool(entry, out_value);
  }

  X_TML_API int tml_path_get_i64(TMLDocument const* doc, char const* path, i64* out_value)
  {
    TMLEntry const* entry = tml_path_find_entry(doc, path);
    return tml_entry_get_i64(entry, out_value);
  }

  X_TML_API int tml_path_get_f64(TMLDocument const* doc, char const* path, f64* out_value)
  {
    TMLEntry const* entry = tml_path_find_entry(doc, path);
    return tml_entry_get_f64(entry, out_value);
  }

  X_TML_API int tml_path_get_string(TMLDocument const* doc, char const* path, TMLString* out_value)
  {
    TMLEntry const* entry = tml_path_find_entry(doc, path);
    return tml_entry_get_string(entry, out_value);
  }

  X_TML_API int tml_path_get_i64_array(TMLDocument const* doc, char const* path, TMLI64Slice* out_value)
  {
    TMLEntry const* entry = tml_path_find_entry(doc, path);
    return tml_entry_get_i64_array(doc, entry, out_value);
  }

  X_TML_API int tml_path_get_f64_array(TMLDocument const* doc, char const* path, TMLF64Slice* out_value)
  {
    TMLEntry const* entry = tml_path_find_entry(doc, path);
    return tml_entry_get_f64_array(doc, entry, out_value);
  }

  X_TML_API int tml_path_get_string_array(TMLDocument const* doc, char const* path, TMLStringSlice* out_value)
  {
    TMLEntry const* entry = tml_path_find_entry(doc, path);
    return tml_entry_get_string_array(doc, entry, out_value);
  }

#ifdef __cplusplus
}
#endif

#endif //X_IMPL_TML
#endif //STDX_TML_H
