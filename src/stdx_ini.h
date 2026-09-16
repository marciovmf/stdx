/**
 * STDX - Minimal INI parser
 * Part of the STDX General Purpose C Library by marciovmf
 * License: MIT
 * <https://github.com/marciovmf/stdx>
 *
 * ## Overview
 *
 * One flat string pool owns all normalized C strings (section names, keys, values).
 * Sections and entries are indexed by arrays for direct and iterative access.
 *
 * ## How to compile
 *
 * To compile the implementation, define `X_IMPL_INI`
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * `X_INI_ALLOC` / `X_INI_REALLOC` / `X_INI_FREE` before including.
 *
 */

#ifndef X_INI_H
#define X_INI_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef X_INI_API
#define X_INI_API
#endif

#define X_INI_VERSION_MAJOR 2
#define X_INI_VERSION_MINOR 0
#define X_INI_VERSION_PATCH 0
#define X_INI_VERSION (X_INI_VERSION_MAJOR*10000 + X_INI_VERSION_MINOR*100 + X_INI_VERSION_PATCH)

/* Tuning */
#define X_INI_DEFAULT_SECTIONS_CAP  16
#define X_INI_DEFAULT_ENTRIES_CAP   64
#define X_INI_MAX_LINE              4096

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ini parsing status.
 */
typedef enum XIniErrorCode
{
  XINI_OK = 0,
  XINI_ERR_IO,
  XINI_ERR_MEMORY,
  XINI_ERR_SYNTAX,
  XINI_ERR_EXPECT_EQUALS,
  XINI_ERR_EXPECT_RBRACKET,
  XINI_ERR_UNTERMINATED_STRING
} XIniErrorCode;

/**
 * @brief Ini error information
 */
typedef struct XIniError
{
  XIniErrorCode code;  /* error kind */
  int line;            /* 1-based line number where error was detected */
  int column;          /* 1-based column of the offending character/token */
  const char *message; /* short, static message string */
} XIniError;


/**
 * @brief Ini section information
 */
typedef struct XIniSection
{
  const char *name;   /* section name or "" for the global section */
  int first_entry;    /* index of first entry for this section, or -1 if none */
} XIniSection;

/**
 * @brief Ini Entry
 */
typedef struct XIniEntry
{
  int section;        /* section index this entry belongs to */
  const char *key;    /* normalized key */
  const char *value;  /* normalized value (may be empty string) */
} XIniEntry;


/**
 * @brief Represents a parsed INI source
 */
typedef struct XIni
{
  /* string pool */
  char  *pool;
  size_t pool_size;
  size_t pool_cap;
  /* sections and entries */
  XIniSection *sections;
  int          sections_count;
  int          sections_cap;
  XIniEntry   *entries;
  int          entries_count;
  int          entries_cap;
  /* bookkeeping */
  int          global_section; // index of the synthetic global section ("")
} XIni;

/**
* @brief Load and parse an INI file from disk.
* @param path Path to the INI file.
* @param out_ini Output structure receiving the parsed INI data.
* @param err Optional output error information (may be NULL).
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_load_file(const char *path, XIni *out_ini, XIniError *err);

/**
* @brief Load and parse INI data from a memory buffer.
* @param data Pointer to the INI data in memory.
* @param size Size of the data buffer in bytes.
* @param out_ini Output structure receiving the parsed INI data.
* @param err Optional output error information (may be NULL).
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_load_mem(const void *data, size_t size, XIni *out_ini, XIniError *err);

/**
* @brief Free all resources associated with an INI structure.
* @param ini INI structure to free.
* @return Nothing.
*/
X_INI_API void x_ini_free(XIni *ini);

/**
* @brief Get a human-readable string for an INI error code.
* @param code Error code.
* @return Pointer to a static, thread-safe error string.
*/
X_INI_API const char* x_ini_err_str(XIniErrorCode code);

/**
* @brief Retrieve a string value from the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param def_value Default value returned if the key is not found.
* @return Pointer to the value string, or def_value if not found.
*/
X_INI_API const char* x_ini_get(const XIni *ini, const char *section, const char *key, const char *def_value);

/**
* @brief Retrieve a 32-bit integer value from the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param def_value Default value returned if the key is not found or cannot be parsed.
* @return Parsed integer value, or def_value on failure.
*/
X_INI_API int32_t x_ini_get_i32(const XIni *ini, const char *section, const char *key, int32_t def_value);

/**
* @brief Retrieve an unsigne 32-bit integer value from the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param def_value Default value returned if the key is not found or cannot be parsed.
* @return Parsed integer value, or def_value on failure.
*/
 X_INI_API uint32_t x_ini_get_u32(const XIni *ini, const char *section, const char *key, uint32_t def_value);

