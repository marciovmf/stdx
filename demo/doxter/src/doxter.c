#include <stdx_common.h>
#define X_IMPL_ARRAY
#define X_IMPL_FILESYSTEM
#define X_IMPL_INI
#define X_IMPL_IO
#define X_IMPL_STRBUILDER
#define X_IMPL_STRING
#include <stdx_array.h>
#include <stdx_filesystem.h>
#include <stdx_ini.h>
#include <stdx_io.h>
#include <stdx_strbuilder.h>
#include <stdx_string.h>

#define MD_IMPL
#include "markdown.h"
#include "doxter.h"
#include "doxter_template.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef DEFAULT_CONFIG_FILE_NAME
#define DEFAULT_CONFIG_FILE_NAME "doxter.ini"
#endif

#ifndef DOXTER_MAX_MODULES
#define DOXTER_MAX_MODULES 64
#endif

// --------------------------------------------------------
// Types
// --------------------------------------------------------

typedef struct
{
  XSlice   text;
  uint32_t line_start;
  uint32_t line_end;
  const char *end_ptr;
  bool     used;
} DoxterDocblock;

typedef enum
{
  DOX_TAG_TEXT,
  DOX_TAG_PARAM,
  DOX_TAG_RETURN,
  DOX_TAG_UNKNOWN
} DoxTagKind;

typedef struct
{
  DoxTagKind kind;
  XSlice     name;
  XSlice     text;
} DoxTag;

typedef struct
{
  char *project_index_html;
  char *project_module_item_html;

  char *module_index_html;
  char *symbol_html;
  char *index_item_html;
  char *params_html;
  char *param_item_html;
  char *return_html;
  char *module_item_html;
  char *style_css;
} DoxterTemplates;

typedef struct
{
  char *module_name;  /* e.g., "stdx_strbuilder.h"     */
  char *output_name;  /* e.g., "stdx_strbuilder.html"  */
} DoxterModule;

typedef struct
{
  DoxterModule modules[DOXTER_MAX_MODULES];
  uint32_t     module_count;
} DoxterModuleList;

typedef struct
{
  XSlice name;
  XSlice desc;
} DoxParamItemCtx;

typedef struct
{
  XSlice items;
} DoxParamsCtx;

// --------------------------------------------------------
// Helpers 
// --------------------------------------------------------

typedef void (*DoxTemplateResolver)(const char *placeholder, void *user_data, XStrBuilder *out);

static const char *s_type_to_string(DoxterType type)
{
  switch (type)
  {
    case DOXTER_FUNCTION: return "Function";
    case DOXTER_MACRO:    return "Macro";
    case DOXTER_STRUCT:   return "Struct";
    case DOXTER_ENUM:     return "Enum";
    case DOXTER_TYPEDEF:  return "Typedef";
    default:              return "Unknown";
  }
}

static const char *s_tag_type_to_string(DoxTagKind type)
{
  switch (type)
  {
    case DOX_TAG_TEXT:    return "@brief";
    case DOX_TAG_PARAM:   return "@param";
    case DOX_TAG_RETURN:  return "@return";
    case DOX_TAG_UNKNOWN: 
    default:              return "Unknown";
  }
}

static char *s_strdup(const char *s)
{
  size_t len;
  char  *p;

  if (!s)
  {
    return NULL;
  }

  len = strlen(s);
  p = (char *) malloc(len + 1);
  if (!p)
  {
    return NULL;
  }

  memcpy(p, s, len + 1);
  p[len] = '\0';
  return p;
}

static void s_modules_init(DoxterModuleList *list)
{
  memset(list, 0, sizeof(*list));
}

static void s_modules_free(DoxterModuleList *list)
{
  uint32_t i;

  if (!list)
  {
    return;
  }

  for (i = 0; i < list->module_count; ++i)
  {
    free(list->modules[i].module_name);
    free(list->modules[i].output_name);
  }

  memset(list, 0, sizeof(*list));
}

/*
 * Derive output html name from an input path. E.g. "foo/bar/stdx_strbuilder.h" -> "stdx_strbuilder.html"
 */
static void _s_get_names(const char *path,
    char *base_name,
    size_t base_cap,
    char *out_html,
    size_t html_cap)
{
  const char *base = path;
  const char *slash = strrchr(path, '/');
#ifdef _WIN32
  const char *bslash = strrchr(path, '\\');
  if (!slash || (bslash && bslash > slash))
  {
    slash = bslash;
  }
#endif

  if (slash)
  {
    base = slash + 1;
  }

  {
    size_t blen = strlen(base);
    if (blen >= base_cap)
    {
      blen = base_cap - 1;
    }

    memcpy(base_name, base, blen);
    base_name[blen] = '\0';
  }

  /* strip extension */
  {
    char *dot = strrchr(base_name, '.');
    if (dot)
    {
      *dot = '\0';
    }
  }

  {
    const char *ext = ".html";
    size_t ext_len  = strlen(ext);
    size_t cur_len  = strlen(base_name);

    if (cur_len + ext_len + 1 < html_cap)
    {
      memcpy(out_html, base_name, cur_len);
      memcpy(out_html + cur_len, ext, ext_len + 1);
    }
    else
    {
      if (html_cap > 0)
      {
        out_html[0] = '\0';
      }
    }
  }
}

