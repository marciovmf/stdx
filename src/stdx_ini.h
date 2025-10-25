/**
 * STDX - INI (nested paths)
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Usage:
 *   #define X_IMPL_INI
 *   #include "stdx_ini.h"
 *
 * Author:  marciovmf
 * License: MIT
 *
 * ------------------------------------------------------------------------
 * FILE FORMAT OVERVIEW
 * ------------------------------------------------------------------------
 *
 * This format is a simplified, hierarchical INI. It supports nested sections,
 * typed values, and scalar arrays with minimal syntax.
 *
 *   # Comments start with '#'
 *   # Sections end with ':' and can be nested using '/'
 *
 *   level/tutorial:
 *     enabled = true
 *     seed    = 934784
 *     description = """
 *       Multi line strings are
 *       also supported!
 *       """
 *
 *   level/tutorial/objects/sphere:
 *     position = 1.0, 2.0, 0.0
 *     scale    = 1.0, 1.0, 1.0
 *     weights  = 10, 20, 40, 103,
 *                99, 71, 44, -1
 *
 *   level/tutorial/objects/tree:
 *     position = 7.0, 0.0, 0.0
 *     scale    = 1.0, 1.0, 1.0
 *
 * FEATURES:
 * - Section paths use '/' to define hierarchy (like directories).
 * - Keys use standard `key = value` syntax.
 * - Values can be:
 *      • bool:   true / false
 *      • int:    42, -10
 *      • float:  3.14, 1e-3
 *      • string: "quoted" or """multi-line"""
 *      • arrays: comma-separated values, may span lines if ending with ','
 * - Whitespace and indentation are ignored.
 * - The format avoids {}, [], and indentation-based scoping.
 *
 * Example API usage:
 *     XIni *ini = NULL;
 *     x_ini_open_from_memory(buf, len, 0, &ini);
 *     XIniCursor lvl;
 *     x_ini_get_section(ini, x_ini_root(ini), "level/tutorial", &lvl);
 *     int enabled = 0;
 *     x_ini_get_bool(ini, lvl, "enabled", &enabled);
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

/* Version */
#define X_INI_VERSION_MAJOR 1
#define X_INI_VERSION_MINOR 0
#define X_INI_VERSION_PATCH 0
#define X_INI_VERSION (X_INI_VERSION_MAJOR*10000 + X_INI_VERSION_MINOR*100 + X_INI_VERSION_PATCH)