/**
* @brief Retrieve a 32-bit floating-point value from the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param def_value Default value returned if the key is not found or cannot be parsed.
* @return Parsed floating-point value, or def_value on failure.
*/
X_INI_API float x_ini_get_f32(const XIni *ini, const char *section, const char *key, float def_value);

/**
* @brief Retrieve a boolean value from the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param def_value Default value returned if the key is not found or cannot be parsed.
* @return Parsed boolean value, or def_value on failure.
*/
X_INI_API bool x_ini_get_bool(const XIni *ini, const char *section, const char *key, bool def_value);

/**
* @brief Set a string value in the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param value Value string.
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_set(XIni *ini, const char *section, const char *key, const char *value);

/**
* @brief Set a 32-bit integer value in the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param value Integer value.
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_set_i32(XIni *ini, const char *section, const char *key, int32_t value);

/**
* @brief Set an unsigned 32-bit integer value in the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param value Integer value.
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_set_u32(XIni *ini, const char *section, const char *key, uint32_t value);

/**
* @brief Set an unsigned 32-bit integer value in hexadecimal form in the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param value Integer value.
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_set_u32_hex(XIni *ini, const char *section, const char *key, uint32_t value);

/**
* @brief Set a 32-bit floating-point value in the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param value Floating-point value.
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_set_f32(XIni *ini, const char *section, const char *key, float value);

/**
* @brief Set a boolean value in the INI data.
* @param ini Parsed INI data.
* @param section Section name.
* @param key Key name.
* @param value Boolean value.
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_set_bool(XIni *ini, const char *section, const char *key, bool value);

/**
* @brief Write INI data to a file.
* @param path Path to the INI file.
* @param ini INI data to write.
* @param err Optional output error information (may be NULL).
* @return True on success, false on failure.
*/
X_INI_API bool x_ini_write_file(const char *path, const XIni *ini, XIniError *err);

/**
* @brief Get the number of sections in the INI data.
* @param ini Parsed INI data.
* @return Number of sections.
*/
X_INI_API int x_ini_section_count(const XIni *ini);

/**
* @brief Get the name of a section by index.
* @param ini Parsed INI data.
* @param section_index Zero-based section index.
* @return Section name string.
*/
X_INI_API const char* x_ini_section_name(const XIni *ini, int section_index);

/**
* @brief Get the number of keys in a section.
* @param ini Parsed INI data.
* @param section_index Zero-based section index.
* @return Number of keys in the section.
*/
X_INI_API int x_ini_key_count(const XIni *ini, int section_index);

/**
* @brief Get the name of a key by section and key index.
* @param ini Parsed INI data.
* @param section_index Zero-based section index.
* @param key_index Zero-based key index within the section.
* @return Key name string.
*/
X_INI_API const char* x_ini_key_name(const XIni *ini, int section_index, int key_index);

/**
* @brief Get the value of a key by section and key index.
* @param ini Parsed INI data.
* @param section_index Zero-based section index.
* @param key_index Zero-based key index within the section.
* @return Value string at the specified position.
*/
X_INI_API const char* x_ini_value_at(const XIni *ini, int section_index, int key_index);

#ifdef __cplusplus
} // extern "C"
#endif


#ifdef X_IMPL_INI

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Allocator override */

#ifndef X_INI_ALLOC
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_INI_ALLOC(sz)        malloc(sz)
#endif

#ifndef X_INI_REALLOC
/**
 * @brief Internal macro for resizing memory.
 * To override how this header resizes memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to resize.
 */
#define X_INI_REALLOC(p,sz)    realloc((p),(sz))
#endif

#ifndef X_INI_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_INI_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- internal helpers ---------------- */

static void s_x_set_err(XIniError *err, XIniErrorCode code, int line, int column, const char *msg)
{
  if (err)
  {
    err->code = code;
    err->line = line;
    err->column = column;
    err->message = msg;
  }
}

static const char *s_x_err_msg(XIniErrorCode code)
{
  switch (code)
  {
    case XINI_OK:                  return "ok";
    case XINI_ERR_IO:              return "i/o error";
    case XINI_ERR_MEMORY:          return "out of memory";
    case XINI_ERR_SYNTAX:          return "syntax error";
    case XINI_ERR_EXPECT_EQUALS:   return "expected '='";
    case XINI_ERR_EXPECT_RBRACKET: return "expected ']'";
    case XINI_ERR_UNTERMINATED_STRING: return "unterminated string";
    default:                       return "unknown error";
  }
}

