/* 
 * STDX - Tree Markup Language (TML)
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Author: marciof
 * License: MIT
 *
 * To compile the implementation define X_IMPL_IO
 * in **one** source file before including this header.
 *
 *  TML is a minimal, indentation-based hierarchical data format.
 *  It borrows the readability of YAML but is parsed in a single pass,
 *  with one arena allocation for all internal data.
 * 
 *  Syntax overview:
 *  - Indentation defines hierarchy. The first indented line sets the unit (2 or 4 spaces).
 *  - Tabs are invalid.
 *  - Lines ending with ':' define a new section (node).
 *  - Key/value pairs use either ':' or '=' separators:
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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef X_TML_API
#define X_TML_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XTml XTml;

  typedef struct XTmlCursor
  {
    int node;
  } XTmlCursor;

  typedef enum XTmlValKind
  {
    XTML_V_NONE = 0,
    XTML_V_STR  = 1,
    XTML_V_I64  = 2,
    XTML_V_F64  = 3,
    XTML_V_BOOL = 4,
    XTML_V_ARR  = 5
  } XTmlValKind;

  typedef struct XTmlStrSlice
  {
    const char *ptr;
    uint32_t    len;
  } XTmlStrSlice;

  enum
  {
    XTML_OPEN_DEFAULT = 0u
  };

  X_TML_API int  x_tml_load(const char *buf, uint32_t size, uint32_t flags, XTml **out_tml);
  X_TML_API void x_tml_unload(XTml *tml);
  X_TML_API XTmlCursor x_tml_root(XTml *tml);
  X_TML_API int x_tml_get_section(XTml *tml, XTmlCursor parent, const char *dot_path, XTmlCursor *out);
  X_TML_API int x_tml_child_count(XTml *tml, XTmlCursor cur, uint32_t *out_count);
  X_TML_API int x_tml_child_at(XTml *tml, XTmlCursor cur, uint32_t index, XTmlCursor *out_child);
  X_TML_API int x_tml_find_child(XTml *tml, XTmlCursor cur, const char *name, uint32_t name_len, XTmlCursor *out_child);
  X_TML_API int x_tml_section_name(XTml *tml, XTmlCursor cur, const char **out_name, uint32_t *out_len);

  X_TML_API int x_tml_entry_count (XTml *tml, XTmlCursor cur, uint32_t *out_count);
  X_TML_API int x_tml_entry_key_at(XTml *tml, XTmlCursor cur, uint32_t index, const char **out_key, uint32_t *out_key_len);

  X_TML_API int x_tml_get_bool(XTml *tml, XTmlCursor cur, const char *key, int *out_bool);
  X_TML_API int x_tml_get_i64 (XTml *tml, XTmlCursor cur, const char *key, int64_t *out_i64);
  X_TML_API int x_tml_get_f64 (XTml *tml, XTmlCursor cur, const char *key, double *out_f64);
  X_TML_API int x_tml_get_str (XTml *tml, XTmlCursor cur, const char *key, const char **out_str, uint32_t *out_len);

  X_TML_API int x_tml_get_array_len (XTml *tml, XTmlCursor cur, const char *key, uint32_t *out_len);
  X_TML_API int x_tml_get_array_f64(XTml *tml, XTmlCursor cur, const char *key, const double **out_vals, uint32_t *out_len);
  X_TML_API int x_tml_get_array_i64(XTml *tml, XTmlCursor cur, const char *key, const int64_t **out_vals, uint32_t *out_len);
  X_TML_API int x_tml_get_array_str(XTml *tml, XTmlCursor cur, const char *key, const XTmlStrSlice **out_vals, uint32_t *out_len);
  X_TML_API int x_tml_has_key(XTml *tml, XTmlCursor cur, const char *key);
  X_TML_API int x_tml_dump(XTml *tml, void *f /* FILE* ou stdout if NULL */);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef X_IMPL_TML

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifndef X_TML_ALLOC
#define X_TML_ALLOC(sz)        malloc(sz)
#define X_TML_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XTmlValue
  {
    XTmlValKind kind;
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
  } XTmlValue;

  typedef struct XTmlKV
  {
    uint32_t key_off;
    uint32_t key_len;
    XTmlValue val;
  } XTmlKV;

  typedef struct XTmlNode
  {
    uint32_t name_off;
    uint32_t name_len;
    uint32_t name_hash;
    int32_t  parent;
    int32_t  first_child;
    int32_t  next_sibling;
    uint32_t kv_start;
    uint32_t kv_count;
  } XTmlNode;

  struct XTml
  {
    const char *text;
    uint32_t    text_len;

    void      *arena;
    uint32_t   arena_size;

    XTmlNode  *nodes;
    uint32_t   node_count;

    XTmlKV    *kvs;
    uint32_t   kv_count;

    double    *nums_f64;
    uint32_t   nums_f64_count;

    int64_t   *nums_i64;
    uint32_t   nums_i64_count;

    XTmlStrSlice *str_slices;
    uint32_t      str_slice_count;
  };

  /* Arena */
  static void *s_tml_arena_alloc(void *base,
      uint32_t *offset,
      uint32_t need,
      uint32_t total)
  {
    if (*offset + need > total)
    {
      return NULL;
    }

    char *b = (char *)base;
    void *p = b + *offset;

    *offset += need;

    return p;
  }

  /* FNV-1a 32 */
  static uint32_t s_tml_hash32(const char *s, uint32_t len)
  {
    if (!len)
    {
      return 0u;
    }

    uint32_t h = 2166136261u;

    for (uint32_t i = 0; i < len; i++)
    {
      h ^= (uint8_t)s[i];
      h *= 16777619u;
    }

    if (!h)
    {
      return 1u;
    }

    return h;
  }

  static uint32_t s_tml_trim_right_len(const char *s, uint32_t len)
  {
    while (len)
    {
      char c = s[len - 1];

      if (c == ' ' || c == '\t' || c == '\r')
      {
        len--;
        continue;
      }

      break;
    }

    return len;
  }

  static int s_tml_str_equal(const char *a, uint32_t al, const char *b, uint32_t bl)
  {
    if (al != bl)
    {
      return 0;
    }

    if (al == 0)
    {
      return 1;
    }

    return memcmp(a, b, al) == 0;
  }

  /* split "a.b.c" */
  static uint32_t s_tml_split_dotpath(const char *path,
      uint32_t len,
      uint32_t *offv,
      uint32_t *lenv,
      uint32_t max_seg)
  {
    uint32_t n = 0;
    uint32_t i = 0;
    uint32_t start = 0;

    while (i <= len)
    {
      char c = (i < len) ? path[i] : '.';

      if (c == '.')
      {
        if (i > start && n < max_seg)
        {
          offv[n] = start;
          lenv[n] = i - start;
          n++;
        }

        start = i + 1;
      }

      i++;
    }

    return n;
  }

  static int s_tml_find_child_linear(XTml *tml,
      int parent_idx,
      const char *name,
      uint32_t len)
  {
    if (len == 0)
    {
      return -1;
    }

    uint32_t h = s_tml_hash32(name, len);

    if (parent_idx < 0)
    {
      for (uint32_t i = 0; i < tml->node_count; i++)
      {
        const XTmlNode *nd = &tml->nodes[i];

        if (nd->parent != -1)
        {
          continue;
        }

        if (nd->name_len == len && nd->name_hash == h)
        {
          const char *np = tml->text + nd->name_off;

          if (memcmp(np, name, len) == 0)
          {
            return (int)i;
          }
        }
      }

      return -1;
    }

    int32_t it = tml->nodes[parent_idx].first_child;

    while (it >= 0)
    {
      const XTmlNode *nd = &tml->nodes[it];

      if (nd->name_len == len && nd->name_hash == h)
      {
        const char *np = tml->text + nd->name_off;

        if (memcmp(np, name, len) == 0)
        {
          return it;
        }
      }

      it = nd->next_sibling;
    }

    return -1;
  }

  static int s_tml_add_child(XTml *tml,
      int parent_idx,
      uint32_t name_off,
      uint32_t name_len,
      uint32_t *next_X_TML_FREE_node)
  {
    int idx = (int)(*next_X_TML_FREE_node);
    *next_X_TML_FREE_node += 1u;

    XTmlNode *n = &tml->nodes[idx];

    n->name_off = name_off;
    n->name_len = name_len;
    n->name_hash = (name_len ? s_tml_hash32(tml->text + name_off, name_len) : 0);
    n->parent = parent_idx;
    n->first_child = -1;
    n->next_sibling = -1;
    n->kv_start = 0;
    n->kv_count = 0;

    if (parent_idx >= 0)
    {
      if (tml->nodes[parent_idx].first_child < 0)
      {
        tml->nodes[parent_idx].first_child = idx;
      }
      else
      {
        int32_t it = tml->nodes[parent_idx].first_child;

        while (tml->nodes[it].next_sibling >= 0)
        {
          it = tml->nodes[it].next_sibling;
        }

        tml->nodes[it].next_sibling = idx;
      }
    }

    return idx;
  }

  static void s_tml_dump_section(XTml *tml, int node, int depth, FILE *f)
  {
    const XTmlNode *nd = &tml->nodes[node];

    for (int d = 0; d < depth; ++d)
    {
      fprintf(f, "  ");
    }

    if (nd->name_len == 0)
    {
      fprintf(f, "-\n");
    }
    else
    {
      fprintf(f, "%.*s:\n", nd->name_len, tml->text + nd->name_off);
    }

    for (uint32_t i = 0; i < nd->kv_count; i++)
    {
      const XTmlKV *kv = &tml->kvs[nd->kv_start + i];
      const XTmlValue *v = &kv->val;
      const char *key = tml->text + kv->key_off;

      for (int d = 0; d < depth + 1; ++d)
      {
        fprintf(f, "  ");
      }

      fprintf(f, "%.*s = ", kv->key_len, key);

      switch (v->kind)
      {
        case XTML_V_BOOL:
          {
            fprintf(f, "%s (bool)\n", v->boolean ? "true" : "false");
          } break;

        case XTML_V_I64:
          {
            fprintf(f, "%lld (i64)\n", (long long)v->i64);
          } break;

        case XTML_V_F64:
          {
            fprintf(f, "%g (f64)\n", v->f64);
          } break;

        case XTML_V_STR:
          {
            const char *sp = tml->text + v->off;
            int has_nl = 0;

            for (uint32_t t = 0; t < v->len; t++)
            {
              if (sp[t] == '\n')
              {
                has_nl = 1;
                break;
              }
            }

            if (has_nl)
            {
              fprintf(f, "\"\"\"\n%.*s\n\"\"\" (str)\n", v->len, sp);
            }
            else
            {
              fprintf(f, "\"%.*s\" (str)\n", v->len, sp);
            }
          } break;

        case XTML_V_ARR:
          {
            if (v->arr_count == 0)
            {
              fprintf(f, "[] (arr)\n");
              break;
            }

            if (v->arr_is_i64)
            {
              fprintf(f, "[");

              for (uint32_t k = 0; k < v->arr_count; k++)
              {
                if (k)
                {
                  fprintf(f, ", ");
                }

                fprintf(f, "%lld", (long long)tml->nums_i64[v->arr_start + k]);
              }

              fprintf(f, "] (i64[])\n");
            }
            else if (v->arr_is_f64)
            {
              fprintf(f, "[");

              for (uint32_t k = 0; k < v->arr_count; k++)
              {
                if (k)
                {
                  fprintf(f, ", ");
                }

                fprintf(f, "%g", tml->nums_f64[v->arr_start + k]);
              }

              fprintf(f, "] (f64[])\n");
            }
            else if (v->arr_is_str)
            {
              fprintf(f, "[");

              for (uint32_t k = 0; k < v->arr_count; k++)
              {
                if (k)
                {
                  fprintf(f, ", ");
                }

                const XTmlStrSlice *s = &tml->str_slices[v->arr_start + k];
                fprintf(f, "\"%.*s\"", s->len, s->ptr);
              }

              fprintf(f, "] (str[])\n");
            }
          } break;

        default:
          {
            fprintf(f, "(unknown)\n");
          } break;
      }
    }

    int32_t child = nd->first_child;

    while (child >= 0)
    {
      s_tml_dump_section(tml, child, depth + 1, f);
      child = tml->nodes[child].next_sibling;
    }
  }

  X_TML_API int x_tml_dump(XTml *tml, void *vf)
  {
    if (!tml)
    {
      return 0;
    }

    FILE *f = (FILE *)vf;

    if (!f)
    {
      f = stdout;
    }

    for (uint32_t i = 0; i < tml->node_count; i++)
    {
      if (tml->nodes[i].parent == -1)
      {
        s_tml_dump_section(tml, (int)i, 0, f);
      }
    }

    return 1;
  }

  static int s_tml_find_triple_end(const char *text,
      uint32_t size,
      uint32_t start_off,
      uint32_t *out_end_off)
  {
    if (!text || start_off + 2u >= size)
    {
      return 0;
    }

    uint32_t i = start_off;

    while (i + 2u < size)
    {
      if (text[i] == '"' && text[i + 1] == '"' && text[i + 2] == '"')
      {
        *out_end_off = i;
        return 1;
      }

      i++;
    }

    return 0;
  }

  static uint32_t s_tml_advance_to_next_line_after(uint32_t off,
      const char *buf,
      uint32_t size)
  {
    uint32_t i = off;

    while (i < size && buf[i] != '\n')
    {
      i++;
    }

    if (i < size && buf[i] == '\n')
    {
      i++;
    }

    return i;
  }

  /* prescan */
  typedef struct s_tml_counts
  {
    uint32_t headers;
    uint32_t kvs;
    uint32_t comma_tokens;
    uint32_t triple_blocks;
  } s_tml_counts;

  static void s_tml_prescan(const char *buf, uint32_t len, s_tml_counts *out)
  {
    out->headers = 0;
    out->kvs = 0;
    out->comma_tokens = 0;
    out->triple_blocks = 0;

    uint32_t i = 0;
    int in_triple = 0;

    while (i < len)
    {
      const char *line = buf + i;
      uint32_t l = 0;

      while (i + l < len && buf[i + l] != '\n')
      {
        l++;
      }

      uint32_t tl = s_tml_trim_right_len(line, l);

      if (tl == 0)
      {
        i += l + ((i + l) < len ? 1u : 0u);
        continue;
      }

      if (line[0] == '#')
      {
        i += l + ((i + l) < len ? 1u : 0u);
        continue;
      }

      if (!in_triple)
      {
        uint32_t ind = 0;

        while (ind < tl && line[ind] == ' ')
        {
          ind++;
        }

        const char *p = line + ind;
        uint32_t pl = (tl > ind) ? (tl - ind) : 0u;

        for (uint32_t k = 0; k + 2 < pl; k++)
        {
          if (p[k] == '"' && p[k + 1] == '"' && p[k + 2] == '"')
          {
            in_triple = 1;
            out->triple_blocks++;
            break;
          }
        }

        if (pl == 0)
        {
          i += l + ((i + l) < len ? 1u : 0u);
          continue;
        }

        if (p[0] == '-')
        {
          out->headers++;
          i += l + ((i + l) < len ? 1u : 0u);
          continue;
        }

        uint32_t pos = UINT32_MAX;

        for (uint32_t k = 0; k < pl; k++)
        {
          char c = p[k];

          if (c == ':' || c == '=')
          {
            pos = k;
            break;
          }
        }

        if (pos != UINT32_MAX)
        {
          int is_colon = (p[pos] == ':');
          uint32_t after = pos + 1u;

          while (after < pl && (p[after] == ' ' || p[after] == '\t'))
          {
            after++;
          }

          if (is_colon && after >= pl)
          {
            out->headers++;
          }
          else
          {
            out->kvs++;
          }

          if (after < pl)
          {
            for (uint32_t k = after; k < pl; k++)
            {
              if (p[k] == ',')
              {
                out->comma_tokens++;
              }
            }

            if (pl > 0 && p[pl - 1] == ',')
            {
              uint32_t j = i + l + ((i + l) < len ? 1u : 0u);

              while (j < len)
              {
                const char *ln = buf + j;
                uint32_t ll = 0;

                while (j + ll < len && buf[j + ll] != '\n')
                {
                  ll++;
                }

                uint32_t tll = s_tml_trim_right_len(ln, ll);

                for (uint32_t kk = 0; kk < tll; kk++)
                {
                  if (ln[kk] == ',')
                  {
                    out->comma_tokens++;
                  }
                }

                if (tll == 0 || ln[tll - 1] != ',')
                {
                  break;
                }

                j += ll + ((j + ll) < len ? 1u : 0u);
              }
            }
          }
        }
      }
      else
      {
        for (uint32_t k = 0; k + 2 < tl; k++)
        {
          if (line[k] == '"' && line[k + 1] == '"' && line[k + 2] == '"')
          {
            in_triple = 0;
            break;
          }
        }
      }

      i += l + ((i + l) < len ? 1u : 0u);
    }
  }

  /* Parser (UNIT=2 or 4, tabs are invalid) */
  typedef struct s_stack_entry
  {
    uint32_t depth;
    int node;
  } s_stack_entry;

  X_TML_API XTmlCursor x_tml_root(XTml *tml)
  {
    (void)tml;
    XTmlCursor c;
    c.node = -1;
    return c;
  }

  X_TML_API int x_tml_load(const char *buf,
      uint32_t size,
      uint32_t flags,
      XTml **out_tml)
  {
    (void)flags;

    if (!buf || !out_tml || !size)
    {
      return 0;
    }

    s_tml_counts cnt;
    s_tml_prescan(buf, size, &cnt);

    uint32_t max_nodes = cnt.headers + 8u;
    uint32_t max_kvs = cnt.kvs + 8u;
    uint32_t max_nums = cnt.comma_tokens + cnt.kvs + 8u;
    uint32_t max_strs = cnt.comma_tokens + 8u;

    uint32_t arena_bytes = 0;
    arena_bytes += sizeof(XTml);
    arena_bytes += max_nodes * (uint32_t)sizeof(XTmlNode);
    arena_bytes += max_kvs * (uint32_t)sizeof(XTmlKV);
    arena_bytes += max_nums * (uint32_t)sizeof(double);
    arena_bytes += max_nums * (uint32_t)sizeof(int64_t);
    arena_bytes += max_strs * (uint32_t)sizeof(XTmlStrSlice);

    void *arena = X_TML_ALLOC(arena_bytes);
    if (!arena)
    {
      return 0;
    }

    uint32_t off = 0;

    XTml *tml = (XTml *)s_tml_arena_alloc(arena, &off, (uint32_t)sizeof(XTml), arena_bytes);
    tml->arena = arena;
    tml->arena_size = arena_bytes;
    tml->text = buf;
    tml->text_len = size;

    tml->nodes = (XTmlNode *)s_tml_arena_alloc(arena, &off, max_nodes * (uint32_t)sizeof(XTmlNode), arena_bytes);
    tml->node_count = 0;

    tml->kvs = (XTmlKV *)s_tml_arena_alloc(arena, &off, max_kvs * (uint32_t)sizeof(XTmlKV), arena_bytes);
    tml->kv_count = 0;

    tml->nums_f64 = (double *)s_tml_arena_alloc(arena, &off, max_nums * (uint32_t)sizeof(double), arena_bytes);
    tml->nums_f64_count = 0;

    tml->nums_i64 = (int64_t *)s_tml_arena_alloc(arena, &off, max_nums * (uint32_t)sizeof(int64_t), arena_bytes);
    tml->nums_i64_count = 0;

    tml->str_slices = (XTmlStrSlice *)s_tml_arena_alloc(arena, &off, max_strs * (uint32_t)sizeof(XTmlStrSlice), arena_bytes);
    tml->str_slice_count = 0;

    s_stack_entry stack[128];
    uint32_t sp = 0;

    int cur_node = -1;
    uint32_t next_free_node = 0;

    uint32_t i = 0;
    uint32_t unit = 0;

    while (i < size)
    {
      const char *line = buf + i;
      uint32_t l = 0;

      while (i + l < size && buf[i + l] != '\n')
      {
        l++;
      }

      uint32_t tl = s_tml_trim_right_len(line, l);
      if (tl == 0)
      {
        i += l + ((i + l) < size ? 1u : 0u);
        continue;
      }

      if (line[0] == '#')
      {
        i += l + ((i + l) < size ? 1u : 0u);
        continue;
      }

      uint32_t ind = 0;
      while (ind < tl && line[ind] == ' ')
      {
        ind++;
      }

      if (ind < tl && line[ind] == '\t')
      {
        x_tml_unload(tml);
        return 0;
      }

      if (unit == 0 && ind > 0)
      {
        unit = ind;
        if (!(unit == 2 || unit == 4))
        {
          x_tml_unload(tml);
          return 0;
        }
      }

      if (ind > 0)
      {
        if (unit == 0)
        {
          x_tml_unload(tml);
          return 0;
        }
        if ((ind % unit) != 0)
        {
          x_tml_unload(tml);
          return 0;
        }
      }

      uint32_t depth = (unit > 0) ? (ind / unit) : 0;

      const char *p = line + ind;
      uint32_t pl = (tl > ind) ? (tl - ind) : 0u;
      if (pl == 0)
      {
        i += l + ((i + l) < size ? 1u : 0u);
        continue;
      }

      while (sp > 0 && stack[sp - 1].depth >= depth)
      {
        sp--;
      }

      cur_node = (sp > 0) ? stack[sp - 1].node : -1;

      int inline_after_dash = 0;
      const char *p_after_dash = NULL;
      uint32_t pl_after_dash = 0;

      if (p[0] == '-' && (pl == 1u || p[1] == ' ' || p[1] == '\t'))
      {
        int anon = s_tml_add_child(tml, cur_node, 0u, 0u, &next_free_node);
        tml->node_count = next_free_node;

        stack[sp].depth = depth;
        stack[sp].node = anon;
        sp++;
        cur_node = anon;

        if (pl > 1u)
        {
          uint32_t sk = 1u;
          while (sk < pl && (p[sk] == ' ' || p[sk] == '\t'))
          {
            sk++;
          }

          if (sk < pl)
          {
            inline_after_dash = 1;
            p_after_dash = p + sk;
            pl_after_dash = pl - sk;
          }
          else
          {
            i += l + ((i + l) < size ? 1u : 0u);
            continue;
          }
        }
        else
        {
          i += l + ((i + l) < size ? 1u : 0u);
          continue;
        }
      }

      if (inline_after_dash)
      {
        p = p_after_dash;
        pl = pl_after_dash;
      }

      uint32_t pos = UINT32_MAX;
      for (uint32_t k = 0; k < pl; k++)
      {
        char c = p[k];
        if (c == ':' || c == '=')
        {
          pos = k;
          break;
        }
      }

      if (pos == UINT32_MAX)
      {
        i += l + ((i + l) < size ? 1u : 0u);
        continue;
      }

      int is_colon = (p[pos] == ':');
      uint32_t after = pos + 1u;

      while (after < pl && (p[after] == ' ' || p[after] == '\t'))
      {
        after++;
      }

      int is_section = (is_colon && after >= pl);

      if (is_section)
      {
        if (pos == 0u || isdigit((unsigned char)p[0]))
        {
          x_tml_unload(tml);
          return 0;
        }

        int exists = s_tml_find_child_linear(tml, cur_node, p, pos);
        if (exists >= 0)
        {
          x_tml_unload(tml);
          return 0;
        }

        int child = s_tml_add_child(tml, cur_node, (uint32_t)(p - tml->text), pos, &next_free_node);
        tml->node_count = next_free_node;

        stack[sp].depth = depth;
        stack[sp].node = child;
        sp++;
        cur_node = child;

        i += l + ((i + l) < size ? 1u : 0u);
        continue;
      }

      uint32_t k_off = 0u;
      uint32_t k_len = pos;
      while (k_off < k_len && (p[k_off] == ' ' || p[k_off] == '\t'))
      {
        k_off++;
      }

      const char *valp = p + pos + 1u;
      uint32_t v_len = pl - (pos + 1u);
      while (v_len > 0u && (*valp == ' ' || *valp == '\t'))
      {
        valp++;
        v_len--;
      }

      if (sp > 0)
      {
        uint32_t cur_depth = stack[sp - 1].depth;
        if (!inline_after_dash && depth <= cur_depth)
        {
          x_tml_unload(tml);
          return 0;
        }
      }

      uint32_t raw_off = (uint32_t)(valp - tml->text);
      uint32_t raw_len = v_len;

      if (raw_len >= 3u &&
          tml->text[raw_off] == '"' &&
          tml->text[raw_off + 1] == '"' &&
          tml->text[raw_off + 2] == '"')
      {
        uint32_t end_off = 0;
        int closed = s_tml_find_triple_end(tml->text, tml->text_len, raw_off + 3u, &end_off);

        XTmlKV *kv = &tml->kvs[tml->kv_count];
        kv->key_off = (uint32_t)((p + k_off) - tml->text);
        kv->key_len = k_len - k_off;
        kv->val.kind = XTML_V_STR;

        if (closed && end_off >= raw_off + 3u)
        {
          uint32_t content_lo = raw_off + 3u;
          uint32_t content_hi = end_off;

          if (content_lo < content_hi)
          {
            char c0 = tml->text[content_lo];
            if (c0 == '\r')
            {
              if (content_lo + 1u < content_hi && tml->text[content_lo + 1u] == '\n')
              {
                content_lo += 2u;
              }
              else
              {
                content_lo += 1u;
              }
            }
            else if (c0 == '\n')
            {
              content_lo += 1u;
            }
          }

          if (content_lo < content_hi)
          {
            uint32_t last = content_hi - 1u;
            if (tml->text[last] == '\n')
            {
              if (last > 0u && tml->text[last - 1u] == '\r')
              {
                content_hi -= 2u;
              }
              else
              {
                content_hi -= 1u;
              }
            }
            else if (tml->text[last] == '\r')
            {
              content_hi -= 1u;
            }
          }

          kv->val.off = content_lo;
          kv->val.len = (content_hi > content_lo) ? (content_hi - content_lo) : 0u;

          i = s_tml_advance_to_next_line_after(end_off, buf, size);
        }
        else
        {
          uint32_t content_lo = raw_off + 3u;
          uint32_t content_hi = tml->text_len;

          if (content_lo < content_hi)
          {
            char c0 = tml->text[content_lo];
            if (c0 == '\r')
            {
              if (content_lo + 1u < content_hi && tml->text[content_lo + 1u] == '\n')
              {
                content_lo += 2u;
              }
              else
              {
                content_lo += 1u;
              }
            }
            else if (c0 == '\n')
            {
              content_lo += 1u;
            }
          }

          kv->val.off = content_lo;
          kv->val.len = (content_hi > content_lo) ? (content_hi - content_lo) : 0u;

          i = size;
        }

        if (tml->nodes[cur_node].kv_count == 0)
        {
          tml->nodes[cur_node].kv_start = tml->kv_count;
        }

        tml->kv_count++;
        tml->nodes[cur_node].kv_count++;

        continue;
      }

      int did_multiline_array = 0;
      uint32_t carry_next_i = 0;

      if (v_len > 0u && valp[v_len - 1] == ',')
      {
        uint32_t j = i + l + ((i + l) < size ? 1u : 0u);

        while (j < size)
        {
          const char *ln = buf + j;
          uint32_t ll = 0;

          while (j + ll < size && buf[j + ll] != '\n')
          {
            ll++;
          }

          uint32_t tll = s_tml_trim_right_len(ln, ll);
          raw_len = (uint32_t)((ln + tll) - (tml->text + raw_off));

          if (tll == 0u)
          {
            j += ll + ((j + ll) < size ? 1u : 0u);
            break;
          }

          if (ln[tll - 1] != ',')
          {
            j += ll + ((j + ll) < size ? 1u : 0u);
            break;
          }

          j += ll + ((j + ll) < size ? 1u : 0u);
        }

        did_multiline_array = 1;
        carry_next_i = j;
      }

      XTmlKV *kv = &tml->kvs[tml->kv_count];
      kv->key_off = (uint32_t)((p + k_off) - tml->text);
      kv->key_len = k_len - k_off;
      kv->val.kind = XTML_V_STR;
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

      const char *vp = tml->text + raw_off;
      uint32_t vl = raw_len;

      int has_comma = 0;
      int in_str_q = 0;

      for (uint32_t t = 0; t < vl; t++)
      {
        char c = vp[t];
        if (c == '"')
        {
          in_str_q = !in_str_q;
        }
        if (c == ',' && !in_str_q)
        {
          has_comma = 1;
          break;
        }
      }

      uint32_t vl2 = vl;
      const char *vp2 = vp;

      while (vl2 > 0 && (*vp2 == ' ' || *vp2 == '\t'))
      {
        vp2++;
        vl2--;
      }

      while (vl2 > 0 && (vp2[vl2 - 1] == ' ' || vp2[vl2 - 1] == '\t' || vp2[vl2 - 1] == '\r'))
      {
        vl2--;
      }

      if (has_comma)
      {
        uint32_t start = 0u;
        int mode_f64 = 1;
        int mode_i64 = 1;
        int mode_str = 0;

        uint32_t f64_before = tml->nums_f64_count;
        uint32_t i64_before = tml->nums_i64_count;
        uint32_t str_before = tml->str_slice_count;

        while (start <= vl2)
        {
          uint32_t end = start;
          int inq = 0;

          while (end < vl2)
          {
            char c = vp2[end];
            if (c == '"')
            {
              inq = !inq;
            }
            if (c == ',' && !inq)
            {
              break;
            }
            end++;
          }

          const char *ep = vp2 + start;
          uint32_t el = end - start;

          while (el > 0 && (*ep == ' ' || *ep == '\t'))
          {
            ep++;
            el--;
          }

          while (el > 0 && (ep[el - 1] == ' ' || ep[el - 1] == '\t'))
          {
            el--;
          }

          if (el > 0)
          {
            if (ep[0] == '"' && el >= 2 && ep[el - 1] == '"')
            {
              mode_i64 = 0;
              mode_f64 = 0;
              mode_str = 1;

              tml->str_slices[tml->str_slice_count].ptr = ep + 1;
              tml->str_slices[tml->str_slice_count].len = el - 2;
              tml->str_slice_count++;
            }
            else
            {
              int is_int = 1;

              for (uint32_t q = 0; q < el; q++)
              {
                char c2 = ep[q];
                if (c2 == '.' || c2 == 'e' || c2 == 'E')
                {
                  is_int = 0;
                  break;
                }
              }

              if (is_int)
              {
                long long iv = strtoll(ep, NULL, 10);
                tml->nums_i64[tml->nums_i64_count] = (int64_t)iv;
                tml->nums_i64_count++;
                mode_f64 = 0;
              }
              else
              {
                char *endp = NULL;
                double dv = strtod(ep, &endp);
                if (endp == ep + (ptrdiff_t)el)
                {
                  tml->nums_f64[tml->nums_f64_count] = dv;
                  tml->nums_f64_count++;
                  mode_i64 = 0;
                }
                else
                {
                  mode_i64 = 0;
                  mode_f64 = 0;
                  mode_str = 1;

                  tml->str_slices[tml->str_slice_count].ptr = ep;
                  tml->str_slices[tml->str_slice_count].len = el;
                  tml->str_slice_count++;
                }
              }
            }
          }

          if (end >= vl2)
          {
            start = vl2 + 1;
          }
          else
          {
            start = end + 1;
          }
        }

        kv->val.kind = XTML_V_ARR;

        if (mode_str)
        {
          kv->val.arr_is_str = 1u;
          kv->val.arr_start = str_before;
          kv->val.arr_count = tml->str_slice_count - str_before;
        }
        else if (mode_i64)
        {
          kv->val.arr_is_i64 = 1u;
          kv->val.arr_start = i64_before;
          kv->val.arr_count = tml->nums_i64_count - i64_before;
        }
        else
        {
          kv->val.arr_is_f64 = 1u;
          kv->val.arr_start = f64_before;
          kv->val.arr_count = tml->nums_f64_count - f64_before;
        }
      }
      else
      {
        if ((vl2 == 4 && memcmp(vp2, "true", 4) == 0) ||
            (vl2 == 5 && memcmp(vp2, "false", 5) == 0))
        {
          kv->val.kind = XTML_V_BOOL;
          kv->val.boolean = (vl2 == 4) ? 1 : 0;

          if (tml->nodes[cur_node].kv_count == 0)
          {
            tml->nodes[cur_node].kv_start = tml->kv_count;
          }

          tml->kv_count++;
          tml->nodes[cur_node].kv_count++;

          if (did_multiline_array)
          {
            i = carry_next_i;
            continue;
          }

          i += l + ((i + l) < size ? 1u : 0u);
          continue;
        }

        int is_int = 1;
        for (uint32_t q = 0; q < vl2; q++)
        {
          char c = vp2[q];
          if (c == '.' || c == 'e' || c == 'E')
          {
            is_int = 0;
            break;
          }
        }

        if (is_int)
        {
          long long iv = strtoll(vp2, NULL, 10);
          kv->val.kind = XTML_V_I64;
          kv->val.i64 = (int64_t)iv;
        }
        else
        {
          char *endp = NULL;
          double dv = strtod(vp2, &endp);
          if (endp == vp2 + (ptrdiff_t)vl2)
          {
            kv->val.kind = XTML_V_F64;
            kv->val.f64 = dv;
          }
          else
          {
            if (vl2 >= 2 && vp2[0] == '"' && vp2[vl2 - 1] == '"')
            {
              kv->val.kind = XTML_V_STR;
              kv->val.off = (uint32_t)((vp2 + 1) - tml->text);
              kv->val.len = vl2 - 2;
            }
            else
            {
              kv->val.kind = XTML_V_STR;
              kv->val.off = (uint32_t)(vp2 - tml->text);
              kv->val.len = vl2;
            }
          }
        }
      }

      if (tml->nodes[cur_node].kv_count == 0)
      {
        tml->nodes[cur_node].kv_start = tml->kv_count;
      }

      tml->kv_count++;
      tml->nodes[cur_node].kv_count++;

      if (did_multiline_array)
      {
        i = carry_next_i;
        continue;
      }

      i += l + ((i + l) < size ? 1u : 0u);
    }

    *out_tml = tml;
    return 1;
  }

  X_TML_API void x_tml_unload(XTml *tml)
  {
    if (!tml)
    {
      return;
    }

    void *arena = tml->arena;

    X_TML_FREE(arena);
  }

  X_TML_API int x_tml_section_name(XTml *tml, XTmlCursor cur,
      const char **out_name, uint32_t *out_len)
  {
    if (!tml || !out_name || !out_len)
    {
      return 0;
    }

    if (cur.node < 0)
    {
      return 0;
    }

    const XTmlNode *n = &tml->nodes[cur.node];

    *out_name = (n->name_len ? tml->text + n->name_off : NULL);
    *out_len  = n->name_len;

    return 1;
  }

  X_TML_API int x_tml_child_count(XTml *tml, XTmlCursor cur, uint32_t *out_count)
  {
    if (!tml || !out_count)
    {
      return 0;
    }

    uint32_t count = 0;

    if (cur.node < 0)
    {
      for (uint32_t i = 0; i < tml->node_count; i++)
      {
        if (tml->nodes[i].parent == -1)
        {
          count++;
        }
      }

      *out_count = count;
      return 1;
    }

    int32_t it = tml->nodes[cur.node].first_child;

    while (it >= 0)
    {
      count++;
      it = tml->nodes[it].next_sibling;
    }

    *out_count = count;

    return 1;
  }

  X_TML_API int x_tml_child_at(XTml *tml, XTmlCursor cur, uint32_t index,
      XTmlCursor *out_child)
  {
    if (!tml || !out_child)
    {
      return 0;
    }

    if (cur.node < 0)
    {
      uint32_t count = 0;

      for (uint32_t i = 0; i < tml->node_count; i++)
      {
        if (tml->nodes[i].parent == -1)
        {
          if (count == index)
          {
            out_child->node = (int)i;
            return 1;
          }

          count++;
        }
      }

      return 0;
    }

    int32_t it = tml->nodes[cur.node].first_child;
    uint32_t k = 0;

    while (it >= 0)
    {
      if (k == index)
      {
        out_child->node = it;
        return 1;
      }

      k++;
      it = tml->nodes[it].next_sibling;
    }

    return 0;
  }

  X_TML_API int x_tml_find_child(XTml *tml, XTmlCursor cur,
      const char *name,
      uint32_t name_len,
      XTmlCursor *out_child)
  {
    if (!tml || !name || !out_child)
    {
      return 0;
    }

    int idx = s_tml_find_child_linear(tml, cur.node, name, name_len);

    if (idx < 0)
    {
      return 0;
    }

    out_child->node = idx;

    return 1;
  }

  /* dot-path: number => index */
  X_TML_API int x_tml_get_section(XTml *tml,
      XTmlCursor parent,
      const char *dot_path,
      XTmlCursor *out)
  {
    if (!tml || !dot_path || !out)
    {
      return 0;
    }

    uint32_t plen = (uint32_t)strlen(dot_path);
    uint32_t offv[64];
    uint32_t lenv[64];
    uint32_t n = s_tml_split_dotpath(dot_path, plen, offv, lenv, 64u);

    int cur = parent.node;

    for (uint32_t i = 0; i < n; i++)
    {
      const char *seg = dot_path + offv[i];
      uint32_t slen = lenv[i];

      int all_digits = (slen > 0u);

      for (uint32_t k = 0; k < slen && all_digits; k++)
      {
        if (seg[k] < '0' || seg[k] > '9')
        {
          all_digits = 0;
          break;
        }
      }

      if (all_digits)
      {
        uint32_t idx = 0;

        for (uint32_t k = 0; k < slen; k++)
        {
          idx = idx * 10u + (uint32_t)(seg[k] - '0');
        }

        XTmlCursor pc;
        pc.node = cur;

        XTmlCursor ch;

        if (!x_tml_child_at(tml, pc, idx, &ch))
        {
          return 0;
        }

        cur = ch.node;
      }
      else
      {
        int child = s_tml_find_child_linear(tml, cur, seg, slen);

        if (child < 0)
        {
          return 0;
        }

        cur = child;
      }
    }

    out->node = cur;

    return 1;
  }

  static int s_tml_find_kv(const XTml *tml,
      int node,
      const char *key,
      uint32_t *out_idx)
  {
    if (node < 0)
    {
      return 0;
    }

    const XTmlNode *nd = &tml->nodes[node];

    uint32_t start = nd->kv_start;
    uint32_t end   = start + nd->kv_count;

    uint32_t klen = (uint32_t)strlen(key);

    for (uint32_t i = start; i < end; i++)
    {
      const XTmlKV *kv = &tml->kvs[i];
      const char *kk = tml->text + kv->key_off;

      if (s_tml_str_equal(kk, kv->key_len, key, klen))
      {
        *out_idx = i;
        return 1;
      }
    }

    return 0;
  }

  X_TML_API int x_tml_entry_count(XTml *tml,
      XTmlCursor cur,
      uint32_t *out_count)
  {
    if (!tml || !out_count)
    {
      return 0;
    }

    if (cur.node < 0)
    {
      *out_count = 0;
      return 1;
    }

    *out_count = tml->nodes[cur.node].kv_count;

    return 1;
  }

  X_TML_API int x_tml_entry_key_at(XTml *tml,
      XTmlCursor cur,
      uint32_t index,
      const char **out_key,
      uint32_t *out_key_len)
  {
    if (!tml || !out_key || !out_key_len)
    {
      return 0;
    }

    if (cur.node < 0)
    {
      return 0;
    }

    const XTmlNode *nd = &tml->nodes[cur.node];

    if (index >= nd->kv_count)
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[nd->kv_start + index];

    *out_key = tml->text + kv->key_off;
    *out_key_len = kv->key_len;

    return 1;
  }

  X_TML_API int x_tml_get_bool(XTml *tml,
      XTmlCursor cur,
      const char *key,
      int *out_bool)
  {
    if (!tml || !key || !out_bool)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind != XTML_V_BOOL)
    {
      return 0;
    }

    *out_bool = kv->val.boolean;

    return 1;
  }

  X_TML_API int x_tml_get_i64(XTml *tml,
      XTmlCursor cur,
      const char *key,
      int64_t *out_i64)
  {
    if (!tml || !key || !out_i64)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind == XTML_V_I64)
    {
      *out_i64 = kv->val.i64;
      return 1;
    }

    if (kv->val.kind == XTML_V_F64)
    {
      *out_i64 = (int64_t)kv->val.f64;
      return 1;
    }

    return 0;
  }

  X_TML_API int x_tml_get_f64(XTml *tml,
      XTmlCursor cur,
      const char *key,
      double *out_f64)
  {
    if (!tml || !key || !out_f64)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind == XTML_V_F64)
    {
      *out_f64 = kv->val.f64;
      return 1;
    }

    if (kv->val.kind == XTML_V_I64)
    {
      *out_f64 = (double)kv->val.i64;
      return 1;
    }

    return 0;
  }

  X_TML_API int x_tml_get_str(XTml *tml,
      XTmlCursor cur,
      const char *key,
      const char **out_str,
      uint32_t *out_len)
  {
    if (!tml || !key || !out_str || !out_len)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind != XTML_V_STR)
    {
      return 0;
    }

    *out_str = tml->text + kv->val.off;
    *out_len = kv->val.len;

    return 1;
  }

  /* Arrays */

  static int s_tml_collect_array(XTml *tml,
      const XTmlKV *kv,
      const double **out_f64,
      uint32_t *out_nf64,
      const int64_t **out_i64,
      uint32_t *out_ni64,
      const XTmlStrSlice **out_str,
      uint32_t *out_ns)
  {
    if (kv->val.kind != XTML_V_ARR)
    {
      return 0;
    }

    if (kv->val.arr_count > 0)
    {
      if (kv->val.arr_is_f64 && out_f64 && out_nf64)
      {
        *out_f64 = tml->nums_f64 + kv->val.arr_start;
        *out_nf64 = kv->val.arr_count;

        if (out_i64)
        {
          *out_i64 = NULL;
        }

        if (out_ni64)
        {
          *out_ni64 = 0;
        }

        if (out_str)
        {
          *out_str = NULL;
        }

        if (out_ns)
        {
          *out_ns = 0;
        }

        return 1;
      }

      if (kv->val.arr_is_i64 && out_i64 && out_ni64)
      {
        *out_i64 = tml->nums_i64 + kv->val.arr_start;
        *out_ni64 = kv->val.arr_count;

        if (out_f64)
        {
          *out_f64 = NULL;
        }

        if (out_nf64)
        {
          *out_nf64 = 0;
        }

        if (out_str)
        {
          *out_str = NULL;
        }

        if (out_ns)
        {
          *out_ns = 0;
        }

        return 1;
      }

      if (kv->val.arr_is_str && out_str && out_ns)
      {
        *out_str = tml->str_slices + kv->val.arr_start;
        *out_ns = kv->val.arr_count;

        if (out_f64)
        {
          *out_f64 = NULL;
        }

        if (out_nf64)
        {
          *out_nf64 = 0;
        }

        if (out_i64)
        {
          *out_i64 = NULL;
        }

        if (out_ni64)
        {
          *out_ni64 = 0;
        }

        return 1;
      }
    }

    const char *vp = tml->text + kv->val.off;
    uint32_t vl = kv->val.len;

    uint32_t nf = tml->nums_f64_count;
    uint32_t ni = tml->nums_i64_count;
    uint32_t ns = tml->str_slice_count;

    uint32_t start = 0u;

    while (start <= vl)
    {
      uint32_t end = start;
      int inq = 0;

      while (end < vl)
      {
        char c = vp[end];

        if (c == '"')
        {
          inq = !inq;
        }

        if (c == ',' && !inq)
        {
          break;
        }

        end++;
      }

      const char *ep = vp + start;
      uint32_t el = end - start;

      while (el > 0 && (*ep == ' ' || *ep == '\t'))
      {
        ep++;
        el--;
      }

      while (el > 0 && (ep[el - 1] == ' ' || ep[el - 1] == '\t' || ep[el - 1] == '\r'))
      {
        el--;
      }

      if (el > 0)
      {
        if (ep[0] == '"' && el >= 2 && ep[el - 1] == '"')
        {
          tml->str_slices[tml->str_slice_count].ptr = ep + 1;
          tml->str_slices[tml->str_slice_count].len = el - 2;
          tml->str_slice_count++;
        }
        else
        {
          int is_int = 1;

          for (uint32_t q = 0; q < el; q++)
          {
            char c2 = ep[q];

            if (c2 == '.' || c2 == 'e' || c2 == 'E')
            {
              is_int = 0;
              break;
            }
          }

          if (is_int)
          {
            long long iv = strtoll(ep, NULL, 10);
            tml->nums_i64[tml->nums_i64_count] = (int64_t)iv;
            tml->nums_i64_count++;
          }
          else
          {
            char *endp = NULL;
            double dv = strtod(ep, &endp);

            if (endp == ep + (ptrdiff_t)el)
            {
              tml->nums_f64[tml->nums_f64_count] = dv;
              tml->nums_f64_count++;
            }
            else
            {
              tml->str_slices[tml->str_slice_count].ptr = ep;
              tml->str_slices[tml->str_slice_count].len = el;
              tml->str_slice_count++;
            }
          }
        }
      }

      if (end >= vl)
      {
        start = vl + 1;
      }
      else
      {
        start = end + 1;
      }
    }

    if (out_f64 && out_nf64)
    {
      *out_f64 = tml->nums_f64 + nf;
      *out_nf64 = tml->nums_f64_count - nf;
    }

    if (out_i64 && out_ni64)
    {
      *out_i64 = tml->nums_i64 + ni;
      *out_ni64 = tml->nums_i64_count - ni;
    }

    if (out_str && out_ns)
    {
      *out_str = tml->str_slices + ns;
      *out_ns = tml->str_slice_count - ns;
    }

    return 1;
  }

  X_TML_API int x_tml_get_array_len(XTml *tml,
      XTmlCursor cur,
      const char *key,
      uint32_t *out_len)
  {
    if (!tml || !key || !out_len)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind != XTML_V_ARR)
    {
      return 0;
    }

    const double *f = NULL;
    const int64_t *i64 = NULL;
    const XTmlStrSlice *ss = NULL;

    uint32_t nf = 0;
    uint32_t ni = 0;
    uint32_t ns = 0;

    if (!s_tml_collect_array(tml, kv, &f, &nf, &i64, &ni, &ss, &ns))
    {
      return 0;
    }

    *out_len = nf + ni + ns;

    return 1;
  }

  X_TML_API int x_tml_get_array_f64(XTml *tml,
      XTmlCursor cur,
      const char *key,
      const double **out_vals,
      uint32_t *out_len)
  {
    if (!tml || !key || !out_vals || !out_len)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind != XTML_V_ARR)
    {
      return 0;
    }

    const double *f = NULL;
    const int64_t *i64 = NULL;
    const XTmlStrSlice *ss = NULL;

    uint32_t nf = 0;
    uint32_t ni = 0;
    uint32_t ns = 0;

    if (!s_tml_collect_array(tml, kv, &f, &nf, &i64, &ni, &ss, &ns))
    {
      return 0;
    }

    if (nf == 0)
    {
      return 0;
    }

    *out_vals = f;
    *out_len = nf;

    return 1;
  }

  X_TML_API int x_tml_get_array_i64(XTml *tml,
      XTmlCursor cur,
      const char *key,
      const int64_t **out_vals,
      uint32_t *out_len)
  {
    if (!tml || !key || !out_vals || !out_len)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind != XTML_V_ARR)
    {
      return 0;
    }

    const double *f = NULL;
    const int64_t *i64 = NULL;
    const XTmlStrSlice *ss = NULL;

    uint32_t nf = 0;
    uint32_t ni = 0;
    uint32_t ns = 0;

    if (!s_tml_collect_array(tml, kv, &f, &nf, &i64, &ni, &ss, &ns))
    {
      return 0;
    }

    if (ni == 0)
    {
      return 0;
    }

    *out_vals = i64;
    *out_len = ni;

    return 1;
  }

  X_TML_API int x_tml_get_array_str(XTml *tml,
      XTmlCursor cur,
      const char *key,
      const XTmlStrSlice **out_vals,
      uint32_t *out_len)
  {
    if (!tml || !key || !out_vals || !out_len)
    {
      return 0;
    }

    uint32_t idx = 0;

    if (!s_tml_find_kv(tml, cur.node, key, &idx))
    {
      return 0;
    }

    const XTmlKV *kv = &tml->kvs[idx];

    if (kv->val.kind != XTML_V_ARR)
    {
      return 0;
    }

    const double *f = NULL;
    const int64_t *i64 = NULL;
    const XTmlStrSlice *ss = NULL;

    uint32_t nf = 0;
    uint32_t ni = 0;
    uint32_t ns = 0;

    if (!s_tml_collect_array(tml, kv, &f, &nf, &i64, &ni, &ss, &ns))
    {
      return 0;
    }

    if (ns == 0)
    {
      return 0;
    }

    *out_vals = ss;
    *out_len = ns;

    return 1;
  }

  X_TML_API int x_tml_has_key(XTml *tml,
      XTmlCursor cur,
      const char *key)
  {
    const char *s = NULL;
    uint32_t sl = 0;

    if (x_tml_get_str(tml, cur, key, &s, &sl))
    {
      return 1;
    }

    double f = 0.0;

    if (x_tml_get_f64(tml, cur, key, &f))
    {
      return 1;
    }

    int64_t i = 0;

    if (x_tml_get_i64(tml, cur, key, &i))
    {
      return 1;
    }

    int b = 0;

    if (x_tml_get_bool(tml, cur, key, &b))
    {
      return 1;
    }

    uint32_t n = 0;

    if (x_tml_get_array_len(tml, cur, key, &n))
    {
      return 1;
    }

    return 0;
  }

#ifdef __cplusplus
}
#endif

#endif /* X_IMPL_TML */
#endif /* STDX_TML_H */