static bool s_modules_add(DoxterModuleList *list, const char* input_file_name)
{
  if (list->module_count >= DOXTER_MAX_MODULES)
  {
    return false;
  }

  DoxterModule *m = &list->modules[list->module_count];

  XSmallstr tmp;
  XSlice base_name = x_fs_path_basename(input_file_name);

  x_smallstr_from_slice(base_name, &tmp);
  m->module_name = _strdup(tmp.buf);

  XSlice html_name = x_fs_path_stem(base_name.ptr);
  x_smallstr_from_slice(html_name, &tmp);
  x_smallstr_append_cstr(&tmp, ".html");
  m->output_name = _strdup(tmp.buf);

  printf("MODULE: %s\nbasename = %s\nstem = %s\n\n",
      input_file_name,
      m->module_name,
      m->output_name);

  if (!m->module_name || !m->output_name)
  {
    free(m->module_name);
    free(m->output_name);
    m->module_name = m->output_name = NULL;
    return false;
  }

  list->module_count++;
  return true;
}

// --------------------------------------------------------
// Identifier extraction
// --------------------------------------------------------

static XSlice s_extract_identifier(XSlice slice)
{
  const char *start = slice.ptr;
  const char *cur   = start + slice.length;

  while (cur > start)
  {
    char c = (char) *(cur - 1);
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
        c == '*' || c == ')' || c == '(' || c == ';'     ||
        c == '{' || c == '}' || c == '[' || c == ']')
    {
      cur--;
    }
    else
    {
      break;
    }
  }

  const char *end = cur;

  while (cur > start)
  {
    char c = (char) *(cur - 1);
    if ((c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '_')
    {
      cur--;
    }
    else
    {
      break;
    }
  }

  if (end <= cur)
  {
    return x_slice_empty();
  }

  return x_slice_init((char *) cur, (size_t) (end - cur));
}

static XSlice s_extract_function_name(XSlice full_decl)
{
  const char *start = full_decl.ptr;
  const char *p     = start;
  const char *end   = start + full_decl.length;

  while (p < end && *p != '(')
  {
    p++;
  }

  {
    XSlice header = x_slice_init((char *) start, (size_t) (p - start));
    header = x_slice_trim(header);
    return s_extract_identifier(header);
  }
}

static XSlice s_extract_macro_name(XSlice define_line)
{
  XSlice s = x_slice_trim(define_line);
  XSlice tok = x_slice("#define");
  x_slice_next_token_white_space(&s, &tok);

  XSlice name = x_slice_empty();
  x_slice_next_token_white_space(&s, &name);

  if (name.length == 0)
  {
    return x_slice_empty();
  }

  const char *p   = name.ptr;
  const char *end = name.ptr + name.length;
  const char *cut = end;

  const char *it;
  for (it = p; it < end; ++it)
  {
    if (*it == '(')
    {
      cut = it;
      break;
    }
  }

  return x_slice_init((char *) p, (size_t) (cut - p));
}

// --------------------------------------------------------
//  Comment tag parsing 
// --------------------------------------------------------

static bool s_comment_next_tag(XSlice *comment, DoxTag *out_tag)
{
  if (comment->length == 0)
  {
    return false;
  }

  *comment = x_slice_trim(*comment);

  if (x_slice_starts_with_cstr(*comment, "/**"))
  {
    comment->ptr    += 3;
    comment->length -= 3;
    if (comment->length >= 2)
    {
      comment->length -= 2;
    }
  }

  *comment = x_slice_trim(*comment);

  while (x_slice_starts_with_cstr(*comment, "*"))
  {
    comment->ptr++;
    comment->length--;
  }

  *comment = x_slice_trim(*comment);

  DoxTagKind kind = DOX_TAG_TEXT;
  XSlice     name = x_slice_empty();

  if ((!x_slice_starts_with_cstr(*comment, "@")) ||
      x_slice_starts_with_cstr(*comment, "@brief ") ||
      x_slice_starts_with_cstr(*comment, "@brief\t"))
  {
    if (x_slice_starts_with_cstr(*comment, "@brief"))
    {
      XSlice tag = x_slice("@brief");
      x_slice_next_token_white_space(comment, &tag);
    }
  }
  else if (x_slice_starts_with_cstr(*comment, "@param ") ||
      x_slice_starts_with_cstr(*comment, "@param\t"))
  {
    kind = DOX_TAG_PARAM;
    XSlice tag = x_slice("@param");
    x_slice_next_token_white_space(comment, &tag);
    x_slice_next_token_white_space(comment, &name);
  }
  else if (x_slice_starts_with_cstr(*comment, "@return ") ||
      x_slice_starts_with_cstr(*comment, "@return\t"))
  {
    kind = DOX_TAG_RETURN;
    XSlice tag = x_slice("@return");
    x_slice_next_token_white_space(comment, &tag);
  }
  else
  {
    kind = DOX_TAG_UNKNOWN;
    XSlice tag = x_slice_empty();
    x_slice_next_token_white_space(comment, &tag);
  }

  *comment = x_slice_trim_left(*comment);

  {
    char *desc = (char *) comment->ptr;
    char *end  = (char *) comment->ptr + comment->length;
    char *p    = desc;

    bool start_of_line = false;
    while (p < end)
    {
      if (*p == '\n')
      {
        start_of_line = true;
      }
      else if (start_of_line &&
          (*p == '\r' || *p == '\t' || *p == ' ' || *p == '*'))
      {
        *p = ' ';
      }
      else if (start_of_line && *p == '@')
      {
        break;
      }
      else
      {
        start_of_line = false;
      }

      p++;
    }

    {
      size_t desc_size = (size_t) (p - desc);
      comment->ptr    = p;
      comment->length -= desc_size;

      out_tag->kind = kind;
      out_tag->name = name;
      out_tag->text = x_slice_init(desc, desc_size);
      out_tag->text = x_slice_trim(out_tag->text);
    }
  }

  if (kind == DOX_TAG_UNKNOWN && out_tag->text.length == 0)
  {
    return false;
  }

  return true;
}