static void *s_x_realloc_grow(void *ptr, size_t elem_size, int *cap)
{
  int new_cap = (*cap > 0) ? (*cap * 2) : 8;
  size_t bytes = (size_t)new_cap * elem_size;
  void *np = X_INI_REALLOC(ptr, bytes);
  if (!np) return NULL;
  *cap = new_cap;
  return np;
}

static bool s_x_pool_init(XIni *ini, size_t cap)
{
  ini->pool = (char *)X_INI_ALLOC(cap);
  if (!ini->pool) return false;
  ini->pool_size = 0;
  ini->pool_cap = cap;
  return true;
}

static bool s_x_pool_reserve(XIni *ini, size_t additional)
{
  if (ini->pool_size + additional <= ini->pool_cap) return true;

  size_t ncap = ini->pool_cap ? ini->pool_cap * 2 : 1024;
  while (ini->pool_size + additional > ncap) ncap *= 2;

  char *old_pool = ini->pool;
  char *np = (char *)X_INI_ALLOC(ncap);
  if (!np) return false;

  memcpy(np, old_pool, ini->pool_size);

  for (int i = 0; i < ini->sections_count; ++i)
  {
    if (ini->sections[i].name)
    {
      ini->sections[i].name = np + (ini->sections[i].name - old_pool);
    }
  }

  for (int i = 0; i < ini->entries_count; ++i)
  {
    if (ini->entries[i].key)
    {
      ini->entries[i].key = np + (ini->entries[i].key - old_pool);
    }
    if (ini->entries[i].value)
    {
      ini->entries[i].value = np + (ini->entries[i].value - old_pool);
    }
  }

  X_INI_FREE(old_pool);
  ini->pool = np;
  ini->pool_cap = ncap;
  return true;
}

static const char *s_x_pool_add(XIni *ini, const char *src, size_t len)
{
  size_t src_offset = 0;
  bool src_in_pool = false;
  uintptr_t src_addr = (uintptr_t)src;
  uintptr_t pool_addr = (uintptr_t)ini->pool;
  if (src_addr >= pool_addr && src_addr < pool_addr + ini->pool_size)
  {
    src_offset = (size_t)(src_addr - pool_addr);
    src_in_pool = true;
  }

  if (!s_x_pool_reserve(ini, len + 1)) return NULL;
  if (src_in_pool) src = ini->pool + src_offset;

  char *dst = ini->pool + ini->pool_size;
  memcpy(dst, src, len);
  dst[len] = '\0';
  ini->pool_size += len + 1;
  return dst;
}

static const char *s_x_pool_add_cstr(XIni *ini, const char *s)
{
  return s_x_pool_add(ini, s, strlen(s));
}

static char *s_x_trim_inplace(char *s)
/* trims leading/trailing ASCII whitespace; returns pointer to first non-space */
{
  if (!s) return s;
  char *start = s;
  while (*start && isspace((unsigned char)*start)) { ++start; }

  if (*start == '\0') return start;

  char *end = start + strlen(start) - 1;
  while (end >= start && isspace((unsigned char)*end)) { *end-- = '\0'; }
  return start;
}

/* remove inline comments ';' or '#' found outside quotes */
static void s_x_chomp_inline_comment_unquoted(char *s)
{
  bool in_quote = false;
  for (char *p = s; *p; ++p)
  {
    if (*p == '\"')
    {
      /* toggle only if not escaped */
      bool escaped = (p > s && p[-1] == '\\');
      if (!escaped) in_quote = !in_quote;
    }
    if (!in_quote && (*p == ';' || *p == '#'))
    {
      *p = '\0';
      break;
    }
  }
}

static int s_x_find_section(const XIni *ini, const char *section)
{
  for (int i = 0; i < ini->sections_count; ++i)
  {
    if (strcmp(ini->sections[i].name, section) == 0) return i;
  }
  return -1;
}

static int s_x_count_keys_in_section(const XIni *ini, int section_index)
{
  int c = 0;
  for (int i = 0; i < ini->entries_count; ++i)
  {
    if (ini->entries[i].section == section_index) ++c;
  }
  return c;
}

static int s_x_stricmp(const char *a, const char *b)
{
  unsigned char ca, cb;
  while (*a && *b)
  {
    ca = (unsigned char)tolower((unsigned char)*a);
    cb = (unsigned char)tolower((unsigned char)*b);
    if (ca != cb) return (int)ca - (int)cb;
    ++a; ++b;
  }
  return (int)(unsigned char)tolower((unsigned char)*a)
       - (int)(unsigned char)tolower((unsigned char)*b);
}

