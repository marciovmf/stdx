#include <stdx_common.h>
#define X_IMPL_ARRAY
#define X_IMPL_ARENA
#define X_IMPL_FILESYSTEM
#define X_IMPL_INI
#define X_IMPL_IO
#define X_IMPL_STRBUILDER
#define X_IMPL_STRING
#define X_IMPL_HASHTABLE
#define X_IMPL_LOG
#include <stdx_array.h>
#include <stdx_arena.h>
#include <stdx_filesystem.h>
#include <stdx_ini.h>
#include <stdx_io.h>
#include <stdx_strbuilder.h>
#include <stdx_string.h>
#include <stdx_hashtable.h>
#include <stdx_log.h>
#define MD_IMPL
#include "markdown.h"
#include "doxter.h"
#include "doxter_template.h"
#include "doxter_fonts.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define INDENTATION "  "

// --------------------------------------------------------
// Types
// --------------------------------------------------------

/**
 * Holds parsed command line information
 */
typedef struct DoxterCmdLine
{
  const char*   project_name;
  const char*   doxter_file;
  const char*   output_directory;
  const char**  input_files;
  bool          skip_static;
  bool          skip_undocumented;
  bool          skip_empty_defines;
  bool          markdown_gobal_comments;
  bool          markdown_index_page;
  u32           num_input_files;
} DoxterCmdLine;

/**
 * Valid comment tags
 */
typedef enum
{
  DOX_TAG_TEXT,
  DOX_TAG_PARAM,
  DOX_TAG_RETURN,
  DOX_TAG_UNKNOWN
} DoxterTagKind;

/**
 * A Tag is a special word prefixed with @ that can appear within a doxter
 * comment block. Valid tags are @brief, @param and @return.
 */
typedef struct
{
  DoxterTagKind kind;
  XSlice     name;
  XSlice     text;
} DoxterTag;

// --------------------------------------------------------
// Helpers 
// --------------------------------------------------------

/**
 * Identify the type of template being rendered. Rendering functions can behave
 * differently depending on the TemplateRole
 */
typedef enum
{
  DOX_TMPL_ROLE_NONE,
  DOX_TMPL_ROLE_PARAM_ITEM,
  DOX_TMPL_ROLE_PARAMS,
  DOX_TMPL_ROLE_RETURN,
  DOX_TMPL_ROLE_SYMBOL,
  DOX_TMPL_ROLE_INDEX_ITEM,
  DOX_TMPL_ROLE_FILE_ITEM,
  DOX_TMPL_ROLE_FILE_INDEX,
  DOX_TMPL_ROLE_PROJECT_FILE_ITEM,
  DOX_TMPL_ROLE_PROJECT_INDEX,
  DOX_TMPL_ROLE_STYLE_CSS
} DoxterTemplateRole;

/**
 *  Holds all necessary information for rendering
 *  a given documentation page.
 */
typedef struct
{
  DoxterTemplateRole      role;
  u32                     source_index;
  DoxterProject           *project;
  const DoxterConfig      *config;
  const DoxterSymbol      *symbol;
  const DoxterSourceInfo  *source;
  XSlice                  param_name;
  XSlice                  param_desc;
  XSlice                  params_items;
  XSlice                  return_desc;
} DoxterTemplateCtx;


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

static bool s_project_source_add(DoxterProject *proj, const char* input_file_name)
{
  if (proj->source_count >= DOXTER_MAX_MODULES)
  {
    return false;
  }

  DoxterSourceInfo *m = &proj->sources[proj->source_count];
  m->path = x_arena_strdup(proj->scratch, input_file_name);

  XSmallstr tmp;
  XSlice base_name = x_fs_path_basename(input_file_name);
  x_smallstr_from_slice(base_name, &tmp);
  m->base_name = x_arena_strdup(proj->scratch, tmp.buf);

  XSlice html_name = x_fs_path_stem(base_name.ptr);
  x_smallstr_from_slice(html_name, &tmp);
  x_smallstr_append_cstr(&tmp, ".html");
  m->output_name = x_arena_strdup(proj->scratch, tmp.buf);

  proj->source_count++;
  return true;
}

// --------------------------------------------------------
//  Comment tag parsing 
// --------------------------------------------------------

static bool s_comment_next_tag(XSlice *comment, DoxterTag *out_tag)
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

  DoxterTagKind kind = DOX_TAG_TEXT;
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
// Templating
// --------------------------------------------------------

static DoxterTemplateCtx s_template_ctx;

static void s_template_ctx_push(DoxterTemplateCtx *backup)
{
  *backup = s_template_ctx;
}

static void s_template_ctx_pop(const DoxterTemplateCtx *backup)
{
  s_template_ctx = *backup;
}

static void s_template_resolve_placeholder(const char *placeholder,
    DoxterProject *project,
    u32 source_index,
    XStrBuilder *out);

static void s_render_template(const char *tmpl,
    DoxterProject *project,
    u32 source_index,
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

    const char *close = strstr(open + 2, "}}");
    if (!close)
    {
      x_strbuilder_append(out, open);
      break;
    }

    XSlice name_slice = x_slice_init(
        (char *) (open + 2),
        (size_t) (close - (open + 2)));
    name_slice = x_slice_trim(name_slice);

    char   placeholder[64];
    size_t copy_len = name_slice.length;
    if (copy_len >= sizeof(placeholder))
    {
      copy_len = sizeof(placeholder) - 1;
    }
    memcpy(placeholder, name_slice.ptr, copy_len);
    placeholder[copy_len] = '\0';
    s_template_resolve_placeholder(placeholder, project, source_index, out);
    p = close + 2;
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
    u32 i;
    for (i = 0; i < name.length; ++i)
    {
      char ch = ptr[i];
      if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
          (ch >= '0' && ch <= '9') || ch == '_' || ch == '-')
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

static XSlice s_symbol_doc_brief(const DoxterSymbol *sym)
{
  XSlice it = sym->comment;
  DoxterTag tag;

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
  DoxterTag tag;

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
      DoxterTag tag;

      while (s_comment_next_tag(&it, &tag))
      {
        if (tag.kind == DOX_TAG_PARAM)
        {
          DoxterTemplateCtx backup;
          s_template_ctx_push(&backup);
          s_template_ctx.role = DOX_TMPL_ROLE_PARAM_ITEM;
          s_template_ctx.param_name = tag.name;
          s_template_ctx.param_desc = tag.text;
          s_template_ctx.symbol = sym;

          s_render_template(templates->param_item_html,
              s_template_ctx.project,
              s_template_ctx.source_index,
              items);

          s_template_ctx_pop(&backup);
        }
      }
    }

    if (items->length > 0)
    {
      char *items_str = x_strbuilder_to_string(items);

      DoxterTemplateCtx backup;
      s_template_ctx_push(&backup);
      s_template_ctx.role = DOX_TMPL_ROLE_PARAMS;
      s_template_ctx.params_items = x_slice_init(items_str, (size_t) items->length);
      s_template_ctx.symbol = sym;

      s_render_template(templates->params_html,
          s_template_ctx.project,
          s_template_ctx.source_index,
          out);

      s_template_ctx_pop(&backup);
    }

    x_strbuilder_destroy(items);
  }
}