// --------------------------------------------------------
//  Template Loading
// --------------------------------------------------------

static bool s_load_templates(DoxterTemplates *t)
{
  memset(t, 0, sizeof(*t));
  t->project_index_html   = (char*) PROJECT_INDEX_HTML;
  t->module_index_html    = (char*) MODULE_INDEX_HTML;
  t->symbol_html          = (char*) SYMBOL_HTML;
  t->index_item_html      = (char*) INDEX_ITEM_HTML;
  t->style_css            = (char*) STYLE_CSS ;
  t->project_module_item_html = (char*) PROJECT_MODULE_ITEM_HTML ;
  t->params_html          = (char*) PARAMS_HTML;
  t->param_item_html      = (char*) PARAM_ITEM_HTML;
  t->return_html          = (char*) RETURN_HTML;
  t->module_item_html     = (char*) MODULE_ITEM_HTML;

  return true;
}

// --------------------------------------------------------
// Templating
// --------------------------------------------------------

static void s_render_template(const char *tmpl,
    DoxTemplateResolver resolver,
    void *user_data,
    XStrBuilder *out)
{
  const char *p = tmpl;

  while (*p != '\0')
  {
    const char *open = strstr(p, "{{");
    if (!open)
    {
      x_strbuilder_append(out, p);
      break;
    }

    if (open > p)
    {
      x_strbuilder_append_substring(out, p, (size_t) (open - p));
    }

    {
      const char *close = strstr(open + 2, "}}");
      if (!close)
      {
        x_strbuilder_append(out, open);
        break;
      }

      XSlice name_slice = x_slice_init((char *) (open + 2),
          (size_t) (close - (open + 2)));
      name_slice = x_slice_trim(name_slice);

      {
        char   name_buf[64];
        size_t copy_len = name_slice.length;
        if (copy_len >= sizeof(name_buf))
        {
          copy_len = sizeof(name_buf) - 1;
        }
        memcpy(name_buf, name_slice.ptr, copy_len);
        name_buf[copy_len] = '\0';

        if (resolver)
        {
          resolver(name_buf, user_data, out);
        }
      }

      p = close + 2;
    }
  }
}

// --------------------------------------------------------
// Rendering Helpers
// --------------------------------------------------------

static void s_append_anchor(const DoxterSymbol *sym, XStrBuilder *out)
{
  XSlice name = sym->name;
  if (name.length == 0)
  {
    x_strbuilder_append(out, "symbol");
    return;
  }

  {
    const char *ptr = name.ptr;
    uint32_t i;
    for (i = 0; i < name.length; ++i)
    {
      char ch = ptr[i];
      if ((ch >= 'A' && ch <= 'Z') ||
          (ch >= 'a' && ch <= 'z') ||
          (ch >= '0' && ch <= '9') ||
          ch == '_' || ch == '-')
      {
        x_strbuilder_append_char(out, ch);
      }
      else
      {
        x_strbuilder_append_char(out, '-');
      }
    }
  }
}

static void s_params_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxParamsCtx *ctx = (DoxParamsCtx *) user_data;

  if (strcmp(placeholder, "PARAMS_ITEMS") == 0)
  {
    x_strbuilder_append_substring(out, ctx->items.ptr, ctx->items.length);
  }
}

static void s_param_item_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxParamItemCtx *ctx = (DoxParamItemCtx *) user_data;

  if (strcmp(placeholder, "PARAM_NAME") == 0)
  {
    x_strbuilder_append_substring(out, ctx->name.ptr, ctx->name.length);
  }
  else if (strcmp(placeholder, "PARAM_DESC") == 0)
  {
    x_strbuilder_append_substring(out, ctx->desc.ptr, ctx->desc.length);
  }
}

static XSlice s_symbol_doc_brief(const DoxterSymbol *sym)
{
  XSlice it = sym->comment;
  DoxTag tag;

  while (s_comment_next_tag(&it, &tag))
  {
    if (tag.kind == DOX_TAG_TEXT && tag.text.length > 0)
    {
      return tag.text;
    }
  }

  return x_slice_empty();
}

static XSlice s_symbol_doc_return(const DoxterSymbol *sym)
{
  XSlice it = sym->comment;
  DoxTag tag;

  while (s_comment_next_tag(&it, &tag))
  {
    if (tag.kind == DOX_TAG_RETURN)
    {
      return tag.text;
    }
  }

  return x_slice_empty();
}