/* decode a quoted string: handles \t \n \\ \" ; returns pointer in pool (no quotes) */
static const char *s_x_decode_quoted_into_pool(XIni *ini, const char *start_quote,
                                               int *out_error_col, bool *ok)
{
  *ok = false;
  const char *s = start_quote;
  if (*s != '\"') return NULL;
  ++s; /* skip opening quote */

  /* temporary dynamic buffer */
  size_t cap = 64, len = 0;
  char *tmp = (char *)X_INI_ALLOC(cap);
  if (!tmp) return NULL;

  int col = 1; /* relative column inside the quoted payload */

  while (*s)
  {
    char c = *s++;
    if (c == '\"')
    {
      /* end of string (unescaped quote) */
      const char *pooled = s_x_pool_add(ini, tmp, len);
      X_INI_FREE(tmp);
      *ok = (pooled != NULL);
      return pooled;
    }
    if (c == '\\')
    {
      char n = *s++;
      if (n == '\0') { *out_error_col = col; X_INI_FREE(tmp); return NULL; }
      switch (n)
      {
        case 'n':  c = '\n'; break;
        case 't':  c = '\t'; break;
        case '\\': c = '\\'; break;
        case '\"': c = '\"'; break;
        default:   c = n;    break; /* pass-through unknown escapes */
      }
      /* fallthrough with translated c */
    }
    /* push c */
    if (len + 1 >= cap)
    {
      size_t ncap = cap * 2;
      char *np = (char *)X_INI_REALLOC(tmp, ncap);
      if (!np) { X_INI_FREE(tmp); return NULL; }
      tmp = np; cap = ncap;
    }
    tmp[len++] = c;
    ++col;
  }

  /* if we got here, string was unterminated */
  *out_error_col = col;
  X_INI_FREE(tmp);
  return NULL;
}

/* ---------------- public implementation ---------------- */

X_INI_API const char* x_ini_err_str(XIniErrorCode code)
{
  return s_x_err_msg(code);
}

