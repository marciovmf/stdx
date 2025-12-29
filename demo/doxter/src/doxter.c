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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// --------------------------------------------------------
// Types
// --------------------------------------------------------

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

typedef enum
{
  DOX_TAG_TEXT,
  DOX_TAG_PARAM,
  DOX_TAG_RETURN,
  DOX_TAG_UNKNOWN
} DoxterTagKind;

typedef struct
{
  DoxterTagKind kind;
  XSlice     name;
  XSlice     text;
} DoxterTag;

// --------------------------------------------------------
// Helpers 
// --------------------------------------------------------

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

static DoxterTemplateCtx s_template_ctx;

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

// --------------------------------------------------------
// File list rendering for per-page navigation
// --------------------------------------------------------

static void s_render_file_list(DoxterProject *proj, XStrBuilder *out)
{
  if (proj->source_count == 0 || !proj->templates.file_item_html)
  {
    return;
  }

  uint32_t i;
  for (i = 0; i < proj->source_count; ++i)
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

  if (project && project->source_count > 0 && source_index < project->source_count)
  {
    source = &project->sources[source_index];
  }

  /* Param item */
  if (strcmp(placeholder, "PARAM_NAME") == 0)
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.param_name.ptr,
        s_template_ctx.param_name.length);
    return;
  }

  if (strcmp(placeholder, "PARAM_DESC") == 0)
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.param_desc.ptr,
        s_template_ctx.param_desc.length);
    return;
  }

  /* Params block */
  if (strcmp(placeholder, "PARAMS_ITEMS") == 0)
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.params_items.ptr,
        s_template_ctx.params_items.length);
    return;
  }

  /* Return block */
  if (strcmp(placeholder, "RETURN_DESC") == 0)
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.return_desc.ptr,
        s_template_ctx.return_desc.length);
    return;
  }

  /* File item (side bar / lists) */
  if (strcmp(placeholder, "MODULE_NAME") == 0)
  {
    if (s_template_ctx.source && s_template_ctx.source->base_name)
    {
      x_strbuilder_append(out, s_template_ctx.source->base_name);
    }
    return;
  }

  if (strcmp(placeholder, "MODULE_HREF") == 0)
  {
    if (s_template_ctx.source && s_template_ctx.source->output_name)
    {
      x_strbuilder_append(out, s_template_ctx.source->output_name);
    }
    return;
  }

  /* CSS */
  if (s_template_ctx.role == DOX_TMPL_ROLE_STYLE_CSS && s_template_ctx.config)
  {
    const DoxterConfig *cfg = s_template_ctx.config;

    if (strcmp(placeholder, "COLOR_PAGE_BACKGROUND") == 0)
    {
      x_strbuilder_append(out, cfg->color_page_background);
      return;
    }
    if (strcmp(placeholder, "COLOR_SIDEBAR_BACKGROUND") == 0)
    {
      x_strbuilder_append(out, cfg->color_sidebar_background);
      return;
    }
    if (strcmp(placeholder, "COLOR_MAIN_TEXT") == 0)
    {
      x_strbuilder_append(out, cfg->color_main_text);
      return;
    }
    if (strcmp(placeholder, "COLOR_SECONDARY_TEXT") == 0)
    {
      x_strbuilder_append(out, cfg->color_secondary_text);
      return;
    }
    if (strcmp(placeholder, "COLOR_HIGHLIGHT") == 0)
    {
      x_strbuilder_append(out, cfg->color_highlight);
      return;
    }
    if (strcmp(placeholder, "COLOR_LIGHT_BORDERS") == 0)
    {
      x_strbuilder_append(out, cfg->color_light_borders);
      return;
    }
    if (strcmp(placeholder, "COLOR_CODE_BLOCKS") == 0)
    {
      x_strbuilder_append(out, cfg->color_code_blocks);
      return;
    }
    if (strcmp(placeholder, "COLOR_CODE_BLOCK_BORDER") == 0)
    {
      x_strbuilder_append(out, cfg->color_code_block_border);
      return;
    }
    if (strcmp(placeholder, "MONO_FONTS") == 0)
    {
      x_strbuilder_append(out, cfg->mono_fonts);
      return;
    }
    if (strcmp(placeholder, "SERIF_FONTS") == 0)
    {
      x_strbuilder_append(out, cfg->serif_fonts);
      return;
    }
    if (strcmp(placeholder, "BORDER_RADIUS") == 0)
    {
      x_strbuilder_append_format(out, "%d", cfg->border_radius);
      return;
    }

    printf("Unknown key for css file: '%s'\n", placeholder);
    return;
  }

  /* File index page / project page */
  if (strcmp(placeholder, "FILE_NAME") == 0)
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

  if (strcmp(placeholder, "TITLE") == 0)
  {
    if (s_template_ctx.role == DOX_TMPL_ROLE_PROJECT_INDEX)
    {
      x_strbuilder_append(out, "Symbols");
    }
    return;
  }

  if (strcmp(placeholder, "FILE_BRIEF") == 0)
  {
    if (!source)
      return;

    DoxterSymbol* file_comment =
      (DoxterSymbol*) x_array_get(project->symbols, source->first_symbol_index).ptr;

    if (!file_comment)
      return;

    XSlice b = file_comment->comment;
    const char* markdown = md_to_html(b.ptr, b.length);
    u32 len = (u32) (markdown ? strlen(markdown) : 0);

    if (b.length > 0)
    {
      x_strbuilder_append_substring(out, markdown, len);
    }
    else
    {
      x_strbuilder_append(out,
          "Generated C API reference. Symbols documented with /** ... */ blocks.");
    }
    return;
  }

  if (strcmp(placeholder, "INDEX_ITEMS") == 0)
  {
    s_render_index_for_type(project, source_index, DOXTER_FUNCTION, out);
    s_render_index_for_type(project, source_index, DOXTER_MACRO,    out);
    s_render_index_for_type(project, source_index, DOXTER_STRUCT,   out);
    s_render_index_for_type(project, source_index, DOXTER_ENUM,     out);
    s_render_index_for_type(project, source_index, DOXTER_TYPEDEF,  out);
    return;
  }

  if (strcmp(placeholder, "INDEX_FUNCTIONS") == 0)
  {
    s_render_index_for_type(project, source_index, DOXTER_FUNCTION, out);
    return;
  }

  if (strcmp(placeholder, "INDEX_MACROS") == 0)
  {
    s_render_index_for_type(project, source_index, DOXTER_MACRO, out);
    return;
  }

  if (strcmp(placeholder, "INDEX_STRUCTS") == 0)
  {
    s_render_index_for_type(project, source_index, DOXTER_STRUCT, out);
    return;
  }

  if (strcmp(placeholder, "INDEX_ENUMS") == 0)
  {
    s_render_index_for_type(project, source_index, DOXTER_ENUM, out);
    return;
  }

  if (strcmp(placeholder, "INDEX_TYPEDEFS") == 0)
  {
    s_render_index_for_type(project, source_index, DOXTER_TYPEDEF, out);
    return;
  }

  if (strcmp(placeholder, "SYMBOLS") == 0)
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

  if (strcmp(placeholder, "MODULE_LIST") == 0)
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
  if (strcmp(placeholder, "ANCHOR") == 0)
  {
    if (s_template_ctx.symbol)
    {
      s_append_anchor(s_template_ctx.symbol, out);
    }
    return;
  }

  if (strcmp(placeholder, "NAME") == 0)
  {
    if (s_template_ctx.symbol)
    {
      x_strbuilder_append_substring(out,
          s_template_ctx.symbol->name.ptr,
          s_template_ctx.symbol->name.length);
    }
    return;
  }

  if (strcmp(placeholder, "KIND") == 0)
  {
    if (s_template_ctx.symbol)
    {
      x_strbuilder_append(out, s_type_to_string(s_template_ctx.symbol->type));
    }
    return;
  }

  if (strcmp(placeholder, "LINE") == 0)
  {
    if (s_template_ctx.symbol)
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "%u", (unsigned) s_template_ctx.symbol->line);
      x_strbuilder_append(out, buf);
    }
    return;
  }

  if (strcmp(placeholder, "DECL") == 0)
  {
    if (s_template_ctx.symbol)
    {
      XSlice t = x_slice_trim(s_template_ctx.symbol->declaration);
      x_strbuilder_append_substring(out, t.ptr, t.length);
    }
    return;
  }

  if (strcmp(placeholder, "BRIEF") == 0)
  {
    if (s_template_ctx.symbol)
    {
      XSlice b = x_slice_trim(s_symbol_doc_brief(s_template_ctx.symbol));
      x_strbuilder_append_substring(out, b.ptr, b.length);
    }
    return;
  }

  if (strcmp(placeholder, "PARAMS_BLOCK") == 0)
  {
    if (s_template_ctx.symbol)
    {
      s_render_params_block(s_template_ctx.symbol, &project->templates, out);
    }
    return;
  }

  if (strcmp(placeholder, "RETURN_BLOCK") == 0)
  {
    if (s_template_ctx.symbol)
    {
      s_render_return_block(s_template_ctx.symbol, &project->templates, out);
    }
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
  cfg->mono_fonts =
    x_ini_get(&ini, "theme", "mono_fonts", cfg->mono_fonts);
  cfg->serif_fonts =
    x_ini_get(&ini, "theme", "serif_fonts", cfg->serif_fonts);
  cfg->project_name =
    x_ini_get(&ini, "project", "name", cfg->project_name);
  cfg->project_url =
    x_ini_get(&ini, "project", "url", cfg->project_url);
  cfg->border_radius =
    x_ini_get_i32(&ini, "theme", "border_radius", cfg->border_radius);
  cfg->option_skip_static_functions =
    x_ini_get_i32(&ini, "options", "option_skip_static_functions", cfg->option_skip_static_functions);
  cfg->option_markdown_index_page =
    x_ini_get_i32(&ini, "options", "option_markdown_index_page", cfg->option_markdown_index_page);
  cfg->option_skip_empty_defines =
    x_ini_get_i32(&ini, "options", "option_skip_empty_defines", cfg->option_skip_empty_defines);
  cfg->option_skip_undocumented_symbols =
    x_ini_get_i32(&ini, "options", "option_skip_undocumented_symbols", cfg->option_skip_undocumented_symbols);

  return true;
}