static void s_render_params_block(const DoxterSymbol *sym,
    const DoxterTemplates *templates,
    XStrBuilder *out)
{
  if (!templates->params_html || !templates->param_item_html)
  {
    return;
  }

  {
    XStrBuilder *items = x_strbuilder_create();
    if (!items)
    {
      return;
    }

    {
      XSlice it = sym->comment;
      DoxTag tag;

      while (s_comment_next_tag(&it, &tag))
      {
        if (tag.kind == DOX_TAG_PARAM)
        {
          DoxParamItemCtx item_ctx;
          item_ctx.name = tag.name;
          item_ctx.desc = tag.text;

          s_render_template(templates->param_item_html,
              s_param_item_resolver,
              &item_ctx,
              items);
        }
      }
    }

    if (items->length > 0)
    {
      DoxParamsCtx ctx;
      {
        char *items_str = x_strbuilder_to_string(items);
        ctx.items = x_slice_init(items_str, (size_t) items->length);

        s_render_template(templates->params_html,
            s_params_resolver,
            &ctx,
            out);

        //free(items_str);
      }
    }
    x_strbuilder_destroy(items);
  }
}

// --------------------------------------------------------
// @return tag
// --------------------------------------------------------
typedef struct
{
  XSlice desc;
} DoxReturnCtx;

static void s_return_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxReturnCtx *ctx = (DoxReturnCtx *) user_data;

  if (strcmp(placeholder, "RETURN_DESC") == 0)
  {
    x_strbuilder_append_substring(out, ctx->desc.ptr, ctx->desc.length);
  }
}

static void s_render_return_block(const DoxterSymbol *sym,
    const DoxterTemplates *templates,
    XStrBuilder *out)
{
  if (!templates->return_html)
  {
    return;
  }

  XSlice r = s_symbol_doc_return(sym);
  if (r.length == 0)
  {
    return;
  }

  {
    DoxReturnCtx ctx;
    ctx.desc = r;

    s_render_template(templates->return_html,
        s_return_resolver,
        &ctx,
        out);
  }
}

// --------------------------------------------------------
// Symbol indexing and rendering
// --------------------------------------------------------

typedef struct
{
  const DoxterSymbol    *symbol;
  const DoxterTemplates *templates;
} DoxSymbolCtx;

static void s_symbol_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxSymbolCtx *ctx = (DoxSymbolCtx *) user_data;
  const DoxterSymbol *sym = ctx->symbol;

  if (strcmp(placeholder, "ANCHOR") == 0)
  {
    s_append_anchor(sym, out);
  }
  else if (strcmp(placeholder, "NAME") == 0)
  {
    x_strbuilder_append_substring(out, sym->name.ptr, sym->name.length);
  }
  else if (strcmp(placeholder, "KIND") == 0)
  {
    x_strbuilder_append(out, s_type_to_string(sym->type));
  }
  else if (strcmp(placeholder, "LINE") == 0)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", (unsigned) sym->line);
    x_strbuilder_append(out, buf);
  }
  else if (strcmp(placeholder, "DECL") == 0)
  {
    XSlice t = x_slice_trim(sym->declaration);
    x_strbuilder_append_substring(out, t.ptr, t.length);
  }
  else if (strcmp(placeholder, "BRIEF") == 0)
  {
    XSlice b = x_slice_trim(s_symbol_doc_brief(sym));
    x_strbuilder_append_substring(out, b.ptr, b.length);
  }
  else if (strcmp(placeholder, "PARAMS_BLOCK") == 0)
  {
    s_render_params_block(sym, ctx->templates, out);
  }
  else if (strcmp(placeholder, "RETURN_BLOCK") == 0)
  {
    s_render_return_block(sym, ctx->templates, out);
  }
  else {
    printf("Lookdig for placeholder: %s\n", placeholder);
    // Config 
  }
}

typedef struct
{
  const DoxterSymbol *symbol;
} DoxIndexItemCtx;

static void s_index_item_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxIndexItemCtx *ctx = (DoxIndexItemCtx *) user_data;
  const DoxterSymbol *sym = ctx->symbol;

  if (strcmp(placeholder, "ANCHOR") == 0)
  {
    s_append_anchor(sym, out);
  }
  else if (strcmp(placeholder, "NAME") == 0)
  {
    x_strbuilder_append_substring(out, sym->name.ptr, sym->name.length);
  }
}

typedef struct
{
  const char               *file_name;
  const char               *output_name;
  XArray                   *symbols; /* array of DoxterSymbol */
  const DoxterTemplates    *templates;
  const DoxterModuleList   *modules;
} DoxIndexCtx;

static void s_render_index_for_type(const DoxIndexCtx *ctx,
    DoxterType type,
    XStrBuilder *out)
{
  if (!ctx->templates->index_item_html)
  {
    return;
  }

  {
    u32 count = x_array_count(ctx->symbols);
    for (u32 i = 0; i < count; ++i)
    {
      DoxterSymbol *sym = (DoxterSymbol *) x_array_get(ctx->symbols, i).ptr;

      if (sym->name.length == 0)
      {
        continue;
      }

      if (sym->type != type)
      {
        continue;
      }

      {
        DoxIndexItemCtx item_ctx;
        item_ctx.symbol = sym;

        s_render_template(ctx->templates->index_item_html,
            s_index_item_resolver,
            &item_ctx,
            out);
      }
    }
  }
}