X_INI_API bool x_ini_load_mem(const void *data, size_t size, XIni *out_ini, XIniError *err)
{
  if (err) { err->code = XINI_OK; err->line = 0; err->column = 0; err->message = s_x_err_msg(XINI_OK); }
  if (!out_ini) { s_x_set_err(err, XINI_ERR_SYNTAX, 0, 0, "null output ini"); return false; }

  memset(out_ini, 0, sizeof(*out_ini));
  if (!s_x_pool_init(out_ini, size > 0 ? (size * 2) : 1024))
  {
    s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY));
    return false;
  }

  out_ini->sections = (XIniSection *)X_INI_ALLOC(sizeof(XIniSection) * X_INI_DEFAULT_SECTIONS_CAP);
  out_ini->entries  = (XIniEntry   *)X_INI_ALLOC(sizeof(XIniEntry)   * X_INI_DEFAULT_ENTRIES_CAP);
  if (!out_ini->sections || !out_ini->entries)
  {
    s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY));
    x_ini_free(out_ini);
    return false;
  }
  out_ini->sections_cap = X_INI_DEFAULT_SECTIONS_CAP;
  out_ini->entries_cap  = X_INI_DEFAULT_ENTRIES_CAP;

  /* global section ("") */
  const char *gs = s_x_pool_add_cstr(out_ini, "");
  if (!gs) { s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY)); x_ini_free(out_ini); return false; }
  out_ini->sections[0].name = gs;
  out_ini->sections[0].first_entry = -1;
  out_ini->sections_count = 1;
  out_ini->global_section = 0;

  /* copy data to a working buffer so we can slice lines easily */
  char *buf = (char *)X_INI_ALLOC(size + 1);
  if (!buf) { s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY)); x_ini_free(out_ini); return false; }
  memcpy(buf, data, size);
  buf[size] = '\0';

  int current_section = out_ini->global_section;

  /* parse line by line */
  char *cursor = buf;
  int line_no = 0;

  while (*cursor)
  {
    char *line = cursor;
    /* advance to next line */
    while (*cursor && *cursor != '\n' && *cursor != '\r') { ++cursor; }
    if (*cursor == '\r') { *cursor++ = '\0'; if (*cursor == '\n') { ++cursor; } }
    else if (*cursor == '\n') { *cursor++ = '\0'; }
    ++line_no;

    s_x_chomp_inline_comment_unquoted(line);
    char *raw = line;
    char *trim = s_x_trim_inplace(raw);
    if (*trim == '\0') continue; /* empty */
    if (*trim == ';' || *trim == '#') continue; /* full-line comment */

    if (*trim == '[')
    {
      /* section line: [name] */
      char *rbr = strchr(trim, ']');
      if (!rbr)
      {
        s_x_set_err(err, XINI_ERR_EXPECT_RBRACKET, line_no, (int)(strlen(trim) + 1), s_x_err_msg(XINI_ERR_EXPECT_RBRACKET));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
      *rbr = '\0';
      char *name = s_x_trim_inplace(trim + 1);
      name = s_x_trim_inplace(name);
      const char *sname = s_x_pool_add_cstr(out_ini, name);
      if (!sname)
      {
        s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }

      if (out_ini->sections_count == out_ini->sections_cap)
      {
        void *np = s_x_realloc_grow(out_ini->sections, sizeof(XIniSection),
            &out_ini->sections_cap);
        if (!np)
        {
          s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
          X_INI_FREE(buf);
          x_ini_free(out_ini);
          return false;
        }
        out_ini->sections = (XIniSection *)np;
      }
      current_section = out_ini->sections_count;
      out_ini->sections[current_section].name = sname;
      out_ini->sections[current_section].first_entry = -1;
      out_ini->sections_count += 1;
      continue;
    }

    /* key = value */
    char *eq = strchr(trim, '=');
    if (!eq)
    {
      /* report column at first non-space char */
      int col = (int)(trim - raw) + 1;
      s_x_set_err(err, XINI_ERR_EXPECT_EQUALS, line_no, col, s_x_err_msg(XINI_ERR_EXPECT_EQUALS));
      X_INI_FREE(buf);
      x_ini_free(out_ini);
      return false;
    }
    *eq = '\0';
    char *k = s_x_trim_inplace(trim);
    char *v = s_x_trim_inplace(eq + 1);

    /* value normalization:
       - if quoted: decode escapes and drop quotes
       - else: chomp inline comments, then trim again
    */
    const char *vv = NULL;
    if (*v == '\"')
    {
      int err_col = 1;
      bool ok = false;
      vv = s_x_decode_quoted_into_pool(out_ini, v, &err_col, &ok);
      if (!ok || !vv)
      {
        s_x_set_err(err, XINI_ERR_UNTERMINATED_STRING, line_no, (int)(v - raw) + err_col + 1, s_x_err_msg(XINI_ERR_UNTERMINATED_STRING));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
    }
    else
    {
      s_x_chomp_inline_comment_unquoted(v);
      v = s_x_trim_inplace(v);
      vv = s_x_pool_add_cstr(out_ini, v);
      if (!vv)
      {
        s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
    }

    const char *kk = s_x_pool_add_cstr(out_ini, k);
    if (!kk)
    {
      s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
      X_INI_FREE(buf);
      x_ini_free(out_ini);
      return false;
    }

    if (out_ini->entries_count == out_ini->entries_cap)
    {
      void *np = s_x_realloc_grow(out_ini->entries, sizeof(XIniEntry),
                                  &out_ini->entries_cap);
      if (!np)
      {
        s_x_set_err(err, XINI_ERR_MEMORY, line_no, 1, s_x_err_msg(XINI_ERR_MEMORY));
        X_INI_FREE(buf);
        x_ini_free(out_ini);
        return false;
      }
      out_ini->entries = (XIniEntry *)np;
    }

    int idx = out_ini->entries_count++;
    out_ini->entries[idx].section = current_section;
    out_ini->entries[idx].key = kk;
    out_ini->entries[idx].value = vv;

    if (out_ini->sections[current_section].first_entry < 0)
    {
      out_ini->sections[current_section].first_entry = idx;
    }
  }

  X_INI_FREE(buf);
  return true;
}

X_INI_API bool x_ini_load_file(const char *path, XIni *out_ini, XIniError *err)
{
  if (err) { err->code = XINI_OK; err->line = 0; err->column = 0; err->message = s_x_err_msg(XINI_OK); }
  if (!path || !out_ini) { s_x_set_err(err, XINI_ERR_SYNTAX, 0, 0, "null argument"); return false; }

  FILE *f = fopen(path, "rb");
  if (!f) { s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO)); return false; }

  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO)); return false; }
  long len = ftell(f);
  if (len < 0) { fclose(f); s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO)); return false; }
  rewind(f);

  char *buf = (char *)X_INI_ALLOC((size_t)len + 1);
  if (!buf) { fclose(f); s_x_set_err(err, XINI_ERR_MEMORY, 0, 0, s_x_err_msg(XINI_ERR_MEMORY)); return false; }

  size_t rd = fread(buf, 1, (size_t)len, f);
  fclose(f);
  if (rd != (size_t)len)
  {
    X_INI_FREE(buf);
    s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO));
    return false;
  }
  buf[len] = '\0';

  bool ok = x_ini_load_mem(buf, (size_t)len, out_ini, err);
  X_INI_FREE(buf);
  return ok;
}