/**
 * Parse the command line
 */
static bool s_cmdline_parse(int argc, char** argv, DoxterCmdLine* out)
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

/**
 * Creates a DoxterProject instance
 */
static DoxterProject* s_doxter_project_create(DoxterCmdLine* args)
{
  const size_t arena_size = 1024 * 1024 * 4; // big enough arena size.

  XArena* arena = x_arena_create(arena_size);
  DoxterProject* proj = (DoxterProject*) malloc (sizeof(DoxterProject));
  proj->scratch                                 = arena;
  proj->symbol_map                              = x_hashtable_create(XSlice, DoxterSymbol);
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
  proj->config.mono_fonts                       = "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, \"Liberation Mono\", \"Courier New\", monospace";
  proj->config.serif_fonts                      = "\"serif_fonts\", \"Segoe UI\", system-ui, -apple-system, BlinkMacSystemFont, sans-serif";
  proj->config.border_radius                    = 10;
  proj->config.project_name                     = "Doxter";
  proj->config.project_url                      = "http://github.com/marciovmf/doxter";
  proj->config.option_skip_static_functions     = 1;
  proj->config.option_markdown_index_page       = 1;
  proj->config.option_markdown_gobal_comments   = 1;
  proj->config.option_markdown_index_page       = 1;
  proj->config.option_skip_undocumented_symbols = 1;

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
int main(int argc, char **argv)
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
    for (u32 i_symbol = source_info->first_symbol_index;
        i_symbol < source_info->num_symbols;
        i_symbol++)
    {
      DoxterSymbol* sym = x_array_get(proj->symbols, i_symbol).ptr;
      x_smallstr_clear(&sym->anchor);
      if (sym->type == DOXTER_FILE)
      {
        x_smallstr_appendf(&sym->anchor, "%s%c", source_info->base_name, 0);
        x_hashtable_set(proj->symbol_map, &sym->name, sym);
      }
      else
      {
        x_smallstr_appendf(&sym->anchor, "%s/#%.*s%c",
            source_info->base_name, (u32) sym->name.length, sym->name.ptr, 0);
        x_hashtable_set(proj->symbol_map, &sym->name, sym);
      }
    }
  }

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

  x_strbuilder_destroy(sb);
  s_doxter_project_destroy(proj);

  return had_error ? 1 : 0;
}