#ifdef __cplusplus
extern "C" {
#endif

  /* ===== Public Types ===== */

  typedef struct XIni XIni;

  typedef struct XIniCursor
  {
    int node;
  } XIniCursor;

  typedef enum XIniValKind
  {
    XINI_V_NONE = 0,
    XINI_V_STR  = 1,
    XINI_V_I64  = 2,
    XINI_V_F64  = 3,
    XINI_V_BOOL = 4,
    XINI_V_ARR  = 5
  } XIniValKind;

  typedef struct XIniStrSlice
  {
    const char *ptr;
    uint32_t    len;
  } XIniStrSlice;

  /* ===== Open / Close ===== */

  enum
  {
    XINI_OPEN_DEFAULT = 0u
  };

 /**
 * Parses an INI-like text buffer directly from memory using **one allocation**.
 *
 * - Performs a fast prescan to estimate required arena size.
 * - Allocates a single memory block that holds all internal arrays.
 * - Builds a tree of sections and key/value entries that reference the
 *   original text buffer (no string copies).
 * - Detects value types automatically: bool, int, float, string, or array.
 * - Parses arrays once at load time and stores typed slices (int64, double,
 *   or string) for O(1) access later.
 *
 * The resulting `XIni*` can be queried directly or explored progressively:
 *
 *     XIni *ini = NULL;
 *     x_ini_open_from_memory(buf, size, 0, &ini);
 *     XIniCursor cur;
 *     x_ini_get_section(ini, x_ini_root(ini), "level/tutorial", &cur);
 *
 * Lifetime:
 *     The structure references the caller's text buffer and one internal arena.
 *     Free all resources with `x_ini_close(ini)`.
 */
  X_INI_API int  x_ini_open_from_memory(const char *buf, uint32_t size, uint32_t flags, XIni **out_ini);
  X_INI_API void x_ini_close(XIni *ini);

  /* ===== Section navigation ===== */

  static inline XIniCursor x_ini_root(XIni *ini)
  {
    XIniCursor c;
    (void)ini;
    c.node = -1;
    return c;
  }

  X_INI_API int x_ini_get_section(XIni *ini, XIniCursor parent, const char *path, XIniCursor *out);
  X_INI_API int x_ini_child_count(XIni *ini, XIniCursor cur, uint32_t *out_count);
  X_INI_API int x_ini_child_at(XIni *ini, XIniCursor cur, uint32_t index, XIniCursor *out_child);
  X_INI_API int x_ini_find_child(XIni *ini, XIniCursor cur, const char *name, uint32_t name_len, XIniCursor *out_child);
  X_INI_API int x_ini_section_name(XIni *ini, XIniCursor cur, const char **out_name, uint32_t *out_len);

  /* ===== Section entries ===== */

  X_INI_API int x_ini_entry_count(XIni *ini, XIniCursor cur, uint32_t *out_count);
  X_INI_API int x_ini_entry_key_at(XIni *ini, XIniCursor cur, uint32_t index, const char **out_key, uint32_t *out_key_len);

  X_INI_API int x_ini_get_bool(XIni *ini, XIniCursor cur, const char *key, int *out_bool);
  X_INI_API int x_ini_get_i64 (XIni *ini, XIniCursor cur, const char *key, int64_t *out_i64);
  X_INI_API int x_ini_get_f64 (XIni *ini, XIniCursor cur, const char *key, double *out_f64);
  X_INI_API int x_ini_get_str (XIni *ini, XIniCursor cur, const char *key, const char **out_str, uint32_t *out_len);

  /* Scalar arrays: numbers and strings */
  X_INI_API int x_ini_get_array_len (XIni *ini, XIniCursor cur, const char *key, uint32_t *out_len);
  X_INI_API int x_ini_get_array_f64(XIni *ini, XIniCursor cur, const char *key, const double **out_vals, uint32_t *out_len);
  X_INI_API int x_ini_get_array_i64(XIni *ini, XIniCursor cur, const char *key, const int64_t **out_vals, uint32_t *out_len);
  X_INI_API int x_ini_get_array_str(XIni *ini, XIniCursor cur, const char *key, const XIniStrSlice **out_vals, uint32_t *out_len);


  /* ===== Inline helpers ===== */

  static inline int x_ini_has_key(XIni *ini, XIniCursor cur, const char *key)
  {
    const char *s = NULL;
    uint32_t slen = 0u;
    if (x_ini_get_str(ini, cur, key, &s, &slen) != 0)
    {
      return 1;
    }
    double f = 0.0;
    if (x_ini_get_f64(ini, cur, key, &f) != 0)
    {
      return 1;
    }
    int64_t i = 0;
    if (x_ini_get_i64(ini, cur, key, &i) != 0)
    {
      return 1;
    }
    int b = 0;
    if (x_ini_get_bool(ini, cur, key, &b) != 0)
    {
      return 1;
    }
    uint32_t n = 0u;
    if (x_ini_get_array_len(ini, cur, key, &n) != 0)
    {
      return 1;
    }
    return 0;
  }

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Implementation */
#ifdef X_IMPL_INI

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Internals */

typedef struct XIniValue
{
  XIniValKind kind;
  uint32_t    off;
  uint32_t    len;
  int64_t     i64;
  double      f64;
  int         boolean;
  uint32_t    arr_start;
  uint32_t    arr_count;
  uint32_t    arr_is_f64;
  uint32_t    arr_is_i64;
  uint32_t    arr_is_str;
} XIniValue;

typedef struct XIniKV
{
  uint32_t key_off;
  uint32_t key_len;
  XIniValue val;
} XIniKV;

typedef struct XIniNode
{
  uint32_t name_off;
  uint32_t name_len;
  uint32_t name_hash;     /* 32-bit hash of the section name */
  int32_t  parent;
  int32_t  first_child;
  int32_t  next_sibling;
  uint32_t kv_start;
  uint32_t kv_count;
} XIniNode;

struct XIni
{
  const char *text;
  uint32_t    text_len;

  /* single arena */
  void      *arena;
  uint32_t   arena_size;

  XIniNode  *nodes;
  uint32_t   node_count;

  XIniKV    *kvs;
  uint32_t   kv_count;

  double    *nums_f64;
  uint32_t   nums_f64_count;

  int64_t   *nums_i64;
  uint32_t   nums_i64_count;

  XIniStrSlice *str_slices;
  uint32_t      str_slice_count;
};

/* Utilities */

static void *s_ini_arena_alloc(void *base, uint32_t *offset, uint32_t need, uint32_t total)
{
  if (*offset + need > total)
  {
    return NULL;
  }
  char *b = (char *)base;
  void *ptr = b + *offset;
  *offset = *offset + need;
  return ptr;
}

/* Simple 32-bit hash (FNV-1a) for section names */
static uint32_t s_ini_hash32(const char *s, uint32_t len)
{
  uint32_t h = 2166136261u;
  for (uint32_t i = 0; i < len; ++i)
  {
    h ^= (uint8_t)s[i];
    h *= 16777619u;
  }
  return h ? h : 1u; /* avoid zero to keep "unset" distinguishable if needed */
}

static int s_ini_is_header_line(const char *line, uint32_t len)
{
  /* valid header: ^[A-Za-z0-9_./-]+: at column 0 */
  if (len == 0)
  {
    return 0;
  }
  uint32_t i = 0u;
  while (i < len)
  {
    char c = line[i];
    if (c == ':')
    {
      return 1;
    }
    if (c == '#')
    {
      return 0;
    }
    if (c == ' ' || c == '\t' || c == '\r')
    {
      return 0;
    }
    if (isalnum((unsigned char)c) == 0 && c != '_' && c != '.' && c != '/' && c != '-')
    {
      return 0;
    }
    i = i + 1u;
  }
  return 0;
}

static uint32_t s_ini_trim_right_len(const char *s, uint32_t len)
{
  while (len > 0u)
  {
    char c = s[len - 1u];
    if (c == ' ' || c == '\t' || c == '\r')
    {
      len = len - 1u;
      continue;
    }
    break;
  }
  return len;
}

static void s_ini_split_key_value(const char *line, uint32_t len, uint32_t *eq_pos)
{
  uint32_t i = 0u;
  *eq_pos = UINT32_MAX;
  int in_str = 0;
  int triple = 0;
  while (i < len)
  {
    char c = line[i];
    if (c == '"' && triple == 0)
    {
      in_str = !in_str;
    }
    if (i + 2u < len && line[i] == '"' && line[i + 1u] == '"' && line[i + 2u] == '"')
    {
      triple = !triple;
      i = i + 2u;
    }
    else if (c == '=' && in_str == 0 && triple == 0)
    {
      *eq_pos = i;
      return;
    }
    i = i + 1u;
  }
}

static int s_ini_str_equal(const char *a, uint32_t alen, const char *b, uint32_t blen)
{
  if (alen != blen)
  {
    return 0;
  }
  if (alen == 0u)
  {
    return 1;
  }
  return memcmp(a, b, alen) == 0 ? 1 : 0;
}

static uint32_t s_ini_split_path(const char *path, uint32_t len, uint32_t *seg_off, uint32_t *seg_len, uint32_t max_seg)
{
  uint32_t n = 0u;
  uint32_t i = 0u;
  uint32_t start = 0u;
  while (i <= len)
  {
    char c = (i < len) ? path[i] : '/';
    if (c == '/')
    {
      if (i > start && n < max_seg)
      {
        seg_off[n] = start;
        seg_len[n] = i - start;
        n = n + 1u;
      }
      start = i + 1u;
    }
    i = i + 1u;
  }
  return n;
}

static int s_ini_find_child_linear(XIni *ini, int parent_idx, const char *name, uint32_t len)
{
  uint32_t target_hash = s_ini_hash32(name, len);

  int idx = -1;
  int32_t it = -1;
  if (parent_idx < 0)
  {
    it = -2;
  }
  if (parent_idx >= 0)
  {
    it = ini->nodes[parent_idx].first_child;
  }

  while (1)
  {
    if (parent_idx < 0)
    {
      for (uint32_t i = 0u; i < ini->node_count; i = i + 1u)
      {
        if (ini->nodes[i].parent == -1)
        {
          /* Hash pre-check before string compare */
          if (ini->nodes[i].name_hash != target_hash)
            continue;

          const char *nm = ini->text + ini->nodes[i].name_off;
          uint32_t nml = ini->nodes[i].name_len;
          if (s_ini_str_equal(nm, nml, name, len) != 0)
          {
            return (int)i;
          }
        }
      }
      return -1;
    }

    if (it < 0)
    {
      break;
    }

    /* Hash pre-check before string compare */
    if (ini->nodes[it].name_hash == target_hash)
    {
      const char *nm = ini->text + ini->nodes[it].name_off;
      uint32_t nml = ini->nodes[it].name_len;
      if (s_ini_str_equal(nm, nml, name, len) != 0)
      {
        idx = it;
        break;
      }
    }

    it = ini->nodes[it].next_sibling;
  }

  return idx;
}

/* Create a child node and link as last sibling */
static int s_ini_add_child(XIni *ini, int parent_idx, uint32_t off, uint32_t len, uint32_t *next_free_node)
{
  int idx = (int)(*next_free_node);
  *next_free_node = *next_free_node + 1u;

  XIniNode *n = &ini->nodes[idx];
  n->name_off = off;
  n->name_len = len;
  n->name_hash = s_ini_hash32(ini->text + off, len); /* compute hash once */
  n->parent = parent_idx;
  n->first_child = -1;
  n->next_sibling = -1;
  n->kv_start = 0u;
  n->kv_count = 0u;

  if (parent_idx >= 0)
  {
    if (ini->nodes[parent_idx].first_child < 0)
    {
      ini->nodes[parent_idx].first_child = idx;
    }
    else
    {
      int32_t it = ini->nodes[parent_idx].first_child;
      while (ini->nodes[it].next_sibling >= 0)
      {
        it = ini->nodes[it].next_sibling;
      }
      ini->nodes[it].next_sibling = idx;
    }
  }

  return idx;
}

/* Triple-quoted helpers */

/* Find the closing delimiter of a triple-quoted string (""" ... """). */
static int s_ini_find_triple_end(const char *text, uint32_t size, uint32_t start_off, uint32_t *out_end_off)
{
  if (text == NULL || start_off + 2u >= size)
  {
    return 0;
  }
  uint32_t i = start_off;
  while (i + 2u < size)
  {
    if (text[i] == '"' && text[i + 1u] == '"' && text[i + 2u] == '"')
    {
      *out_end_off = i;
      return 1;
    }
    i = i + 1u;
  }
  return 0;
}

/* Advance to the beginning of the next line after 'off' (or EOF). */
static uint32_t s_ini_advance_to_next_line_after(uint32_t off, const char *buf, uint32_t size)
{
  uint32_t i = off;
  while (i < size && buf[i] != '\n')
  {
    i = i + 1u;
  }
  if (i < size && buf[i] == '\n')
  {
    i = i + 1u;
  }
  return i;
}

/* Prescan to size arena */

typedef struct s_ini_counts
{
  uint32_t headers;
  uint32_t kvs;
  uint32_t comma_tokens;
  uint32_t triple_blocks;
} s_ini_counts;

static void s_ini_prescan(const char *buf, uint32_t len, s_ini_counts *out)
{
  out->headers = 0u;
  out->kvs = 0u;
  out->comma_tokens = 0u;
  out->triple_blocks = 0u;

  uint32_t i = 0u;
  int in_triple = 0;

  while (i < len)
  {
    const char *line = buf + i;
    uint32_t l = 0u;
    while (i + l < len && buf[i + l] != '\n')
    {
      l = l + 1u;
    }

    uint32_t tl = l;
    tl = s_ini_trim_right_len(line, tl);

    if (in_triple == 0)
    {
      if (tl > 0u && s_ini_is_header_line(line, tl) != 0)
      {
        out->headers = out->headers + 1u;
      }
      else
      {
        uint32_t eq = UINT32_MAX;
        s_ini_split_key_value(line, tl, &eq);
        if (eq != UINT32_MAX)
        {
          out->kvs = out->kvs + 1u;
          for (uint32_t k = eq + 1u; k < tl; k = k + 1u)
          {
            char c = line[k];
            if (c == ',')
            {
              out->comma_tokens = out->comma_tokens + 1u;
            }
          }
          if (tl > 0u && line[tl - 1u] == ',')
          {
            uint32_t j = i + l + ((i + l) < len ? 1u : 0u);
            while (j < len)
            {
              const char *ln = buf + j;
              uint32_t ll = 0u;
              while (j + ll < len && buf[j + ll] != '\n')
              {
                ll = ll + 1u;
              }
              uint32_t tll = s_ini_trim_right_len(ln, ll);
              for (uint32_t k = 0u; k < tll; k = k + 1u)
              {
                if (ln[k] == ',')
                {
                  out->comma_tokens = out->comma_tokens + 1u;
                }
              }
              if (tll == 0u)
              {
                break;
              }
              if (ln[tll - 1u] != ',')
              {
                break;
              }
              j = j + ll + ((j + ll) < len ? 1u : 0u);
            }
          }
        }
      }

      for (uint32_t k = 0u; k + 2u < tl; k = k + 1u)
      {
        if (line[k] == '"' && line[k + 1u] == '"' && line[k + 2u] == '"')
        {
          in_triple = 1;
          out->triple_blocks = out->triple_blocks + 1u;
          break;
        }
      }
    }
    else
    {
      for (uint32_t k = 0u; k + 2u < tl; k = k + 1u)
      {
        if (line[k] == '"' && line[k + 1u] == '"' && line[k + 2u] == '"')
        {
          in_triple = 0;
          break;
        }
      }
    }

    i = i + l + ((i + l) < len ? 1u : 0u);
  }
}

/* Parser */

X_INI_API int x_ini_open_from_memory(const char *buf, uint32_t size, uint32_t flags, XIni **out_ini)
{
  (void)flags;
  if (buf == NULL || out_ini == NULL || size == 0u)
  {
    return 0;
  }

  s_ini_counts cnt;
  s_ini_prescan(buf, size, &cnt);

  uint32_t max_segments = 8u;
  uint32_t max_nodes = cnt.headers * max_segments + 8u;
  uint32_t max_kvs = cnt.kvs + 8u;
  uint32_t max_nums = cnt.comma_tokens + cnt.kvs + 8u;
  uint32_t max_str_slices = cnt.comma_tokens + 8u;

  uint32_t arena_bytes = 0u;
  arena_bytes = arena_bytes + (uint32_t)sizeof(XIni);
  arena_bytes = arena_bytes + max_nodes * (uint32_t)sizeof(XIniNode);
  arena_bytes = arena_bytes + max_kvs   * (uint32_t)sizeof(XIniKV);
  arena_bytes = arena_bytes + max_nums  * (uint32_t)sizeof(double);
  arena_bytes = arena_bytes + max_nums  * (uint32_t)sizeof(int64_t);
  arena_bytes = arena_bytes + max_str_slices * (uint32_t)sizeof(XIniStrSlice);

  void *arena = (void *)malloc(arena_bytes);
  if (arena == NULL)
  {
    return 0;
  }
  uint32_t off = 0u;

  XIni *ini = (XIni *)s_ini_arena_alloc(arena, &off, (uint32_t)sizeof(XIni), arena_bytes);
  ini->arena = arena;
  ini->arena_size = arena_bytes;

  ini->text = buf;
  ini->text_len = size;

  ini->nodes = (XIniNode *)s_ini_arena_alloc(arena, &off, max_nodes * (uint32_t)sizeof(XIniNode), arena_bytes);
  ini->node_count = 0u;

  ini->kvs = (XIniKV *)s_ini_arena_alloc(arena, &off, max_kvs * (uint32_t)sizeof(XIniKV), arena_bytes);
  ini->kv_count = 0u;

  ini->nums_f64 = (double *)s_ini_arena_alloc(arena, &off, max_nums * (uint32_t)sizeof(double), arena_bytes);
  ini->nums_f64_count = 0u;

  ini->nums_i64 = (int64_t *)s_ini_arena_alloc(arena, &off, max_nums * (uint32_t)sizeof(int64_t), arena_bytes);
  ini->nums_i64_count = 0u;

  ini->str_slices = (XIniStrSlice *)s_ini_arena_alloc(arena, &off, max_str_slices * (uint32_t)sizeof(XIniStrSlice), arena_bytes);
  ini->str_slice_count = 0u;

  int cur_node = -1;
  uint32_t next_free_node = 0u;

  uint32_t i = 0u;

  while (i < size)
  {
    const char *line = buf + i;
    uint32_t l = 0u;
    while (i + l < size && buf[i + l] != '\n')
    {
      l = l + 1u;
    }

    uint32_t tl = s_ini_trim_right_len(line, l);
    const char *trim = line;
    uint32_t pos = 0u;
    while (pos < tl && (trim[pos] == ' ' || trim[pos] == '\t'))
    {
      pos = pos + 1u;
    }

    if (tl == 0u)
    {
      i = i + l + ((i + l) < size ? 1u : 0u);
      continue;
    }

    if (line[0] == '#')
    {
      i = i + l + ((i + l) < size ? 1u : 0u);
      continue;
    }

    if (s_ini_is_header_line(line, tl) != 0 && pos == 0u)
    {
      uint32_t seg_off[16];
      uint32_t seg_len[16];
      uint32_t nseg = s_ini_split_path(line, tl - 1u, seg_off, seg_len, 16u);
      int parent = -1;

      for (uint32_t s = 0u; s < nseg; s = s + 1u)
      {
        int child = s_ini_find_child_linear(ini, parent, line + seg_off[s], seg_len[s]);
        if (child < 0)
        {
          child = s_ini_add_child(ini, parent, (uint32_t)((line + seg_off[s]) - ini->text), seg_len[s], &next_free_node);
          ini->node_count = next_free_node;
        }
        parent = child;
      }
      cur_node = parent;

      i = i + l + ((i + l) < size ? 1u : 0u);
      continue;
    }

    {
      uint32_t eq = UINT32_MAX;
      s_ini_split_key_value(line, tl, &eq);
      if (eq != UINT32_MAX && cur_node >= 0)
      {
        uint32_t k_off = 0u;
        uint32_t k_len = s_ini_trim_right_len(line, eq);
        while (k_off < k_len && (line[k_off] == ' ' || line[k_off] == '\t'))
        {
          k_off = k_off + 1u;
        }

        const char *valp = line + eq + 1u;
        uint32_t v_len = tl - (eq + 1u);
        while (v_len > 0u && (valp[0] == ' ' || valp[0] == '\t'))
        {
          valp = valp + 1u;
          v_len = v_len - 1u;
        }

        uint32_t raw_off = (uint32_t)(valp - ini->text);
        uint32_t raw_len = v_len;

        /* Handle triple-quoted immediately */
        if (raw_len >= 3u &&
            ini->text[raw_off + 0u] == '"' &&
            ini->text[raw_off + 1u] == '"' &&
            ini->text[raw_off + 2u] == '"')
        {
          uint32_t end_off = 0u;
          int closed = s_ini_find_triple_end(ini->text, ini->text_len, raw_off + 3u, &end_off);

          XIniKV *kv = &ini->kvs[ini->kv_count];
          kv->key_off = (uint32_t)((line + k_off) - ini->text);
          kv->key_len = k_len - k_off;
          kv->val.kind = XINI_V_STR;
          kv->val.off  = raw_off + 3u;

          if (closed != 0 && end_off >= kv->val.off)
          {
            kv->val.len = end_off - kv->val.off;
            i = s_ini_advance_to_next_line_after(end_off, buf, size);
          }
          else
          {
            kv->val.len = ini->text_len - kv->val.off; /* tolerant: until EOF */
            i = size;
          }

          if (ini->nodes[cur_node].kv_count == 0u)
          {
            ini->nodes[cur_node].kv_start = ini->kv_count;
          }
          ini->kv_count = ini->kv_count + 1u;
          ini->nodes[cur_node].kv_count = ini->nodes[cur_node].kv_count + 1u;

          continue;
        }

        /* Multi-line scalar arrays by hanging comma */
        if (v_len > 0u && valp[v_len - 1u] == ',')
        {
          uint32_t j = i + l + ((i + l) < size ? 1u : 0u);
          while (j < size)
          {
            const char *ln = buf + j;
            uint32_t ll = 0u;
            while (j + ll < size && buf[j + ll] != '\n')
            {
              ll = ll + 1u;
            }
            uint32_t tll = s_ini_trim_right_len(ln, ll);
            raw_len = (uint32_t)((ln + tll) - (ini->text + raw_off));
            if (tll == 0u)
            {
              j = j + ll + ((j + ll) < size ? 1u : 0u);
              break;
            }
            if (ln[tll - 1u] != ',')
            {
              j = j + ll + ((j + ll) < size ? 1u : 0u);
              break;
            }
            j = j + ll + ((j + ll) < size ? 1u : 0u);
          }
        }

        XIniKV *kv = &ini->kvs[ini->kv_count];
        kv->key_off = (uint32_t)((line + k_off) - ini->text);
        kv->key_len = k_len - k_off;
        kv->val.kind = XINI_V_STR;
        kv->val.off = raw_off;
        kv->val.len = raw_len;
        kv->val.i64 = 0;
        kv->val.f64 = 0.0;
        kv->val.boolean = 0;
        kv->val.arr_start = 0u;
        kv->val.arr_count = 0u;
        kv->val.arr_is_f64 = 0u;
        kv->val.arr_is_i64 = 0u;
        kv->val.arr_is_str = 0u;

        const char *vp = ini->text + raw_off;
        uint32_t vl = raw_len;

        int has_comma = 0;
        int in_str_q = 0;
        for (uint32_t t = 0u; t < vl; t = t + 1u)
        {
          char c = vp[t];
          if (c == '"') { in_str_q = !in_str_q; }
          if (c == ',' && in_str_q == 0) { has_comma = 1; break; }
        }

        uint32_t vl2 = vl;
        const char *vp2 = vp;
        while (vl2 > 0u && (vp2[0] == ' ' || vp2[0] == '\t'))
        {
          vp2 = vp2 + 1u;
          vl2 = vl2 - 1u;
        }
        while (vl2 > 0u &&
               (vp2[vl2 - 1u] == ' ' || vp2[vl2 - 1u] == '\t' || vp2[vl2 - 1u] == '\r'))
        {
          vl2 = vl2 - 1u;
        }

        if (has_comma != 0)
        {
          uint32_t start = 0u;
          int mode_f64 = 1;
          int mode_i64 = 1;
          int mode_str = 0;

          /* NEW: capture pool indices before tokenizing */
          uint32_t f64_before = ini->nums_f64_count;
          uint32_t i64_before = ini->nums_i64_count;
          uint32_t str_before = ini->str_slice_count;

          while (start <= vl2)
          {
            uint32_t end = start;
            int inq = 0;
            while (end < vl2)
            {
              char c = vp2[end];
              if (c == '"') { inq = !inq; }
              if (c == ',' && inq == 0) { break; }
              end = end + 1u;
            }

            const char *ep = vp2 + start;
            uint32_t el = end - start;
            while (el > 0u && (ep[0] == ' ' || ep[0] == '\t')) { ep = ep + 1u; el = el - 1u; }
            while (el > 0u && (ep[el - 1u] == ' ' || ep[el - 1u] == '\t')) { el = el - 1u; }

            if (el > 0u)
            {
              if (ep[0] == '"' && el >= 2u && ep[el - 1u] == '"')
              {
                mode_i64 = 0; mode_f64 = 0; mode_str = 1;
                ini->str_slices[ini->str_slice_count].ptr = ep + 1u;
                ini->str_slices[ini->str_slice_count].len = el - 2u;
                ini->str_slice_count = ini->str_slice_count + 1u;
              }
              else
              {
                /* Integer fast-path before strtod */
                int is_int = 1;
                for (uint32_t q = 0u; q < el; q = q + 1u)
                {
                  char c2 = ep[q];
                  if (c2 == '.' || c2 == 'e' || c2 == 'E') { is_int = 0; break; }
                }
                if (is_int)
                {
                  long long iv = strtoll(ep, NULL, 10);
                  ini->nums_i64[ini->nums_i64_count] = (int64_t)iv;
                  ini->nums_i64_count = ini->nums_i64_count + 1u;
                  mode_f64 = 0;
                }
                else
                {
                  char *endp = NULL;
                  double dv = strtod(ep, &endp);
                  if (endp == ep + (ptrdiff_t)el)
                  {
                    ini->nums_f64[ini->nums_f64_count] = dv;
                    ini->nums_f64_count = ini->nums_f64_count + 1u;
                    mode_i64 = 0;
                  }
                  else
                  {
                    mode_i64 = 0; mode_f64 = 0; mode_str = 1;
                    ini->str_slices[ini->str_slice_count].ptr = ep;
                    ini->str_slices[ini->str_slice_count].len = el;
                    ini->str_slice_count = ini->str_slice_count + 1u;
                  }
                }
              }
            }

            if (end >= vl2) { start = vl2 + 1u; }
            else { start = end + 1u; }
          }

          kv->val.kind = XINI_V_ARR;

          /* NEW: set arr_start and arr_count using the deltas */
          if (mode_str != 0)
          {
            kv->val.arr_is_str = 1u;
            kv->val.arr_start  = str_before;
            kv->val.arr_count  = ini->str_slice_count - str_before;
          }
          else if (mode_i64 != 0)
          {
            kv->val.arr_is_i64 = 1u;
            kv->val.arr_start  = i64_before;
            kv->val.arr_count  = ini->nums_i64_count - i64_before;
          }
          else
          {
            kv->val.arr_is_f64 = 1u;
            kv->val.arr_start  = f64_before;
            kv->val.arr_count  = ini->nums_f64_count - f64_before;
          }
        }
        else
        {
          /* booleans (trimmed, exact case) */
          if ((vl2 == 4u && memcmp(vp2, "true", 4) == 0) ||
              (vl2 == 5u && memcmp(vp2, "false", 5) == 0))
          {
            kv->val.kind    = XINI_V_BOOL;
            kv->val.boolean = (vl2 == 4u) ? 1 : 0;

            if (ini->nodes[cur_node].kv_count == 0u)
            {
              ini->nodes[cur_node].kv_start = ini->kv_count;
            }
            ini->kv_count = ini->kv_count + 1u;
            ini->nodes[cur_node].kv_count = ini->nodes[cur_node].kv_count + 1u;

            i = i + l + ((i + l) < size ? 1u : 0u);
            continue;
          }

          /* Integer fast-path before strtod (scalar) */
          int is_int = 1;
          for (uint32_t q = 0u; q < vl2; q = q + 1u)
          {
            char c = vp2[q];
            if (c == '.' || c == 'e' || c == 'E') { is_int = 0; break; }
          }
          if (is_int)
          {
            long long iv = strtoll(vp2, NULL, 10);
            kv->val.kind = XINI_V_I64;
            kv->val.i64  = (int64_t)iv;
          }
          else
          {
            char *endp = NULL;
            double dv = strtod(vp2, &endp);
            if (endp == vp2 + (ptrdiff_t)vl2)
            {
              kv->val.kind = XINI_V_F64;
              kv->val.f64 = dv;
            }
            else
            {
              if (vl2 >= 2u && vp2[0] == '"' && vp2[vl2 - 1u] == '"')
              {
                kv->val.kind = XINI_V_STR;
                kv->val.off = (uint32_t)((vp2 + 1u) - ini->text);
                kv->val.len = vl2 - 2u;
              }
              else
              {
                kv->val.kind = XINI_V_STR;
                kv->val.off = (uint32_t)(vp2 - ini->text);
                kv->val.len = vl2;
              }
            }
          }
        }

        if (ini->nodes[cur_node].kv_count == 0u)
        {
          ini->nodes[cur_node].kv_start = ini->kv_count;
        }
        ini->kv_count = ini->kv_count + 1u;
        ini->nodes[cur_node].kv_count = ini->nodes[cur_node].kv_count + 1u;

        i = i + l + ((i + l) < size ? 1u : 0u);
        continue;
      }
    }

    i = i + l + ((i + l) < size ? 1u : 0u);
  }

  *out_ini = ini;
  return 1;
}

X_INI_API void x_ini_close(XIni *ini)
{
  if (ini == NULL)
  {
    return;
  }
  void *arena = ini->arena;
  free(arena);
}

/* Navigation */

X_INI_API int x_ini_section_name(XIni *ini, XIniCursor cur, const char **out_name, uint32_t *out_len)
{
  if (ini == NULL || out_name == NULL || out_len == NULL)
  {
    return 0;
  }
  if (cur.node < 0)
  {
    return 0;
  }
  const XIniNode *n = &ini->nodes[cur.node];
  *out_name = ini->text + n->name_off;
  *out_len = n->name_len;
  return 1;
}

X_INI_API int x_ini_child_count(XIni *ini, XIniCursor cur, uint32_t *out_count)
{
  if (ini == NULL || out_count == NULL)
  {
    return 0;
  }
  uint32_t count = 0u;
  if (cur.node < 0)
  {
    for (uint32_t i = 0u; i < ini->node_count; i = i + 1u)
    {
      if (ini->nodes[i].parent == -1)
      {
        count = count + 1u;
      }
    }
    *out_count = count;
    return 1;
  }

  int32_t it = ini->nodes[cur.node].first_child;
  while (it >= 0)
  {
    count = count + 1u;
    it = ini->nodes[it].next_sibling;
  }
  *out_count = count;
  return 1;
}

X_INI_API int x_ini_child_at(XIni *ini, XIniCursor cur, uint32_t index, XIniCursor *out_child)
{
  if (ini == NULL || out_child == NULL)
  {
    return 0;
  }
  if (cur.node < 0)
  {
    uint32_t count = 0u;
    for (uint32_t i = 0u; i < ini->node_count; i = i + 1u)
    {
      if (ini->nodes[i].parent == -1)
      {
        if (count == index)
        {
          out_child->node = (int)i;
          return 1;
        }
        count = count + 1u;
      }
    }
    return 0;
  }

  int32_t it = ini->nodes[cur.node].first_child;
  uint32_t k = 0u;
  while (it >= 0)
  {
    if (k == index)
    {
      out_child->node = it;
      return 1;
    }
    k = k + 1u;
    it = ini->nodes[it].next_sibling;
  }
  return 0;
}

X_INI_API int x_ini_find_child(XIni *ini, XIniCursor cur, const char *name, uint32_t name_len, XIniCursor *out_child)
{
  if (ini == NULL || name == NULL || out_child == NULL)
  {
    return 0;
  }

  int idx = s_ini_find_child_linear(ini, cur.node, name, name_len);
  if (idx < 0)
  {
    return 0;
  }
  out_child->node = idx;
  return 1;
}

X_INI_API int x_ini_get_section(XIni *ini, XIniCursor parent, const char *path, XIniCursor *out)
{
  if (ini == NULL || path == NULL || out == NULL)
  {
    return 0;
  }

  uint32_t plen = (uint32_t)strlen(path);
  uint32_t offv[32];
  uint32_t lenv[32];
  uint32_t n = s_ini_split_path(path, plen, offv, lenv, 32u);
  int cur = parent.node;

  for (uint32_t i = 0u; i < n; i = i + 1u)
  {
    int child = s_ini_find_child_linear(ini, cur, path + offv[i], lenv[i]);
    if (child < 0)
    {
      return 0;
    }
    cur = child;
  }

  out->node = cur;
  return 1;
}

/* Entries */

static int s_ini_find_kv(const XIni *ini, int node, const char *key, uint32_t *out_idx)
{
  if (node < 0)
  {
    return 0;
  }
  const XIniNode *nd = &ini->nodes[node];
  uint32_t start = nd->kv_start;
  uint32_t end = nd->kv_start + nd->kv_count;

  uint32_t klen = (uint32_t)strlen(key);

  for (uint32_t i = start; i < end; i = i + 1u)
  {
    const XIniKV *kv = &ini->kvs[i];
    const char *kk = ini->text + kv->key_off;
    if (s_ini_str_equal(kk, kv->key_len, key, klen) != 0)
    {
      *out_idx = i;
      return 1;
    }
  }
  return 0;
}

X_INI_API int x_ini_entry_count(XIni *ini, XIniCursor cur, uint32_t *out_count)
{
  if (ini == NULL || out_count == NULL)
  {
    return 0;
  }
  if (cur.node < 0)
  {
    *out_count = 0u;
    return 1;
  }
  *out_count = ini->nodes[cur.node].kv_count;
  return 1;
}

X_INI_API int x_ini_entry_key_at(XIni *ini, XIniCursor cur, uint32_t index, const char **out_key, uint32_t *out_key_len)
{
  if (ini == NULL || out_key == NULL || out_key_len == NULL)
  {
    return 0;
  }
  if (cur.node < 0)
  {
    return 0;
  }

  const XIniNode *nd = &ini->nodes[cur.node];
  if (index >= nd->kv_count)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[nd->kv_start + index];
  *out_key = ini->text + kv->key_off;
  *out_key_len = kv->key_len;
  return 1;
}

X_INI_API int x_ini_get_bool(XIni *ini, XIniCursor cur, const char *key, int *out_bool)
{
  if (ini == NULL || key == NULL || out_bool == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind != XINI_V_BOOL)
  {
    return 0;
  }
  *out_bool = kv->val.boolean;
  return 1;
}

X_INI_API int x_ini_get_i64(XIni *ini, XIniCursor cur, const char *key, int64_t *out_i64)
{
  if (ini == NULL || key == NULL || out_i64 == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind == XINI_V_I64)
  {
    *out_i64 = kv->val.i64;
    return 1;
  }
  if (kv->val.kind == XINI_V_F64)
  {
    *out_i64 = (int64_t)kv->val.f64;
    return 1;
  }
  return 0;
}

X_INI_API int x_ini_get_f64(XIni *ini, XIniCursor cur, const char *key, double *out_f64)
{
  if (ini == NULL || key == NULL || out_f64 == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind == XINI_V_F64)
  {
    *out_f64 = kv->val.f64;
    return 1;
  }
  if (kv->val.kind == XINI_V_I64)
  {
    *out_f64 = (double)kv->val.i64;
    return 1;
  }
  return 0;
}

X_INI_API int x_ini_get_str(XIni *ini, XIniCursor cur, const char *key, const char **out_str, uint32_t *out_len)
{
  if (ini == NULL || key == NULL || out_str == NULL || out_len == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind != XINI_V_STR)
  {
    return 0;
  }
  *out_str = ini->text + kv->val.off;
  *out_len = kv->val.len;
  return 1;
}

/* Arrays: now return already-tokenized slices if available. */

static int s_ini_collect_array(XIni *ini, const XIniKV *kv,
    const double **out_f64, uint32_t *out_nf64,
    const int64_t **out_i64, uint32_t *out_ni64,
    const XIniStrSlice **out_str, uint32_t *out_ns)
{
  if (kv->val.kind != XINI_V_ARR)
  {
    return 0;
  }

  /* Fast path: parsed once during load, just slice the pool. */
  if (kv->val.arr_count > 0)
  {
    if (kv->val.arr_is_f64 && out_f64 && out_nf64)
    {
      *out_f64  = ini->nums_f64 + kv->val.arr_start;
      *out_nf64 = kv->val.arr_count;
      if (out_i64) *out_i64 = NULL; if (out_ni64) *out_ni64 = 0;
      if (out_str) *out_str = NULL; if (out_ns)   *out_ns   = 0;
      return 1;
    }
    if (kv->val.arr_is_i64 && out_i64 && out_ni64)
    {
      *out_i64  = ini->nums_i64 + kv->val.arr_start;
      *out_ni64 = kv->val.arr_count;
      if (out_f64) *out_f64 = NULL; if (out_nf64) *out_nf64 = 0;
      if (out_str) *out_str = NULL; if (out_ns)   *out_ns   = 0;
      return 1;
    }
    if (kv->val.arr_is_str && out_str && out_ns)
    {
      *out_str = ini->str_slices + kv->val.arr_start;
      *out_ns  = kv->val.arr_count;
      if (out_f64) *out_f64 = NULL; if (out_nf64) *out_nf64 = 0;
      if (out_i64) *out_i64 = NULL; if (out_ni64) *out_ni64 = 0;
      return 1;
    }
    /* If types don't match the requested output, fall through to legacy parse and return 0 below. */
  }

  /* Legacy slow path (should be rare now): re-tokenize on demand. */
  const char *vp = ini->text + kv->val.off;
  uint32_t vl = kv->val.len;

  uint32_t nf64_before = ini->nums_f64_count;
  uint32_t ni64_before = ini->nums_i64_count;
  uint32_t ns_before = ini->str_slice_count;

  uint32_t start = 0u;
  int inq = 0;
  while (start <= vl)
  {
    uint32_t end = start;
    inq = 0;
    while (end < vl)
    {
      char c = vp[end];
      if (c == '"')
      {
        inq = !inq;
      }
      if (c == ',' && inq == 0)
      {
        break;
      }
      end = end + 1u;
    }

    const char *ep = vp + start;
    uint32_t el = end - start;
    while (el > 0u && (ep[0] == ' ' || ep[0] == '\t'))
    {
      ep = ep + 1u;
      el = el - 1u;
    }
    while (el > 0u && (ep[el - 1u] == ' ' || ep[el - 1u] == '\t' || ep[el - 1u] == '\r'))
    {
      el = el - 1u;
    }

    if (el > 0u)
    {
      if (ep[0] == '"' && el >= 2u && ep[el - 1u] == '"')
      {
        ini->str_slices[ini->str_slice_count].ptr = ep + 1u;
        ini->str_slices[ini->str_slice_count].len = el - 2u;
        ini->str_slice_count = ini->str_slice_count + 1u;
      }
      else
      {
        /* Integer fast-path before strtod (array element) */
        int is_int = 1;
        for (uint32_t q = 0u; q < el; q = q + 1u)
        {
          char c2 = ep[q];
          if (c2 == '.' || c2 == 'e' || c2 == 'E') { is_int = 0; break; }
        }
        if (is_int)
        {
          long long iv = strtoll(ep, NULL, 10);
          ini->nums_i64[ini->nums_i64_count] = (int64_t)iv;
          ini->nums_i64_count = ini->nums_i64_count + 1u;
        }
        else
        {
          char *endp = NULL;
          double dv = strtod(ep, &endp);
          if (endp == ep + (ptrdiff_t)el)
          {
            ini->nums_f64[ini->nums_f64_count] = dv;
            ini->nums_f64_count = ini->nums_f64_count + 1u;
          }
          else
          {
            ini->str_slices[ini->str_slice_count].ptr = ep;
            ini->str_slices[ini->str_slice_count].len = el;
            ini->str_slice_count = ini->str_slice_count + 1u;
          }
        }
      }
    }

    if (end >= vl)
    {
      start = vl + 1u;
    }
    else
    {
      start = end + 1u;
    }
  }

  if (out_f64 != NULL && out_nf64 != NULL)
  {
    *out_f64 = ini->nums_f64 + nf64_before;
    *out_nf64 = ini->nums_f64_count - nf64_before;
  }
  if (out_i64 != NULL && out_ni64 != NULL)
  {
    *out_i64 = ini->nums_i64 + ni64_before;
    *out_ni64 = ini->nums_i64_count - ni64_before;
  }
  if (out_str != NULL && out_ns != NULL)
  {
    *out_str = ini->str_slices + ns_before;
    *out_ns = ini->str_slice_count - ns_before;
  }
  return 1;
}

X_INI_API int x_ini_get_array_len(XIni *ini, XIniCursor cur, const char *key, uint32_t *out_len)
{
  if (ini == NULL || key == NULL || out_len == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind != XINI_V_ARR)
  {
    return 0;
  }

  const double *f = NULL;
  const int64_t *i64 = NULL;
  const XIniStrSlice *ss = NULL;
  uint32_t nf = 0u;
  uint32_t ni = 0u;
  uint32_t ns = 0u;
  if (s_ini_collect_array(ini, kv, &f, &nf, &i64, &ni, &ss, &ns) == 0)
  {
    return 0;
  }
  *out_len = nf + ni + ns;
  return 1;
}

X_INI_API int x_ini_get_array_f64(XIni *ini, XIniCursor cur, const char *key, const double **out_vals, uint32_t *out_len)
{
  if (ini == NULL || key == NULL || out_vals == NULL || out_len == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind != XINI_V_ARR)
  {
    return 0;
  }

  const double *f = NULL;
  const int64_t *i64 = NULL;
  const XIniStrSlice *ss = NULL;
  uint32_t nf = 0u;
  uint32_t ni = 0u;
  uint32_t ns = 0u;
  if (s_ini_collect_array(ini, kv, &f, &nf, &i64, &ni, &ss, &ns) == 0)
  {
    return 0;
  }
  if (nf == 0u)
  {
    return 0;
  }
  *out_vals = f;
  *out_len = nf;
  return 1;
}

X_INI_API int x_ini_get_array_i64(XIni *ini, XIniCursor cur, const char *key, const int64_t **out_vals, uint32_t *out_len)
{
  if (ini == NULL || key == NULL || out_vals == NULL || out_len == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind != XINI_V_ARR)
  {
    return 0;
  }

  const double *f = NULL;
  const int64_t *i64 = NULL;
  const XIniStrSlice *ss = NULL;
  uint32_t nf = 0u;
  uint32_t ni = 0u;
  uint32_t ns = 0u;
  if (s_ini_collect_array(ini, kv, &f, &nf, &i64, &ni, &ss, &ns) == 0)
  {
    return 0;
  }
  if (ni == 0u)
  {
    return 0;
  }
  *out_vals = i64;
  *out_len = ni;
  return 1;
}

X_INI_API int x_ini_get_array_str(XIni *ini, XIniCursor cur, const char *key, const XIniStrSlice **out_vals, uint32_t *out_len)
{
  if (ini == NULL || key == NULL || out_vals == NULL || out_len == NULL)
  {
    return 0;
  }
  uint32_t idx = 0u;
  if (s_ini_find_kv(ini, cur.node, key, &idx) == 0)
  {
    return 0;
  }
  const XIniKV *kv = &ini->kvs[idx];
  if (kv->val.kind != XINI_V_ARR)
  {
    return 0;
  }

  const double *f = NULL;
  const int64_t *i64 = NULL;
  const XIniStrSlice *ss = NULL;
  uint32_t nf = 0u;
  uint32_t ni = 0u;
  uint32_t ns = 0u;
  if (s_ini_collect_array(ini, kv, &f, &nf, &i64, &ni, &ss, &ns) == 0)
  {
    return 0;
  }
  if (ns == 0u)
  {
    return 0;
  }
  *out_vals = ss;
  *out_len = ns;
  return 1;
}


#endif /* X_IMPL_INI */
#endif /* X_INI_H */