X_INI_API void x_ini_free(XIni *ini)
{
  if (!ini) return;

  X_INI_FREE(ini->pool);
  X_INI_FREE(ini->sections);
  X_INI_FREE(ini->entries);

  memset(ini, 0, sizeof(*ini));
  ini->global_section = -1;
}

X_INI_API const char *x_ini_get(const XIni *ini, const char *section, const char *key, const char *def_value)
{
  if (!ini || !key) return def_value;

  const char *secname = section ? section : "";
  int sidx = s_x_find_section(ini, secname);
  if (sidx < 0) return def_value;

  /* last definition wins -> iterate from the end */
  for (int i = ini->entries_count - 1; i >= 0; --i)
  {
    const XIniEntry *e = &ini->entries[i];
    if (e->section == sidx && strcmp(e->key, key) == 0) return e->value;
  }
  return def_value;
}

X_INI_API int32_t x_ini_get_i32(const XIni *ini, const char *section, const char *key, int32_t def_value)
{
  const char *s = x_ini_get(ini, section, key, NULL);
  if (!s) return def_value;
  char *end = NULL;
  long v = strtol(s, &end, 0);
  if (end == s) return def_value;
  return (int32_t)v;
}

X_INI_API uint32_t x_ini_get_u32(const XIni *ini, const char *section, const char *key, uint32_t def_value)
{
  const char *s = x_ini_get(ini, section, key, NULL);
  if (!s) return def_value;

  const char *p = s;
  while (*p && isspace((unsigned char)*p)) { ++p; }
  if (*p == '-') return def_value;

  char *end = NULL;
  unsigned long long v = strtoull(s, &end, 0);
  if (end == s || v > UINT32_MAX) return def_value;
  return (uint32_t)v;
}

X_INI_API float x_ini_get_f32(const XIni *ini, const char *section, const char *key, float def_value)
{
  const char *s = x_ini_get(ini, section, key, NULL);
  if (!s) return def_value;
  char *end = NULL;
  float v = (float)strtod(s, &end);
  if (end == s) return def_value;
  return v;
}

X_INI_API bool x_ini_get_bool(const XIni *ini, const char *section, const char *key, bool def_value)
{
  const char *s = x_ini_get(ini, section, key, NULL);
  if (!s) return def_value;

  /* case-insensitive checks for common forms */
  char c0 = (char)tolower((unsigned char)s[0]);
  if ((c0 == 't' && s_x_stricmp(s, "true")  == 0) ||
      (c0 == 'y' && s_x_stricmp(s, "yes")   == 0) ||
      (c0 == 'o' && s_x_stricmp(s, "on")    == 0) ||
      (c0 == '1' && s[1] == '\0'))
  {
    return true;
  }
  if ((c0 == 'f' && s_x_stricmp(s, "false") == 0) ||
      (c0 == 'n' && s_x_stricmp(s, "no")    == 0) ||
      (c0 == 'o' && s_x_stricmp(s, "off")   == 0) ||
      (c0 == '0' && s[1] == '\0'))
  {
    return false;
  }
  return def_value;
}