// --------------------------------------------------------
// @return tag
// --------------------------------------------------------

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
    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_RETURN;
    s_template_ctx.return_desc = r;
    s_template_ctx.symbol = sym;

    s_render_template(templates->return_html,
        s_template_ctx.project,
        s_template_ctx.source_index,
        out);

    s_template_ctx_pop(&backup);
  }
}

// --------------------------------------------------------
// Symbol indexing and rendering
// --------------------------------------------------------

typedef struct
{
  u32             source_index;
  DoxterProject   *project;
} DoxIndexCtx;

/*
 * Render index page for an entire source file
 */
static void s_render_index_for_type(DoxterProject *proj,
    u32 source_index,
    DoxterType type,
    XStrBuilder *out)
{
  const DoxterSourceInfo* source = &(proj->sources[source_index]);

  const u32 first = source->first_symbol_index;
  const u32 count = first + source->num_symbols;
  for (u32 symbol_i = first; symbol_i < count; symbol_i++)
  {
    DoxterSymbol *sym = (DoxterSymbol *) x_array_get(proj->symbols, symbol_i).ptr;

    if (sym->name.length == 0) continue;
    if (sym->type != type) continue;

    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_INDEX_ITEM;
    s_template_ctx.symbol = sym;

    s_render_template(proj->templates.index_item_html,
        proj,
        source_index,
        out);

    s_template_ctx_pop(&backup);
  }
}

/*
 * File list rendering for per-page navigation
 */
static void s_render_file_list(DoxterProject *proj, XStrBuilder *out)
{
  if (proj->source_count == 0 || !proj->templates.file_item_html)
  {
    return;
  }

  for (u32 i = 0; i < proj->source_count; ++i)
  {
    const DoxterSourceInfo* mctx = &proj->sources[i];

    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_FILE_ITEM;
    s_template_ctx.source = mctx;

    s_render_template(proj->templates.file_item_html,
        proj,
        s_template_ctx.source_index,
        out);

    s_template_ctx_pop(&backup);
  }
}

// --------------------------------------------------------
// Syntax Highlight render
// --------------------------------------------------------

static void s_append_html_escaped(XStrBuilder* out, XSlice s)
{
  for (size_t i = 0; i < s.length; ++i)
  {
    char c = s.ptr[i];
    if (c == '&') { x_strbuilder_append(out, "&amp;"); continue; }
    if (c == '<') { x_strbuilder_append(out, "&lt;");  continue; }
    if (c == '>') { x_strbuilder_append(out, "&gt;");  continue; }
    if (c == '"') { x_strbuilder_append(out, "&quot;"); continue; }
    x_strbuilder_append_char(out, c);
  }
}

static bool s_is_c_keyword(XSlice s)
{
  static const char* kw[] =
  {
    "auto","break","case","char","const","continue","default","do","double","else","enum",
    "extern","float","for","goto","if","inline","int","long","register","restrict","return",
    "short","signed","sizeof","static","struct","switch","typedef","union","unsigned","void",
    "volatile","while","_Alignas","_Alignof","_Atomic","_Bool","_Complex","_Generic",
    "_Imaginary","_Noreturn","_Static_assert","_Thread_local", "bool", "__FILE__", "__LINE__",
    "int32_t", "uint32_t", "int8_t", "uint8_t", "int16_t", "uint16_t"
  };

  for (size_t i = 0; i < sizeof(kw) / sizeof(kw[0]); ++i)
  {
    if (x_slice_eq_cstr(s, kw[i]))
      return true;
  }

  return false;
}

static void s_emit_tok_span_begin(XStrBuilder* out, const char* cls)
{
  x_strbuilder_append(out, "<span class=\"tok_");
  x_strbuilder_append(out, cls);
  x_strbuilder_append(out, "\">");
}

static void s_emit_tok_span_end(XStrBuilder* out)
{
  x_strbuilder_append(out, "</span>");
}

static void s_emit_token_highlighted(DoxterProject* project, XStrBuilder* out, const DoxterToken* t)
{
  const char* cls = "unk";

  switch (t->kind)
  {
    case DOXTER_MACRO_DIRECTIVE: cls = "pp"; break;
    case DOXTER_IDENT:           cls = s_is_c_keyword(t->text) ? "kw" : "id"; break;
    case DOXTER_NUMBER:          cls = "num"; break;
    case DOXTER_STRING:          cls = "str"; break;
    case DOXTER_CHAR:            cls = "chr"; break;
    case DOXTER_PUNCT:           cls = "pun"; break;
    case DOXTER_DOX_COMMENT:     cls = "doc"; break;
    default:                     cls = "unk"; break;
  }


  s_emit_tok_span_begin(out, cls);

#if 0
  DoxterSymbol ref;
  bool is_href = false;
  if (t->kind == DOXTER_IDENT || t->kind == DOXTER_MACRO_DIRECTIVE)
  {
    //printf("checking anchor for %s\n", t->text.ptr);
    if (x_hashtable_get(project->symbol_map, &t->start, &ref))
    {
      x_strbuilder_append_format(out, "<a href=\"%s\">", ref.anchor.buf);
      printf("-- anchor for %s = %s\n", t->start, ref.anchor.buf);
      is_href = true;
    }
  }
#endif

  /* Macro directive tokens are stored as "define"/"include"/etc. Add the '#'. */
  if (t->kind == DOXTER_MACRO_DIRECTIVE)
  {
    s_append_html_escaped(out, x_slice_from_cstr("#"));
  }


  s_append_html_escaped(out, t->text);
  s_emit_tok_span_end(out);
#if 0
  if(is_href) x_strbuilder_append_cstr(out, "</a>");
#endif
}