// --------------------------------------------------------
// Module list rendering for per-page navigation
// --------------------------------------------------------

typedef struct
{
  const DoxterModule *module;
} DoxModuleItemCtx;

static void s_module_item_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxModuleItemCtx *ctx = (DoxModuleItemCtx *) user_data;
  const DoxterModule *m = ctx->module;

  if (strcmp(placeholder, "MODULE_NAME") == 0)
  {
    if (m->module_name)
    {
      x_strbuilder_append(out, m->module_name);
    }
  }
  else if (strcmp(placeholder, "MODULE_HREF") == 0)
  {
    if (m->output_name)
    {
      x_strbuilder_append(out, m->output_name);
    }
  }
}

static void s_render_module_list(const DoxIndexCtx *ctx,
    XStrBuilder *out)
{
  if (!ctx->modules ||
      ctx->modules->module_count == 0 ||
      !ctx->templates->module_item_html)
  {
    return;
  }

  {
    uint32_t i;
    for (i = 0; i < ctx->modules->module_count; ++i)
    {
      DoxModuleItemCtx mctx;
      mctx.module = &ctx->modules->modules[i];

      s_render_template(ctx->templates->module_item_html,
          s_module_item_resolver,
          &mctx,
          out);
    }
  }
}

static void s_index_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxIndexCtx *ctx = (DoxIndexCtx *) user_data;

  if (strcmp(placeholder, "FILE_NAME") == 0)
  {
    x_strbuilder_append(out, ctx->file_name);
  }
  else if (strcmp(placeholder, "FILE_BRIEF") == 0)
  {
    DoxterSymbol* file_comment = (DoxterSymbol*) x_array_get(ctx->symbols, 0).ptr;
    if (!file_comment)
      return;
    XSlice b = file_comment->comment;

    const char* markdown = md_to_html(b.ptr, b.length);
    u32 len = (u32) (markdown ? strlen(markdown) : 0);

    if (b.length > 0)
    {
      //x_strbuilder_append_substring(out, b.ptr, b.length);
      x_strbuilder_append_substring(out, markdown, len);
    }
    else
    {
      x_strbuilder_append(out, "Generated C API reference. Symbols documented with /** ... */ blocks.");
    }
  }
  else if (strcmp(placeholder, "INDEX_ITEMS") == 0)
  {
    s_render_index_for_type(ctx, DOXTER_FUNCTION, out);
    s_render_index_for_type(ctx, DOXTER_MACRO,    out);
    s_render_index_for_type(ctx, DOXTER_STRUCT,   out);
    s_render_index_for_type(ctx, DOXTER_ENUM,     out);
    s_render_index_for_type(ctx, DOXTER_TYPEDEF,  out);
  }
  else if (strcmp(placeholder, "INDEX_FUNCTIONS") == 0)
  {
    s_render_index_for_type(ctx, DOXTER_FUNCTION, out);
  }
  else if (strcmp(placeholder, "INDEX_MACROS") == 0)
  {
    s_render_index_for_type(ctx, DOXTER_MACRO, out);
  }
  else if (strcmp(placeholder, "INDEX_STRUCTS") == 0)
  {
    s_render_index_for_type(ctx, DOXTER_STRUCT, out);
  }
  else if (strcmp(placeholder, "INDEX_ENUMS") == 0)
  {
    s_render_index_for_type(ctx, DOXTER_ENUM, out);
  }
  else if (strcmp(placeholder, "INDEX_TYPEDEFS") == 0)
  {
    s_render_index_for_type(ctx, DOXTER_TYPEDEF, out);
  }
  else if (strcmp(placeholder, "SYMBOLS") == 0)
  {
    if (!ctx->templates->symbol_html)
    {
      return;
    }

    {
      u32 count = x_array_count(ctx->symbols);
      for (u32 i = 0; i < count; ++i)
      {
        DoxterSymbol *sym = (DoxterSymbol *) x_array_get(ctx->symbols, i).ptr;

        if (sym->name.length == 0)
        {
          continue;
        }

        {
          DoxSymbolCtx sym_ctx;
          sym_ctx.symbol    = sym;
          sym_ctx.templates = ctx->templates;

          s_render_template(ctx->templates->symbol_html,
              s_symbol_resolver,
              &sym_ctx,
              out);
        }
      }
    }
  }
  else if (strcmp(placeholder, "MODULE_LIST") == 0)
  {
    s_render_module_list(ctx, out);
  }
}

// --------------------------------------------------------
// Project index rendering
// --------------------------------------------------------

typedef struct
{
  const DoxterModuleList  *modules;
  const DoxterTemplates   *templates;
} DoxProjectIndexCtx;

typedef struct
{
  const DoxterModule *module;
} DoxProjectModuleItemCtx;

static void s_project_module_item_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxProjectModuleItemCtx *ctx = (DoxProjectModuleItemCtx *) user_data;
  const DoxterModule *m = ctx->module;

  if (strcmp(placeholder, "MODULE_NAME") == 0)
  {
    if (m->module_name)
    {
      x_strbuilder_append(out, m->module_name);
    }
  }
  else if (strcmp(placeholder, "MODULE_HREF") == 0)
  {
    if (m->output_name)
    {
      x_strbuilder_append(out, m->output_name);
    }
  }
}