X_INI_API bool x_ini_set(XIni *ini, const char *section, const char *key, const char *value)
{
  if (!ini || !ini->pool || !ini->sections || !ini->entries || !key || !value) return false;

  const char *secname = section ? section : "";
  int sidx = s_x_find_section(ini, secname);

  if (sidx >= 0)
  {
    for (int i = ini->entries_count - 1; i >= 0; --i)
    {
      XIniEntry *e = &ini->entries[i];
      if (e->section == sidx && strcmp(e->key, key) == 0)
      {
        const char *vv = s_x_pool_add_cstr(ini, value);
        if (!vv) return false;
        e->value = vv;
        return true;
      }
    }

    if (ini->entries_count == ini->entries_cap)
    {
      void *np = s_x_realloc_grow(ini->entries, sizeof(XIniEntry),
                                  &ini->entries_cap);
      if (!np) return false;
      ini->entries = (XIniEntry *)np;
    }

    size_t key_len = strlen(key);
    size_t value_len = strlen(value);
    size_t key_offset = 0;
    size_t value_offset = 0;
    uintptr_t pool_addr = (uintptr_t)ini->pool;
    uintptr_t key_addr = (uintptr_t)key;
    uintptr_t value_addr = (uintptr_t)value;
    bool key_in_pool = key_addr >= pool_addr && key_addr < pool_addr + ini->pool_size;
    bool value_in_pool = value_addr >= pool_addr && value_addr < pool_addr + ini->pool_size;
    if (key_in_pool) key_offset = (size_t)(key_addr - pool_addr);
    if (value_in_pool) value_offset = (size_t)(value_addr - pool_addr);

    if (!s_x_pool_reserve(ini, key_len + 1 + value_len + 1)) return false;
    if (key_in_pool) key = ini->pool + key_offset;
    if (value_in_pool) value = ini->pool + value_offset;

    const char *kk = s_x_pool_add(ini, key, key_len);
    if (!kk) return false;

    const char *vv = s_x_pool_add(ini, value, value_len);
    if (!vv) return false;

    int idx = ini->entries_count++;
    ini->entries[idx].section = sidx;
    ini->entries[idx].key = kk;
    ini->entries[idx].value = vv;

    if (ini->sections[sidx].first_entry < 0)
    {
      ini->sections[sidx].first_entry = idx;
    }

    return true;
  }

  if (ini->sections_count == ini->sections_cap)
  {
    void *np = s_x_realloc_grow(ini->sections, sizeof(XIniSection),
                                &ini->sections_cap);
    if (!np) return false;
    ini->sections = (XIniSection *)np;
  }

  if (ini->entries_count == ini->entries_cap)
  {
    void *np = s_x_realloc_grow(ini->entries, sizeof(XIniEntry),
                                &ini->entries_cap);
    if (!np) return false;
    ini->entries = (XIniEntry *)np;
  }

  size_t section_len = strlen(secname);
  size_t key_len = strlen(key);
  size_t value_len = strlen(value);
  size_t section_offset = 0;
  size_t key_offset = 0;
  size_t value_offset = 0;
  uintptr_t pool_addr = (uintptr_t)ini->pool;
  uintptr_t section_addr = (uintptr_t)secname;
  uintptr_t key_addr = (uintptr_t)key;
  uintptr_t value_addr = (uintptr_t)value;
  bool section_in_pool = section_addr >= pool_addr && section_addr < pool_addr + ini->pool_size;
  bool key_in_pool = key_addr >= pool_addr && key_addr < pool_addr + ini->pool_size;
  bool value_in_pool = value_addr >= pool_addr && value_addr < pool_addr + ini->pool_size;
  if (section_in_pool) section_offset = (size_t)(section_addr - pool_addr);
  if (key_in_pool) key_offset = (size_t)(key_addr - pool_addr);
  if (value_in_pool) value_offset = (size_t)(value_addr - pool_addr);

  if (!s_x_pool_reserve(ini, section_len + 1 + key_len + 1 + value_len + 1)) return false;
  if (section_in_pool) secname = ini->pool + section_offset;
  if (key_in_pool) key = ini->pool + key_offset;
  if (value_in_pool) value = ini->pool + value_offset;

  const char *sname = s_x_pool_add(ini, secname, section_len);
  if (!sname) return false;

  const char *kk = s_x_pool_add(ini, key, key_len);
  if (!kk) return false;

  const char *vv = s_x_pool_add(ini, value, value_len);
  if (!vv) return false;

  sidx = ini->sections_count++;
  ini->sections[sidx].name = sname;
  ini->sections[sidx].first_entry = ini->entries_count;

  int idx = ini->entries_count++;
  ini->entries[idx].section = sidx;
  ini->entries[idx].key = kk;
  ini->entries[idx].value = vv;

  return true;
}

X_INI_API bool x_ini_set_i32(XIni *ini, const char *section, const char *key, int32_t value)
{
  char buf[32];
  int n = snprintf(buf, sizeof(buf), "%ld", (long)value);
  if (n < 0 || (size_t)n >= sizeof(buf)) return false;
  return x_ini_set(ini, section, key, buf);
}

X_INI_API bool x_ini_set_u32(XIni *ini, const char *section, const char *key, uint32_t value)
{
  char buf[32];
  int n = snprintf(buf, sizeof(buf), "%lu", (unsigned long)value);
  if (n < 0 || (size_t)n >= sizeof(buf)) return false;
  return x_ini_set(ini, section, key, buf);
}

X_INI_API bool x_ini_set_u32_hex(XIni *ini, const char *section, const char *key, uint32_t value)
{
  char buf[16];
  int n = snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)value);
  if (n < 0 || (size_t)n >= sizeof(buf)) return false;
  return x_ini_set(ini, section, key, buf);
}