static bool s_tok_is_word(DoxterTokenKind k)
{
  return k == DOXTER_IDENT || k == DOXTER_NUMBER || k == DOXTER_STRING || k == DOXTER_CHAR;
}

static bool s_tok_is_punct_char(const DoxterToken* t, char c)
{
  return t->kind == DOXTER_PUNCT && t->text.length == 1 && t->text.ptr[0] == c;
}

static bool s_tok_is_punct_slice(const DoxterToken* t, const char* s)
{
  size_t n = strlen(s);
  return t->kind == DOXTER_PUNCT && t->text.length == n && memcmp(t->text.ptr, s, n) == 0;
}




static bool s_join_need_space_simple(const DoxterToken* prev, const DoxterToken* cur, bool glue_star_to_ident)
{
  if (!prev || !cur)
  {
    return false;
  }

  /* Always separate after preprocessor directive (we print "#define" as one token). */
  if (prev->kind == DOXTER_MACRO_DIRECTIVE)
  {
    return true;
  }

  /* No space after openers. */
  if (prev->kind == DOXTER_PUNCT)
  {
    if (s_tok_is_punct_char(prev, '(') || s_tok_is_punct_char(prev, '[') || s_tok_is_punct_char(prev, '{'))
    {
      return false;
    }

    if (s_tok_is_punct_char(prev, '.') || s_tok_is_punct_slice(prev, "->") || s_tok_is_punct_slice(prev, "::"))
    {
      return false;
    }

    /* Never put a space after '*' or '&' when we are gluing them to identifiers (params style). */
    if (glue_star_to_ident && (s_tok_is_punct_char(prev, '*') || s_tok_is_punct_char(prev, '&')))
    {
      return false;
    }
  }

  /* No space before closers and separators. */
  if (cur->kind == DOXTER_PUNCT)
  {
    if (s_tok_is_punct_char(cur, ')') || s_tok_is_punct_char(cur, ']') || s_tok_is_punct_char(cur, '}'))
    {
      return false;
    }

    if (s_tok_is_punct_char(cur, ',') || s_tok_is_punct_char(cur, ';'))
    {
      return false;
    }
  }

  /* Pointer/ref: space before '*' / '&' when following a word, but optionally glue after. */
  if (cur->kind == DOXTER_PUNCT && (s_tok_is_punct_char(cur, '*') || s_tok_is_punct_char(cur, '&')))
  {
    if (prev->kind == DOXTER_PUNCT)
    {
      if (s_tok_is_punct_char(prev, '(') || s_tok_is_punct_char(prev, '[') || s_tok_is_punct_char(prev, '{'))
      {
        return false;
      }
    }

    if (s_tok_is_word(prev->kind)) { return true; }

    if (prev->kind == DOXTER_PUNCT && (s_tok_is_punct_char(prev, ')') || s_tok_is_punct_char(prev, ']')))
    {
      return true;
    }

    return false;
  }

  /* Space after comma. */
  if (prev->kind == DOXTER_PUNCT && s_tok_is_punct_char(prev, ','))
  {
    return true;
  }

  /* Space between two word-ish tokens. */
  if (s_tok_is_word(prev->kind) && s_tok_is_word(cur->kind))
  {
    return true;
  }

  /* No space between identifier and '(' (call / declaration name). */
  if (prev->kind == DOXTER_IDENT && cur->kind == DOXTER_PUNCT && s_tok_is_punct_char(cur, '('))
  {
    return false;
  }

  return false;
}

static void s_emit_span_tokens(
    DoxterProject* project,
    DoxterTokenSpan ts,
    bool glue_star_to_ident,
    XStrBuilder* out)
{
  const DoxterToken* prev = NULL;

  for (u32 i = 0; i < ts.count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, ts.first + i).ptr;

    if (prev && s_join_need_space_simple(prev, t, glue_star_to_ident))
    {
      x_strbuilder_append_char(out, ' ');
    }

    s_emit_token_highlighted(project, out, t);
    prev = t;
  }
}

static bool s_params_has_multiple_args(DoxterProject* project, DoxterTokenSpan params_ts)
{
  int paren_depth = 0;

  for (u32 i = 0; i < params_ts.count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, params_ts.first + i).ptr;

    if (t->kind == DOXTER_PUNCT && t->text.length == 1)
    {
      char c = t->text.ptr[0];

      if (c == '(')
      {
        paren_depth++;
      }
      else if (c == ')')
      {
        paren_depth--;
      }
      else if (c == ',' && paren_depth == 1)
      {
        return true;
      }
    }
  }

  return false;
}

static void s_emit_function_params( DoxterProject* project,
    DoxterTokenSpan params_ts, bool multiline, XStrBuilder* out)
{
  const DoxterToken* prev = NULL;
  int paren_depth = 0;

  for (u32 i = 0; i < params_ts.count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, params_ts.first + i).ptr;

    if (prev && s_join_need_space_simple(prev, t, true /* glue star to ident for params */))
    {
      x_strbuilder_append_char(out, ' ');
    }

    s_emit_token_highlighted(project, out, t);

    if (t->kind == DOXTER_PUNCT && t->text.length == 1)
    {
      char c = t->text.ptr[0];

      if (c == '(')
      {
        paren_depth++;
        if (multiline && paren_depth == 1)
        {
          x_strbuilder_append_cstr(out, "\n");
          x_strbuilder_append_cstr(out, INDENTATION );

          prev = NULL;
          continue;
        }
      }
      else if (c == ')')
      {
        paren_depth--;
      }
      else if (c == ',' && multiline && paren_depth == 1)
      {
        x_strbuilder_append_cstr(out, "\n");
        x_strbuilder_append_cstr(out, INDENTATION );
        prev = NULL;
        continue;
      }
    }

    prev = t;
  }
}