static void s_render_project_module_list(const DoxProjectIndexCtx *ctx,
    XStrBuilder *out)
{
  uint32_t i;

  if (!ctx->templates->project_module_item_html ||
      !ctx->modules ||
      ctx->modules->module_count == 0)
  {
    return;
  }

  for (i = 0; i < ctx->modules->module_count; ++i)
  {
    DoxProjectModuleItemCtx mctx;
    mctx.module = &ctx->modules->modules[i];

    s_render_template(ctx->templates->project_module_item_html,
        s_project_module_item_resolver,
        &mctx,
        out);
  }
}

static void s_project_index_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxProjectIndexCtx *ctx = (DoxProjectIndexCtx *) user_data;

  if (strcmp(placeholder, "MODULE_LIST") == 0)
  {
    s_render_project_module_list(ctx, out);
  }
  else if (strcmp(placeholder, "TITLE") == 0 ||
      strcmp(placeholder, "FILE_NAME") == 0)
  {
    /* Simple default title; template can ignore if not needed */
    x_strbuilder_append(out, "API Index");
  }
}

static void s_project_css_resolver(const char *placeholder,
    void *user_data,
    XStrBuilder *out)
{
  DoxterConfig *cfg = (DoxterConfig *) user_data;

  if (strcmp(placeholder, "COLOR_PAGE_BACKGROUND") == 0)
  {
    x_strbuilder_append(out,  cfg->color_page_background);
  }
  else if (strcmp(placeholder, "COLOR_SIDEBAR_BACKGROUND") == 0)
  {
    x_strbuilder_append(out,  cfg->color_sidebar_background);
  }
  else if (strcmp(placeholder, "COLOR_MAIN_TEXT") == 0)
  {
    x_strbuilder_append(out,  cfg->color_main_text);
  }
  else if (strcmp(placeholder, "COLOR_SECONDARY_TEXT") == 0)
  {
    x_strbuilder_append(out,  cfg->color_secondary_text);
  }
  else if (strcmp(placeholder, "COLOR_HIGHLIGHT") == 0)
  {
    x_strbuilder_append(out,  cfg->color_highlight);
  }
  else if (strcmp(placeholder, "COLOR_LIGHT_BORDERS") == 0)
  {
    x_strbuilder_append(out,  cfg->color_light_borders);
  }
  else if (strcmp(placeholder, "COLOR_CODE_BLOCKS") == 0)
  {
    x_strbuilder_append(out,  cfg->color_code_blocks);
  }
  else if (strcmp(placeholder, "COLOR_CODE_BLOCK_BORDER") == 0)
  {
    x_strbuilder_append(out,  cfg->color_code_block_border);
  }
  else if (strcmp(placeholder, "MONO_FONTS") == 0)
  {
    x_strbuilder_append(out,  cfg->mono_fonts);
  }
  else if (strcmp(placeholder, "SERIF_FONTS") == 0)
  {
    x_strbuilder_append(out,  cfg->serif_fonts);
  }
  else if (strcmp(placeholder, "BORDER_RADIUS") == 0)
  {
    x_strbuilder_append_format(out, "%d", cfg->border_radius);
  }
  else 
  {
    printf("Unknown key for css file: '%s'\n", placeholder);
  }
}

static void s_doxter_config_init_default(DoxterConfig* cfg)
{
  cfg->color_page_background           = "ffffff";
  cfg->color_sidebar_background        = "f8f9fb";
  cfg->color_main_text                 = "1b1b1b";
  cfg->color_secondary_text            = "5f6368";
  cfg->color_highlight                 = "0067b8";
  cfg->color_light_borders             = "e1e4e8";
  cfg->color_code_blocks               = "f6f8fa";
  cfg->color_code_block_border         = "d0d7de";
  cfg->mono_fonts                      = "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, \"Liberation Mono\", \"Courier New\", monospace";
  cfg->serif_fonts                     = "\"serif_fonts\", \"Segoe UI\", system-ui, -apple-system, BlinkMacSystemFont, sans-serif";
  cfg->border_radius                   = 10;
  cfg->project_name                    = "Doxter";
  cfg->project_url                     = "http://github.com/marciovmf/doxter";
  cfg->option_skip_static_functions    = 0;
  cfg->option_markdown_index_page      = 1;
  cfg->option_markdown_gobal_comments  = 1;
  cfg->option_markdown_index_page      = 1;
  cfg->option_skip_undocumented_symbols = 0;
}