X_INI_API bool x_ini_set_f32(XIni *ini, const char *section, const char *key, float value)
{
  char buf[32];
  int n = snprintf(buf, sizeof(buf), "%.9g", (double)value);
  if (n < 0 || (size_t)n >= sizeof(buf)) return false;
  return x_ini_set(ini, section, key, buf);
}

X_INI_API bool x_ini_set_bool(XIni *ini, const char *section, const char *key, bool value)
{
  return x_ini_set(ini, section, key, value ? "true" : "false");
}

static bool s_x_write_value(FILE *f, const char *value)
{
  bool quote = false;
  size_t len = strlen(value);

  if (len > 0 && (isspace((unsigned char)value[0]) ||
                  isspace((unsigned char)value[len - 1])))
  {
    quote = true;
  }

  for (const char *p = value; *p; ++p)
  {
    if (*p == ';' || *p == '#' || *p == '\"' || *p == '\n' || *p == '\t')
    {
      quote = true;
      break;
    }
  }

  if (!quote)
  {
    return fputs(value, f) >= 0;
  }

  if (fputc('\"', f) == EOF) return false;

  for (const char *p = value; *p; ++p)
  {
    switch (*p)
    {
      case '\n':
        if (fputs("\\n", f) < 0) return false;
        break;
      case '\t':
        if (fputs("\\t", f) < 0) return false;
        break;
      case '\\':
        if (fputs("\\\\", f) < 0) return false;
        break;
      case '\"':
        if (fputs("\\\"", f) < 0) return false;
        break;
      default:
        if (*p == '\r') return false;
        if (fputc((unsigned char)*p, f) == EOF) return false;
        break;
    }
  }

  return fputc('\"', f) != EOF;
}

X_INI_API bool x_ini_write_file(const char *path, const XIni *ini, XIniError *err)
{
  if (err) { err->code = XINI_OK; err->line = 0; err->column = 0; err->message = s_x_err_msg(XINI_OK); }
  if (!path || !ini) { s_x_set_err(err, XINI_ERR_SYNTAX, 0, 0, "null argument"); return false; }

  FILE *f = fopen(path, "wb");
  if (!f) { s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO)); return false; }

  bool wrote_any_section = false;

  for (int s = 0; s < ini->sections_count; ++s)
  {
    const XIniSection *section = &ini->sections[s];
    bool is_global = (s == ini->global_section);

    if (!is_global)
    {
      if (wrote_any_section)
      {
        if (fputc('\n', f) == EOF) goto write_error;
      }

      if (fprintf(f, "[%s]\n", section->name) < 0) goto write_error;
      wrote_any_section = true;
    }

    for (int i = 0; i < ini->entries_count; ++i)
    {
      const XIniEntry *e = &ini->entries[i];
      if (e->section != s) continue;

      if (fprintf(f, "%s=", e->key) < 0) goto write_error;
      if (!s_x_write_value(f, e->value)) goto write_error;
      if (fputc('\n', f) == EOF) goto write_error;
    }
  }

  if (fclose(f) != 0)
  {
    s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO));
    return false;
  }

  return true;

write_error:
  fclose(f);
  s_x_set_err(err, XINI_ERR_IO, 0, 0, s_x_err_msg(XINI_ERR_IO));
  return false;
}

X_INI_API int x_ini_section_count(const XIni *ini)
{
  if (!ini) return 0;
  return ini->sections_count;
}

X_INI_API const char *x_ini_section_name(const XIni *ini, int section_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return NULL;
  return ini->sections[section_index].name;
}

X_INI_API int x_ini_key_count(const XIni *ini, int section_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return 0;
  return s_x_count_keys_in_section(ini, section_index);
}

X_INI_API const char *x_ini_key_name(const XIni *ini, int section_index, int key_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return NULL;

  /* linear scan; stable order of appearance */
  int seen = 0;
  for (int i = 0; i < ini->entries_count; ++i)
  {
    const XIniEntry *e = &ini->entries[i];
    if (e->section == section_index)
    {
      if (seen == key_index) return e->key;
      ++seen;
    }
  }
  return NULL;
}

X_INI_API const char *x_ini_value_at(const XIni *ini, int section_index, int key_index)
{
  if (!ini || section_index < 0 || section_index >= ini->sections_count) return NULL;

  int seen = 0;
  for (int i = 0; i < ini->entries_count; ++i)
  {
    const XIniEntry *e = &ini->entries[i];
    if (e->section == section_index)
    {
      if (seen == key_index) return e->value;
      ++seen;
    }
  }
  return NULL;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // X_IMPL_INI
#endif // X_INI_H