static void s_emit_decl_function(DoxterProject* project, const DoxterSymbol* sym, XStrBuilder* out)
{
  s_emit_span_tokens(project, sym->stmt.fn.return_ts, false, out);

  /* Always separate return part and name. This enforces: "char * x_func". */
  x_strbuilder_append_char(out, ' ');

  const DoxterToken* name = (const DoxterToken*)x_array_get(project->tokens, sym->stmt.fn.name_tok).ptr;
  s_emit_token_highlighted(project, out, name);

  bool multiline = s_params_has_multiple_args(project, sym->stmt.fn.params_ts);
  s_emit_function_params(project, sym->stmt.fn.params_ts, multiline, out);

  x_strbuilder_append_char(out, ';');
}

static void s_emit_decl_macro(DoxterProject* project, const DoxterSymbol* sym, XStrBuilder* out)
{
  /* Emit deterministically using parser spans. */
  if (sym->stmt.macro.name_tok != 0)
  {
    DoxterTokenSpan pre;
    pre.first = sym->first_token_index;
    pre.count = (sym->stmt.macro.name_tok - sym->first_token_index) + 1;
    s_emit_span_tokens(project, pre, true, out);

    if (sym->stmt.macro.args_ts.count > 0)
    {
      s_emit_span_tokens(project, sym->stmt.macro.args_ts, true, out);
    }

    if (sym->stmt.macro.value_ts.count > 0)
    {
      /* Keep your readability rule for function-like macros. */
      if (sym->stmt.macro.args_ts.count > 0)
      {
        x_strbuilder_append_char(out, ' ');
      }
      else
      {
        x_strbuilder_append_char(out, ' ');
      }

      s_emit_span_tokens(project, sym->stmt.macro.value_ts, true, out);
    }

    return;
  }

  /* Fallback: emit raw tokens. */
  {
    DoxterTokenSpan ts;
    ts.first = sym->first_token_index;
    ts.count = sym->num_tokens;
    s_emit_span_tokens(project, ts, true, out);
  }
}

static void s_emit_decl_record(DoxterProject* project, const DoxterSymbol* sym, XStrBuilder* out)
{
  const u32 first = sym->first_token_index;
  const u32 count = sym->num_tokens;

  const DoxterToken* prev = NULL;
  int brace_depth = 0;

  for (u32 i = 0; i < count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, first + i).ptr;

    if (prev && s_join_need_space_simple(prev, t, true))
    {
      x_strbuilder_append_char(out, ' ');
    }

    s_emit_token_highlighted(project, out, t);

    if (t->kind == DOXTER_PUNCT && t->text.length == 1)
    {
      char c = t->text.ptr[0];

      if (c == '{')
      {
        brace_depth++;
        if (brace_depth == 1)
        {
          x_strbuilder_append_cstr(out, "\n");
          x_strbuilder_append_cstr(out, INDENTATION );
          prev = NULL;
          continue;
        }
      }
      else if (c == '}')
      {
        brace_depth--;
      }
      else if (c == ';' && brace_depth == 1)
      {
        x_strbuilder_append_cstr(out, "\n");
        x_strbuilder_append_cstr(out, INDENTATION );
        prev = NULL;
        continue;
      }
      else if (c == ',' && sym->type == DOXTER_ENUM && brace_depth == 1)
      {
        x_strbuilder_append_cstr(out, "\n");
        x_strbuilder_append_cstr(out, INDENTATION );
        prev = NULL;
        continue;
      }
    }

    prev = t;
  }
}

static void s_doxter_emit_decl_highlighted(DoxterProject* project, const DoxterSymbol* sym, XStrBuilder* out)
{
  switch (sym->type)
  {
    case DOXTER_FUNCTION:
      s_emit_decl_function(project, sym, out);
      break;

    case DOXTER_MACRO:
      s_emit_decl_macro(project, sym, out);
      break;

    case DOXTER_STRUCT:
    case DOXTER_UNION:
    case DOXTER_ENUM:
      s_emit_decl_record(project, sym, out);
      break;

    case DOXTER_TYPEDEF:
    default:
      {
        DoxterTokenSpan ts;
        ts.first = sym->first_token_index;
        ts.count = sym->num_tokens;
        s_emit_span_tokens(project, ts, true, out);
      } break;
  }
}

// --------------------------------------------------------
// Project index rendering
// --------------------------------------------------------

static void s_render_project_file_list(DoxterProject *proj,
    XStrBuilder *out)
{
  if (!proj->templates.project_file_item_html || proj->source_count == 0)
    return;

  for (u32 i = 0; i < proj->source_count; ++i)
  {
    const DoxterSourceInfo* mctx = &(proj->sources[i]);

    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_PROJECT_FILE_ITEM;
    s_template_ctx.source = mctx;

    s_render_template(proj->templates.project_file_item_html,
        proj,
        s_template_ctx.source_index,
        out);

    s_template_ctx_pop(&backup);
  }
}

static inline bool s_placeholder_match(const char* a, u32 hash_a, const char* b, u32 hash_b)
{
  return ((hash_a == hash_b) && (strcmp(a, b) == 0));
}

/**
 * Finds the apropriate value for replacing the template's placeholder and
 * outputs it to the passed in StringBuilder.
 */