static bool s_load_doxter_file(const char* path, DoxterConfig* out)
{
  DoxterConfig cfg;
  XIni ini;
  XIniError iniError;
  if (path == NULL)
    return false;

  if (!x_ini_load_file(path, &ini, &iniError))
  {
    printf("Failed to load '%s'Error %d: %s on line %d, %d. ",
        DEFAULT_CONFIG_FILE_NAME,
        iniError.code,
        iniError.message,
        iniError.line,
        iniError.column);
    return false;
  }

  cfg.color_page_background     = x_ini_get(&ini, "theme", "color_page_background",
      "ffffff");
  cfg.color_sidebar_background  = x_ini_get(&ini, "theme", "color_sidebar_background",
      "f8f9fb");
  cfg.color_main_text           = x_ini_get(&ini, "theme", "color_main_text",
      "1b1b1b");
  cfg.color_secondary_text      = x_ini_get(&ini, "theme", "color_secondary_text",
      "5f6368");
  cfg.color_highlight           = x_ini_get(&ini, "theme", "color_highlight",
      "0067b8");
  cfg.color_light_borders       = x_ini_get(&ini, "theme", "color_light_borders",
      "e1e4e8");
  cfg.color_code_blocks         = x_ini_get(&ini, "theme", "color_code_blocks",
      "f6f8fa");
  cfg.color_code_block_border   = x_ini_get(&ini, "theme", "color_code_block_border",
      "d0d7de");
  cfg.mono_fonts                = x_ini_get(&ini, "theme", "mono_fonts",
      "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, \"Liberation Mono\", \"Courier New\", monospace");
  cfg.serif_fonts               = x_ini_get(&ini, "theme", "serif_fonts", " \"serif_fonts\", \"Segoe UI\", system-ui, -apple-system, BlinkMacSystemFont, sans-serif");

  cfg.border_radius               = x_ini_get_i32(&ini, "theme", "border_radius", 10);


  cfg.project_name  = x_ini_get(&ini, "project", "name", "Doxter");
  cfg.project_url   = x_ini_get(&ini, "project", "url", "http://github.com/marciovmf/doxter");
  cfg.h_files = x_ini_get(&ini, "project", "h_files", ".");
  cfg.c_files  = x_ini_get(&ini, "project", "c_files", ".");

  cfg.option_skip_static_functions    = x_ini_get_i32(&ini, "options", "option_skip_static_functions", 0);
  cfg.option_skip_undocumented_symbols = x_ini_get_i32(&ini, "options", "option_skip_undocumented_symbols", 0);

  cfg.option_markdown_index_page      = x_ini_get_i32(&ini, "options", "option_markdown_index_page", 1);
  cfg.option_markdown_gobal_comments  = x_ini_get_i32(&ini, "options", "option_markdown_gobal_comments", 1);
  cfg.option_markdown_index_page      = x_ini_get_i32(&ini, "options", "option_markdown_index_page", 1);

  *out = cfg;
  return true;
}

typedef struct DoxterCmdLine
{
  const char*   project_name;
  const char*   doxter_file;
  const char*   output_directory;
  const char**  input_files;
  bool          skip_static;
  bool          skip_undocumented;
  bool          skip_empty_defines;
  u32           num_input_files;
} DoxterCmdLine;

static bool s_cmdline_parse(int argc, char** argv, DoxterCmdLine* out)
{
  memset(out, 0, sizeof(DoxterCmdLine));

  out->output_directory = ".";
  // out->project_name = NULL; // optional, depends on your struct defaults

  if (argc < 2)
  {
    printf("Usage:\n\n  doxter [options] input_files\n\n");
    printf("Options\n\n");
    printf("  %-32s = %s\n", "-f <path-to-doxter.init>", "Path to doxter.ini.");
    printf("  %-32s = %s\n", "-d <path-to-output-dir>", "Specify output directory for generated files.");
    printf("  %-32s = %s\n", "-p <project-name>", "Project name.");
    printf("  %-32s = %s\n", "--skip-static", "Skip static functions.");
    printf("  %-32s = %s\n", "--skip-undocumented", "Skip undocumented symbols.");
    printf("  %-32s = %s\n", "--skip-empty-defines", "Skip empty defines.");
    return false;
  }

  // parse options (must appear before input_files)
  i32 i = 1;
  while (i < argc)
  {
    const char* a = argv[i];

    if (a[0] != '-')
      break;

    if (strcmp(a, "-f") == 0)
    {
      if (++i >= argc)
      {
        fprintf(stderr, "Invalid command line: -f requires path argument.\n");
        return false;
      }
      out->doxter_file = argv[i++];
    }
    else if (strcmp(a, "-d") == 0)
    {
      if (++i >= argc)
      {
        fprintf(stderr, "Invalid command line: -d requires path argument.\n");
        return false;
      }
      out->output_directory = argv[i++];
    }
    else if (strcmp(a, "-p") == 0)
    {
      if (++i >= argc)
      {
        fprintf(stderr, "Invalid command line: -p requires project name argument.\n");
        return false;
      }
      out->project_name = argv[i++];
    }
    else if (strcmp(a, "--skip-static") == 0)
    {
      out->skip_static = 1;
      i++;
    }
    else if (strcmp(a, "--skip-undocumented") == 0)
    {
      out->skip_undocumented = 1;
      i++;
    }
    else if (strcmp(a, "--skip-empty-defines") == 0)
    {
      out->skip_empty_defines = 1;
      i++;
    }
    //else if (strcmp(a, "--md-global-comments") == 0)
    //{
    //  out->option_markdown_gobal_comments = 1; // keep your existing field name if it's spelled this way
    //  i++;
    //}
    //else if (strcmp(a, "--md-index") == 0)
    //{
    //  out->option_markdown_index_page = 1;
    //  i++;
    //}
    else
    {
      fprintf(stderr, "Invalid command line: Unknown option '%s'.\n", a);
      return false;
    }
  }

  // collect input files
  if (i >= argc)
  {
    fprintf(stderr, "Invalid command line: No input files.\n");
    return false;
  }

  out->input_files = (const char**)&(argv[i]);
  out->num_input_files = argc - i;
  return true;
}

// --------------------------------------------------------
// Main
// --------------------------------------------------------
int main(int argc, char **argv)
{
  DoxterTemplates templates;
  DoxterModuleList modules;
  DoxterCmdLine args;
  DoxterConfig cfg;
  bool had_error = false;
  XStrBuilder *sb;
  XFSPath full_path;

  if (!s_cmdline_parse(argc, argv, &args))
    return 1;

  // --------------------------------------------------------
  // Create output directory if it does not exist 
  // --------------------------------------------------------
  XFSPath out_dir;
  x_fs_path(&out_dir, args.output_directory);
  if (! x_fs_path_exists(&out_dir) && !x_fs_directory_create_recursive(args.output_directory))
  {
    printf("Failed to create directory '%s'\n", args.output_directory);
    return 1;
  }

  if (! x_fs_is_directory(args.output_directory))
  {
    printf("Path exists and is not a directory '%s'\n", args.output_directory);
    return 1;
  }

  sb = x_strbuilder_create();
  if (!sb)
  {
    fprintf(stderr, "Failed to create string builder\n");
    return 1;
  }

  // --------------------------------------------------------
  // Initialize the configuration from the doxter.ini if possible.
  // Otherwise use default values.
  // --------------------------------------------------------
  if (args.doxter_file && x_fs_is_file(args.doxter_file))
    s_load_doxter_file(args.doxter_file, &cfg);
  else
    s_doxter_config_init_default(&cfg);

  s_load_templates(&templates);
  s_modules_init(&modules);

  // --------------------------------------------------------
  //  Collect module list and html file names for each of them
  // --------------------------------------------------------

  for (u32 argi = 0; argi < args.num_input_files; ++argi)
  {
    if (!s_modules_add(&modules, args.input_files[argi]))
    {
      fprintf(stderr, "Warning: could not record module '%s'\n", args.input_files[argi]);
      had_error = true;
    }
  }

  // --------------------------------------------------------
  // process each header and generate its HTML page
  // --------------------------------------------------------

  XArray* array = x_array_create(sizeof(DoxterSymbol), 64);
  for (u32 argi = 0; argi < args.num_input_files; ++argi)
  {
    const char *source_path = args.input_files[argi];
    size_t file_size = 0;
    char *input = x_io_read_text(source_path, &file_size);

    if (!input)
    {
      fprintf(stderr, "Failed to read '%s'\n", source_path);
      had_error = true;
      continue;
    }

    // --------------------------------------------------------
    // Collect symbols for current source
    // --------------------------------------------------------
    XSlice source = x_slice_init(input, file_size);
    x_array_clear(array);
    doxter_source_symbols_collect(source, &cfg, array);

    // --------------------------------------------------------
    // Render templares per project module
    // --------------------------------------------------------

    DoxterModule* m = &(modules.modules[argi]);

    x_strbuilder_clear(sb);
    if (!sb)
    {
      fprintf(stderr, "Failed to create string builder\n");
      free(input);
      had_error = true;
      break;
    }

    DoxIndexCtx ctx;
    ctx.file_name    = m->module_name;
    ctx.output_name  = m->output_name;
    ctx.symbols      = array;
    ctx.templates    = &templates;
    ctx.modules      = &modules;

    s_render_template(templates.module_index_html,
        s_index_resolver,
        &ctx,
        sb);

    char *html = x_strbuilder_to_string(sb);
    x_fs_path(&full_path, args.output_directory, m->output_name);
    if (!x_io_write_text(full_path.buf, html))
    {
      fprintf(stderr, "Failed to write output file '%s'\n", full_path.buf);
      had_error = true;
    }

    free(input);
  }

  // --------------------------------------------------------
  // Generate main project index.html
  // --------------------------------------------------------
  {
    x_strbuilder_clear(sb);
    if (!sb)
    {
      fprintf(stderr, "Failed to create string builder for project index\n");
      had_error = true;
    }
    else
    {
      DoxProjectIndexCtx pctx;
      pctx.modules   = &modules;
      pctx.templates = &templates;

      s_render_template(templates.project_index_html,
          s_project_index_resolver,
          &pctx,
          sb);

      {
        char *html = x_strbuilder_to_string(sb);
        x_fs_path(&full_path, args.output_directory, "index.html");
        if (!x_io_write_text(full_path.buf, html))
        {
          fprintf(stderr,
              "Failed to write project index file '%s'\\n", full_path.buf);
          had_error = true;
        }
      }

      // --------------------------------------------------------
      // Generate CSS file
      // --------------------------------------------------------

      x_strbuilder_clear(sb);
      s_render_template(templates.style_css,
          s_project_css_resolver,
          &cfg,
          sb);
      {
        char *css = x_strbuilder_to_string(sb);
        x_fs_path(&full_path, args.output_directory, "style.css");
        if (!x_io_write_text(full_path.buf, css))
        {
          fprintf(stderr, "Failed to write project css file '%s'\\n", full_path.buf);
          had_error = true;
        }
      }

    }
  }

  s_modules_free(&modules);
  x_array_destroy(array);
  x_strbuilder_destroy(sb);

  return had_error ? 1 : 0;
}