static void s_template_resolve_placeholder(const char *placeholder,
    DoxterProject *project,
    u32 source_index,
    XStrBuilder *out)
{
  const DoxterSourceInfo* source = NULL;
  u32 hash = x_cstr_hash(placeholder);

  if (project && project->source_count > 0 && source_index < project->source_count)
  {
    source = &project->sources[source_index];
  }

  /* Param item */
  if (s_placeholder_match(placeholder, hash, "PARAM_NAME", HASH_PARAM_NAME))
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.param_name.ptr,
        s_template_ctx.param_name.length);
    return;
  }

  if (s_placeholder_match(placeholder, hash, "PARAM_DESC", HASH_PARAM_DESC))
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.param_desc.ptr,
        s_template_ctx.param_desc.length);
    return;
  }

  /* Params block */
  if (s_placeholder_match(placeholder, hash, "PARAMS_ITEMS", HASH_PARAMS_ITEMS))
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.params_items.ptr,
        s_template_ctx.params_items.length);
    return;
  }

  /* Return block */
  if (s_placeholder_match(placeholder, hash, "RETURN_DESC", HASH_RETURN_DESC))
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.return_desc.ptr,
        s_template_ctx.return_desc.length);
    return;
  }

  /* File item (side bar / lists) */
  if (s_placeholder_match(placeholder, hash, "MODULE_NAME", HASH_MODULE_NAME))
  {
    if (s_template_ctx.source && s_template_ctx.source->base_name)
    {
      x_strbuilder_append(out, s_template_ctx.source->base_name);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "MODULE_HREF", HASH_MODULE_HREF))
  {
    if (s_template_ctx.source && s_template_ctx.source->output_name)
    {
      x_strbuilder_append(out, s_template_ctx.source->output_name);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "FILE_NAME", HASH_FILE_NAME))
  {
    if (s_template_ctx.role == DOX_TMPL_ROLE_PROJECT_INDEX)
    {
      x_strbuilder_append(out, "Symbols");
    }
    else if (source && source->base_name)
    {
      x_strbuilder_append(out, source->base_name);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "TITLE", HASH_TITLE))
  {
    if (s_template_ctx.role == DOX_TMPL_ROLE_PROJECT_INDEX)
    {
      x_strbuilder_append(out, "Symbols");
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "FILE_BRIEF", HASH_FILE_BRIEF))
  {
    if (!source)
      return;

    DoxterSymbol* file_comment =
      (DoxterSymbol*) x_array_get(project->symbols, source->first_symbol_index).ptr;

    if (!file_comment)
      return;

    XSlice b = file_comment->comment;
    if (b.length == 0)
      return;

    if (project->config.markdown_gobal_comments)
    {
      const char* markdown = md_to_html(b.ptr, b.length);
      u32 len = (u32) (markdown ? strlen(markdown) : 0);
      x_strbuilder_append_substring(out, markdown, len);
    }
    else
    {
      x_strbuilder_append_substring(out, b.ptr, b.length);
    }

    return;
  }

  if (s_placeholder_match(placeholder, hash, "INDEX_ITEMS", HASH_INDEX_ITEMS))
  {
    s_render_index_for_type(project, source_index, DOXTER_FUNCTION, out);
    s_render_index_for_type(project, source_index, DOXTER_MACRO,    out);
    s_render_index_for_type(project, source_index, DOXTER_STRUCT,   out);
    s_render_index_for_type(project, source_index, DOXTER_ENUM,     out);
    s_render_index_for_type(project, source_index, DOXTER_TYPEDEF,  out);
    return;
  }

  if (s_placeholder_match(placeholder, hash, "INDEX_FUNCTIONS", HASH_INDEX_FUNCTIONS))
  {
    s_render_index_for_type(project, source_index, DOXTER_FUNCTION, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash, "INDEX_MACROS", HASH_INDEX_MACROS))
  {
    s_render_index_for_type(project, source_index, DOXTER_MACRO, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash, "INDEX_STRUCTS", HASH_INDEX_STRUCTS))
  {
    s_render_index_for_type(project, source_index, DOXTER_STRUCT, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash, "INDEX_ENUMS", HASH_INDEX_ENUMS))
  {
    s_render_index_for_type(project, source_index, DOXTER_ENUM, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash, "INDEX_TYPEDEFS", HASH_INDEX_TYPEDEFS))
  {
    s_render_index_for_type(project, source_index, DOXTER_TYPEDEF, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash, "SYMBOLS", HASH_SYMBOLS))
  {
    if (!source || !project->templates.symbol_html)
      return;

    const u32 first = source->first_symbol_index;
    const u32 count = first + source->num_symbols;
    for (u32 symbol_i = first; symbol_i < count; symbol_i++)
    {
      DoxterSymbol *sym = (DoxterSymbol *) x_array_get(project->symbols, symbol_i).ptr;

      if (sym->name.length == 0)
        continue;

      DoxterTemplateCtx backup;
      s_template_ctx_push(&backup);
      s_template_ctx.role = DOX_TMPL_ROLE_SYMBOL;
      s_template_ctx.symbol = sym;

      s_render_template(project->templates.symbol_html,
          project,
          source_index,
          out);

      s_template_ctx_pop(&backup);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "MODULE_LIST", HASH_MODULE_LIST))
  {
    if (s_template_ctx.role == DOX_TMPL_ROLE_PROJECT_INDEX)
    {
      s_render_project_file_list(project, out);
    }
    else
    {
      s_render_file_list(project, out);
    }
    return;
  }

  /* Symbol / index item */
  if (s_placeholder_match(placeholder, hash, "ANCHOR", HASH_ANCHOR))
  {
    if (s_template_ctx.symbol)
    {
      s_append_anchor(s_template_ctx.symbol, out);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "NAME", HASH_NAME))
  {
    if (s_template_ctx.symbol)
    {
      x_strbuilder_append_substring(out,
          s_template_ctx.symbol->name.ptr,
          s_template_ctx.symbol->name.length);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "KIND", HASH_KIND))
  {
    if (s_template_ctx.symbol)
    {
      x_strbuilder_append(out, s_type_to_string(s_template_ctx.symbol->type));
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "LINE", HASH_LINE))
  {
    if (s_template_ctx.symbol)
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "%u", (unsigned) s_template_ctx.symbol->line);
      x_strbuilder_append(out, buf);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "DECL", HASH_DECL))
  {
    if (s_template_ctx.symbol)
    {
      s_doxter_emit_decl_highlighted(project, s_template_ctx.symbol, out);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "BRIEF", HASH_BRIEF))
  {
    if (s_template_ctx.symbol)
    {
      XSlice b = x_slice_trim(s_symbol_doc_brief(s_template_ctx.symbol));
      x_strbuilder_append_substring(out, b.ptr, b.length);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "PARAMS_BLOCK", HASH_PARAMS_BLOCK))
  {
    if (s_template_ctx.symbol)
    {
      s_render_params_block(s_template_ctx.symbol, &project->templates, out);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash, "RETURN_BLOCK", HASH_RETURN_BLOCK))
  {
    if (s_template_ctx.symbol)
    {
      s_render_return_block(s_template_ctx.symbol, &project->templates, out);
    }
    return;
  }

  /* CSS */
  if (s_template_ctx.role == DOX_TMPL_ROLE_STYLE_CSS && s_template_ctx.config)
  {
    const DoxterConfig *cfg = s_template_ctx.config;

    if (s_placeholder_match(placeholder, hash, "COLOR_PAGE_BACKGROUND", HASH_COLOR_PAGE_BACKGROUND))
    {
      x_strbuilder_append(out, cfg->color_page_background);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_SIDEBAR_BACKGROUND", HASH_COLOR_SIDEBAR_BACKGROUND))
    {
      x_strbuilder_append(out, cfg->color_sidebar_background);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_MAIN_TEXT", HASH_COLOR_MAIN_TEXT))
    {
      x_strbuilder_append(out, cfg->color_main_text);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_SECONDARY_TEXT", HASH_COLOR_SECONDARY_TEXT))
    {
      x_strbuilder_append(out, cfg->color_secondary_text);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_HIGHLIGHT", HASH_COLOR_HIGHLIGHT))
    {
      x_strbuilder_append(out, cfg->color_highlight);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_LIGHT_BORDERS", HASH_COLOR_LIGHT_BORDERS))
    {
      x_strbuilder_append(out, cfg->color_light_borders);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_CODE_BLOCKS", HASH_COLOR_CODE_BLOCKS))
    {
      x_strbuilder_append(out, cfg->color_code_blocks);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_CODE_BLOCK_BORDER", HASH_COLOR_CODE_BLOCK_BORDER))
    {
      x_strbuilder_append(out, cfg->color_code_block_border);
      return;
    }

    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_PP", HASH_COLOR_TOK_PP))
    {
      x_strbuilder_append(out, cfg->color_tok_pp);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_KW", HASH_COLOR_TOK_KW))
    {
      x_strbuilder_append(out, cfg->color_tok_kw);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_ID", HASH_COLOR_TOK_ID))
    {
      x_strbuilder_append(out, cfg->color_tok_id);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_NUM", HASH_COLOR_TOK_NUM))
    {
      x_strbuilder_append(out, cfg->color_tok_num);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_STR", HASH_COLOR_TOK_STR))
    {
      x_strbuilder_append(out, cfg->color_tok_str);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_CRT", HASH_COLOR_TOK_CRT))
    {
      x_strbuilder_append(out, cfg->color_tok_crt);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_PUN", HASH_COLOR_TOK_PUN))
    {
      x_strbuilder_append(out, cfg->color_tok_pun);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "COLOR_TOK_DOC", HASH_COLOR_TOK_DOC))
    {
      x_strbuilder_append(out, cfg->color_tok_doc);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "FONT_UI", HASH_FONT_UI))
    {
      x_strbuilder_append(out, cfg->font_ui);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "FONT_HEADING", HASH_FONT_HEADING))
    {
      x_strbuilder_append(out, cfg->font_heading);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "FONT_CODE", HASH_FONT_CODE))
    {
      x_strbuilder_append(out, cfg->font_code);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "FONT_SYMBOL", HASH_FONT_SYMBOL))
    {
      x_strbuilder_append(out, cfg->font_symbol);
      return;
    }
    if (s_placeholder_match(placeholder, hash, "BORDER_RADIUS", HASH_BORDER_RADIUS))
    {
      x_strbuilder_append_format(out, "%d", cfg->border_radius);
      return;
    }

    printf("Unknown key for css file: '%s'\n", placeholder);
    return;
  }

  doxter_error("Unknown template placeholder: %s\n", placeholder);
}

/**
 * Loads the doxter.ini (or whatever name passed by the user)
 */
static bool s_load_doxter_file(const char* path, DoxterConfig* cfg)
{
  XIni ini;
  XIniError iniError;
  if (path == NULL) return false;

  if (!x_ini_load_file(path, &ini, &iniError))
  {
    printf("Failed to load '%s'Error %d: %s on line %d, %d. ",
        path,
        iniError.code,
        iniError.message,
        iniError.line,
        iniError.column);
    return false;
  }

  cfg->color_page_background =
    x_ini_get(&ini, "theme", "color_page_background", cfg->color_page_background);
  cfg->color_sidebar_background =
    x_ini_get(&ini, "theme", "color_sidebar_background", cfg->color_sidebar_background);
  cfg->color_main_text =
    x_ini_get(&ini, "theme", "color_main_text", cfg->color_main_text);
  cfg->color_secondary_text =
    x_ini_get(&ini, "theme", "color_secondary_text", cfg->color_secondary_text);
  cfg->color_highlight =
    x_ini_get(&ini, "theme", "color_highlight", cfg->color_highlight);
  cfg->color_light_borders =
    x_ini_get(&ini, "theme", "color_light_borders", cfg->color_light_borders);
  cfg->color_code_blocks =
    x_ini_get(&ini, "theme", "color_code_blocks", cfg->color_code_blocks);
  cfg->color_code_block_border =
    x_ini_get(&ini, "theme", "color_code_block_border", cfg->color_code_block_border);

  cfg->color_tok_pp  = x_ini_get(&ini, "theme", "color_tok_pp", cfg->color_tok_pp);
  cfg->color_tok_kw  = x_ini_get(&ini, "theme", "color_tok_kw", cfg->color_tok_kw);
  cfg->color_tok_id  = x_ini_get(&ini, "theme", "color_tok_id", cfg->color_tok_id);
  cfg->color_tok_num = x_ini_get(&ini, "theme", "color_tok_num", cfg->color_tok_num);
  cfg->color_tok_str = x_ini_get(&ini, "theme", "color_tok_str", cfg->color_tok_str);
  cfg->color_tok_crt = x_ini_get(&ini, "theme", "color_tok_crt", cfg->color_tok_crt);
  cfg->color_tok_pun = x_ini_get(&ini, "theme", "color_tok_pun", cfg->color_tok_pun);
  cfg->color_tok_doc = x_ini_get(&ini, "theme", "color_tok_doc", cfg->color_tok_doc);

  cfg->font_ui       = x_ini_get(&ini, "theme", "font_ui", cfg->font_ui);
  cfg->font_code     = x_ini_get(&ini, "theme", "font_code", cfg->font_code);
  cfg->font_heading  = x_ini_get(&ini, "theme", "font_heading", cfg->font_heading);
  cfg->font_symbol   = x_ini_get(&ini, "theme", "font_symbol", cfg->font_symbol);

  cfg->project_name  = x_ini_get(&ini, "project", "name", cfg->project_name);
  cfg->project_url   = x_ini_get(&ini, "project", "url", cfg->project_url);
  cfg->border_radius = x_ini_get_i32(&ini, "theme", "border_radius", cfg->border_radius);
  cfg->skip_static_functions =
    x_ini_get_i32(&ini, "project", "skip_static_functions", cfg->skip_static_functions);
  cfg->markdown_index_page =
    x_ini_get_i32(&ini, "project", "markdown_index_page", cfg->markdown_index_page);
  cfg->skip_empty_defines =
    x_ini_get_i32(&ini, "project", "skip_empty_defines", cfg->skip_empty_defines);
  cfg->skip_undocumented =
    x_ini_get_i32(&ini, "project", "skip_undocumented", cfg->skip_undocumented);

  return true;
}

/**
 * Parse the command line
 */
static bool s_cmdline_parse(i32 argc, char** argv, DoxterCmdLine* out)
{
  memset(out, 0, sizeof(DoxterCmdLine));
  out->output_directory = ".";

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
    else if (strcmp(a, "--md-global-comments") == 0)
    {
      out->markdown_gobal_comments = 1;
      i++;
    }
    else if (strcmp(a, "--md-index") == 0)
    {
      out->markdown_index_page = 1;
      i++;
    }
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

/**
 * Creates a DoxterProject instance
 */
static DoxterProject* s_doxter_project_create(DoxterCmdLine* args)
{
  const size_t arena_size = 1024 * 1024 * 4; // big enough arena size.

  XArena* arena = x_arena_create(arena_size);
  DoxterProject* proj = (DoxterProject*) malloc (sizeof(DoxterProject));
  proj->scratch                                 = arena;
  proj->symbol_map                              = x_hashtable_create(char*, DoxterSymbol);
  proj->symbols                                 = x_array_create(sizeof(DoxterSymbol), 256);
  proj->tokens                                  = x_array_create(sizeof(DoxterToken), 64);
  proj->source_count                            = 0;

  // Default config values
  proj->config.color_page_background            = "ffffff";
  proj->config.color_sidebar_background         = "f8f9fb";
  proj->config.color_main_text                  = "1b1b1b";
  proj->config.color_secondary_text             = "5f6368";
  proj->config.color_highlight                  = "0067b8";
  proj->config.color_light_borders              = "e1e4e8";
  proj->config.color_code_blocks                = "f6f8fa";
  proj->config.color_code_block_border          = "d0d7de";

  proj->config.color_tok_pp                     = "6f42c1";
  proj->config.color_tok_kw                     = "005cc5";
  proj->config.color_tok_id                     = "24292f";
  proj->config.color_tok_num                    = "b31d28";
  proj->config.color_tok_str                    = "032f62";
  proj->config.color_tok_crt                    = "032f62";
  proj->config.color_tok_pun                    = "6a737d";
  proj->config.color_tok_doc                    = "22863a";

  proj->config.font_code                        = "JetBrains Mono";
  proj->config.font_ui                          = "JetBrains Mono";
  proj->config.font_symbol                      = "JetBrains Mono";
  proj->config.font_heading                     = "JetBrains Mono";
  proj->config.border_radius                    = 10;
  proj->config.project_name                     = "Doxter";
  proj->config.project_url                      = "http://github.com/marciovmf/doxter";
  proj->config.skip_static_functions     = 1;
  proj->config.markdown_index_page       = 1;
  proj->config.markdown_gobal_comments   = 1;
  proj->config.markdown_index_page       = 1;
  proj->config.skip_undocumented         = 1;

  // If provided, load the doxter file
  if (args->doxter_file && x_fs_is_file(args->doxter_file))
    s_load_doxter_file(args->doxter_file, &proj->config);

  // Load templates
  proj->templates.project_index_html   = (char*) PROJECT_INDEX_HTML;
  proj->templates.file_index_html      = (char*) MODULE_INDEX_HTML;
  proj->templates.symbol_html          = (char*) SYMBOL_HTML;
  proj->templates.index_item_html      = (char*) INDEX_ITEM_HTML;
  proj->templates.style_css            = (char*) STYLE_CSS ;
  proj->templates.project_file_item_html = (char*) PROJECT_MODULE_ITEM_HTML ;
  proj->templates.params_html          = (char*) PARAMS_HTML;
  proj->templates.param_item_html      = (char*) PARAM_ITEM_HTML;
  proj->templates.return_html          = (char*) RETURN_HTML;
  proj->templates.file_item_html       = (char*) MODULE_ITEM_HTML;
  memset(&proj->sources, 0, sizeof(proj->sources));

  // Add source files to project
  for (u32 i = 0; i < args->num_input_files; i++)
  {
    const char* f = args->input_files[i];
    if (! s_project_source_add(proj, f))
    {
      fprintf(stderr, "Warning: could not record file '%s'\n", f);
    }
  }
  return proj;
}

/**
 * Destroys a DoxterProject instance
 */
static void s_doxter_project_destroy(DoxterProject* proj)
{
  x_arena_destroy(proj->scratch);
  x_array_destroy(proj->tokens);
  x_hashtable_destroy(proj->symbol_map);
  free(proj);
}

// --------------------------------------------------------
// Main
// --------------------------------------------------------
i32 main(i32 argc, char **argv)
{
  DoxterProject* proj;
  DoxterCmdLine args;
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
    doxter_error("Failed to create directory '%s'\n", args.output_directory);
    return 1;
  }

  if (! x_fs_is_directory(args.output_directory))
  {
    doxter_error("Path exists and is not a directory '%s'\n", args.output_directory);
    return 1;
  }

  proj = s_doxter_project_create(&args);

  // --------------------------------------------------------
  // Parse all files, collect symbols and comments
  // --------------------------------------------------------
  for (u32 arg_i = 0; arg_i < proj->source_count; arg_i++)
  {
    const char* path = proj->sources[arg_i].path;
    doxter_info("Parsing %s\n", path);
    i32 count = doxter_source_parse(proj, arg_i);
    if (count < 0)
      doxter_error("Error parsing %s. Failed to open file.", path);
  }

  // --------------------------------------------------------
  // Save each symbol from each source into the stdx_hashtable
  // for cross links later.
  // --------------------------------------------------------
  for (u32 source_i = 0; source_i < proj->source_count; source_i++)
  {
    DoxterSourceInfo* source_info = &(proj->sources[source_i]);
    u32 start = source_info->first_symbol_index;
    u32 end   = start + source_info->num_symbols;

    for (u32 i_symbol = start; i_symbol < end; i_symbol++)
    {
      DoxterSymbol* sym = x_array_get(proj->symbols, i_symbol).ptr;
      x_smallstr_clear(&sym->anchor);
      if (sym->type == DOXTER_FILE)
      {
        x_smallstr_appendf(&sym->anchor, "%s%c", source_info->base_name, 0);
        x_hashtable_set(proj->symbol_map, &sym->name.ptr, sym);
      }
      else
      {
        x_smallstr_appendf(&sym->anchor, "%s/#%.*s%c",
            source_info->base_name, (u32) sym->name.length, sym->name.ptr, 0);
        x_hashtable_set(proj->symbol_map, &sym->name.ptr, sym);
      }
    }
  }

#if 0
  for (u32 source_i = 0; source_i < proj->source_count; source_i++)
  {
    DoxterSourceInfo* source_info = &(proj->sources[source_i]);
    printf("-- SOURCE '%s'\n", source_info->path);

    u32 start = source_info->first_symbol_index;
    u32 end   = start + source_info->num_symbols;

    for (u32 i_symbol = start; i_symbol < end; i_symbol++)
    {
      DoxterSymbol* sym = x_array_get(proj->symbols, i_symbol).ptr;
      //printf("-- '%s'\n", sym->name.ptr);

      DoxterSymbol out_sym;
      if (sym->name.length == 0)
        continue;

      if (x_hashtable_get(proj->symbol_map, &sym->name.ptr, &out_sym))
      {
        printf("-- '%s' => %s\n", sym->name.ptr, out_sym.name.ptr);
      }
    }
  }
#endif


  // --------------------------------------------------------
  // process each header and generate corresponding HTML page
  // --------------------------------------------------------

  sb = x_strbuilder_create();
  if (!sb)
  {
    fprintf(stderr, "Failed to create string builder\n");
    return 1;
  }

  for (u32 source_i = 0; source_i < proj->source_count; source_i++)
  {
    const DoxterSourceInfo* source =
      (const DoxterSourceInfo*) &proj->sources[source_i];

    x_strbuilder_clear(sb);

    // --------------------------------------------------------
    // Render templates per project file
    // --------------------------------------------------------

    DoxIndexCtx ctx;
    ctx.project      = proj;
    ctx.source_index = source_i;

    s_render_template(proj->templates.file_index_html,
        proj, source_i, sb);

    char *html = x_strbuilder_to_string(sb);
    x_fs_path(&full_path, args.output_directory, source->output_name);
    if (!x_io_write_text(full_path.buf, html))
    {
      fprintf(stderr, "Failed to write output file '%s'\n", full_path.buf);
      had_error = true;
    }
  }

  // --------------------------------------------------------
  // Generate main project index.html
  // --------------------------------------------------------
  x_strbuilder_clear(sb);
  {
    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_PROJECT_INDEX;
    s_render_template(proj->templates.project_index_html,
        proj,
        0,
        sb);
    s_template_ctx_pop(&backup);
  }

  char *html = x_strbuilder_to_string(sb);
  x_fs_path(&full_path, args.output_directory, "index.html");
  if (!x_io_write_text(full_path.buf, html))
  {
    fprintf(stderr,
        "Failed to write project index file '%s'\\n", full_path.buf);
    had_error = true;
  }

  // --------------------------------------------------------
  // Generate CSS file
  // --------------------------------------------------------

  x_strbuilder_clear(sb);
  DoxterTemplateCtx backup;
  s_template_ctx_push(&backup);
  s_template_ctx.role = DOX_TMPL_ROLE_STYLE_CSS;
  s_template_ctx.config = &proj->config;

  s_render_template(proj->templates.style_css,
      proj, 0, sb); s_template_ctx_pop(&backup);

  char *css = x_strbuilder_to_string(sb);
  x_fs_path(&full_path, args.output_directory, "style.css");
  if (!x_io_write_text(full_path.buf, css))
  {
    fprintf(stderr, "Failed to write project css file '%s'\\n", full_path.buf);
    had_error = true;
  }

  // --------------------------------------------------------
  // Generate Fonts
  // --------------------------------------------------------
  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Bold.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, JETBRAINSMONO_BOLD_WOFF2, sizeof(JETBRAINSMONO_BOLD_WOFF2));
    x_io_close(f);
  }

  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Italic.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, JETBRAINSMONO_ITALIC_WOFF2, sizeof(JETBRAINSMONO_ITALIC_WOFF2));
    x_io_close(f);
  }

  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Medium.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, JETBRAINSMONO_MEDIUM_WOFF2, sizeof(JETBRAINSMONO_MEDIUM_WOFF2));
    x_io_close(f);
  }

  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Regular.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, JETBRAINSMONO_REGULAR_WOFF2, sizeof(JETBRAINSMONO_REGULAR_WOFF2));
    x_io_close(f);
  }

  x_strbuilder_destroy(sb);
  s_doxter_project_destroy(proj);

  return had_error ? 1 : 0;
}

