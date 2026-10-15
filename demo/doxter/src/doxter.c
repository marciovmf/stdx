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
#include <markdown.h>

#include "doxter.h"
#include "generated/doxter_template.h"
#include "generated/doxter_fonts.h"

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
  DOX_TAG_GROUP,
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
  DOX_TMPL_ROLE_GROUP_INDEX,
  DOX_TMPL_ROLE_ALL_SYMBOLS,
  DOX_TMPL_ROLE_UNGROUPED,
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
  const DoxterGroup       *group;
  XSlice                  param_desc;
  XSlice                  param_name;
  DoxterTokenSpan         param_decl;
  bool                    has_param_decl;
  XSlice                  params_items;
  XSlice                  return_desc;
  bool                    local_symbol_links;
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

  XFSPath tmp = {0};

  // basename
  x_fs_path(&tmp, input_file_name);
  x_fs_path_basename(&tmp, &tmp);
  m->base_name = x_arena_strdup(proj->scratch, tmp.buf);

  // extract basename and change extension to html
  x_fs_path(&tmp, input_file_name);
  x_fs_path_change_extension(&tmp, ".html");
  x_fs_path_basename(&tmp, &tmp);
  m->output_name = x_arena_strdup(proj->scratch, tmp.buf);

  proj->source_count++;
  return true;
}

static int s_symbol_name_compare(const void *a, const void *b)
{
  const DoxterSymbol *sa = *(const DoxterSymbol * const *)a;
  const DoxterSymbol *sb = *(const DoxterSymbol * const *)b;

  size_t min_len = sa->name.length < sb->name.length ? sa->name.length : sb->name.length;
  int cmp = memcmp(sa->name.ptr, sb->name.ptr, min_len);

  if (cmp != 0)
  {
    return cmp;
  }

  if (sa->name.length < sb->name.length)
  {
    return -1;
  }

  if (sa->name.length > sb->name.length)
  {
    return 1;
  }

  return 0;
}

static u32 s_collect_sorted_symbols_for_type(
    DoxterProject *proj,
    u32 source_index,
    DoxterType type,
    DoxterSymbol **out_syms,
    u32 max_syms)
{
  const DoxterSourceInfo *source = &proj->sources[source_index];
  u32 first = source->first_symbol_index;
  u32 end = first + source->num_symbols;
  u32 count = 0;

  for (u32 i = first; i < end; ++i)
  {
    DoxterSymbol *sym = x_array_get(proj->symbols, i);

    if (sym->name.length == 0)
    {
      continue;
    }

    if (sym->type != type)
    {
      continue;
    }

    if (count < max_syms)
    {
      out_syms[count++] = sym;
    }
  }

  qsort(out_syms, count, sizeof(out_syms[0]), s_symbol_name_compare);
  return count;
}

bool doxter_symbol_map_get(DoxterProject *project, XSlice text, DoxterSymbol *out_sym)
{
  XSmallstr key;
  memset(&key, 0, sizeof(key));
  x_smallstr_from_slice(text, &key);

  return x_hashtable_dox_symbol_map_get(project->symbol_map, key, out_sym);
}

static bool s_param_span_contains_name(
    DoxterProject *project,
    DoxterTokenSpan ts,
    XSlice param_name)
{
  for (u32 i = 0; i < ts.count; ++i)
  {
    const DoxterToken *t =
      (const DoxterToken *)x_array_get(project->tokens, ts.first + i);

    if (t->kind == DOXTER_IDENT && x_slice_eq(t->text, param_name))
    {
      return true;
    }
  }

  return false;
}

static bool s_function_find_param_span(
    DoxterProject *project,
    const DoxterSymbol *sym,
    XSlice param_name,
    DoxterTokenSpan *out_param_ts)
{
  if (!project || !sym || !out_param_ts)
  {
    return false;
  }

  if (sym->type != DOXTER_FUNCTION)
  {
    printf("!!! %.*s is not function\n", (int) sym->name.length, sym->name.ptr);
    return false;
  }

  for (u32 i = 0; i < sym->stmt.fn.param_count; ++i)
  {
    DoxterTokenSpan ts = sym->stmt.fn.param_ts[i];
    if (ts.count == 0)
    {
      continue;
    }

    if (s_param_span_contains_name(project, ts, param_name))
    {
      *out_param_ts = ts;
      return true;
    }
  }

  return false;
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
  else if (x_slice_starts_with_cstr(*comment, "@group ") ||
      x_slice_starts_with_cstr(*comment, "@group\t") ||
      x_slice_starts_with_cstr(*comment, "@group\n") ||
      x_slice_starts_with_cstr(*comment, "@group\r"))
  {
    kind = DOX_TAG_GROUP;
    XSlice tag = x_slice_empty();
    x_slice_next_token_white_space(comment, &tag);
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

/* Groups are project-wide and are resolved after all source files are parsed. */
static bool s_group_path_valid(XSlice path)
{
  if (path.length == 0 || path.length > 120)
  {
    return false;
  }

  bool at_segment_start = true;
  for (size_t i = 0; i < path.length; ++i)
  {
    char c = path.ptr[i];
    if (c == '.')
    {
      if (at_segment_start)
      {
        return false;
      }
      at_segment_start = true;
      continue;
    }

    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
        !(c >= '0' && c <= '9') && c != '_' && c != '-')
    {
      return false;
    }
    at_segment_start = false;
  }
  return !at_segment_start;
}

static XSlice s_symbol_doc_group(const DoxterSymbol *sym)
{
  XSlice it = sym->comment;
  XSlice found = x_slice_empty();
  DoxterTag tag;

  while (s_comment_next_tag(&it, &tag))
  {
    if (tag.kind != DOX_TAG_GROUP)
    {
      continue;
    }

    XSlice path = x_slice_trim(tag.text);
    bool has_extra_text = false;
    for (size_t i = 0; i < path.length; ++i)
    {
      if (path.ptr[i] == ' ' || path.ptr[i] == '\t' ||
          path.ptr[i] == '\r' || path.ptr[i] == '\n')
      {
        for (size_t j = i; j < path.length; ++j)
        {
          char c = path.ptr[j];
          if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
          {
            has_extra_text = true;
            break;
          }
        }
        path.length = i;
        break;
      }
    }

    if (has_extra_text || !s_group_path_valid(path))
    {
      doxter_warning("Invalid @group on %.*s\n", (int)sym->name.length, sym->name.ptr);
      continue;
    }

    if (found.length > 0)
    {
      doxter_warning("Multiple @group directives on %.*s (using first)\n",
          (int)sym->name.length, sym->name.ptr);
      continue;
    }
    found = path;
  }
  return found;
}

static bool s_group_path_equal(XSlice a, XSlice b)
{
  if (a.length != b.length)
  {
    return false;
  }
  for (size_t i = 0; i < a.length; ++i)
  {
    char ca = a.ptr[i];
    char cb = b.ptr[i];
    if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
    if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
    if (ca != cb)
    {
      return false;
    }
  }
  return true;
}

/* Zero is not a group node; valid IDs are XArray indices + 1. */
static DoxterGroup *s_group_by_id(DoxterProject *proj, u32 id)
{
  if (id == 0 || id > x_array_count(proj->groups))
  {
    return NULL;
  }
  return x_array_get(proj->groups, id - 1);
}

static u32 s_group_intern(DoxterProject *proj, XSlice path)
{
  u32 count = x_array_count(proj->groups);
  for (u32 i = 0; i < count; ++i)
  {
    DoxterGroup *g = x_array_get(proj->groups, i);
    if (s_group_path_equal(g->path, path))
    {
      return i + 1;
    }
  }

  u32 parent_id = 0;
  for (size_t i = path.length; i > 0; --i)
  {
    if (path.ptr[i - 1] == '.')
    {
      parent_id = s_group_intern(proj, x_slice_init(path.ptr, i - 1));
      break;
    }
  }

  DoxterGroup group;
  memset(&group, 0, sizeof(group));
  group.path = path;
  group.parent_id = parent_id;
  size_t name_offset = 0;
  for (size_t i = 0; i < path.length; ++i)
  {
    if (path.ptr[i] == '.')
    {
      name_offset = i + 1;
    }
  }
  group.name = x_slice_init(path.ptr + name_offset, path.length - name_offset);

  group.output_name = NULL;

  x_array_push(proj->groups, &group);
  return x_array_count(proj->groups);
}

static bool s_output_name_equal(const char *a, const char *b)
{
  while (*a && *b)
  {
    char ca = *a++;
    char cb = *b++;
    if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
    if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
    if (ca != cb)
    {
      return false;
    }
  }
  return *a == *b;
}

static void s_project_build_groups(DoxterProject *proj)
{
  for (u32 source_i = 0; source_i < proj->source_count; ++source_i)
  {
    DoxterSourceInfo *source = &proj->sources[source_i];
    u32 end = source->first_symbol_index + source->num_symbols;
    for (u32 i = source->first_symbol_index; i < end; ++i)
    {
      DoxterSymbol *sym = x_array_get(proj->symbols, i);
      sym->source_index = source_i;
      sym->group_id = 0;
      if (sym->type == DOXTER_FILE || sym->name.length == 0)
      {
        continue;
      }
      XSlice path = s_symbol_doc_group(sym);
      if (path.length > 0)
      {
        sym->group_id = s_group_intern(proj, path);
      }
    }
  }
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
  DoxterTemplateCtx backup = s_template_ctx;
  s_template_ctx.project = project;
  s_template_ctx.source_index = source_index;

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

    char placeholder[64];
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

  s_template_ctx = backup;
}

// --------------------------------------------------------
// Rendering Helpers
// --------------------------------------------------------

static void s_append_anchor(const DoxterSymbol *sym, XStrBuilder *out)
{
  const char *fragment = strchr(sym->anchor.buf, '#');
  if (fragment)
  {
    x_strbuilder_append(out, fragment + 1);
  }
  else
  {
    x_strbuilder_append_substring(out, sym->name.ptr, sym->name.length);
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
          DoxterTokenSpan param_decl = {0};
          bool has_param_decl = false;

          if (sym->type == DOXTER_FUNCTION)
          {
            has_param_decl = s_function_find_param_span(
                s_template_ctx.project,
                sym,
                tag.name,
                &param_decl);
          }

          DoxterTemplateCtx backup;
          s_template_ctx_push(&backup);
          s_template_ctx.role = DOX_TMPL_ROLE_PARAM_ITEM;
          s_template_ctx.param_name = tag.name;
          s_template_ctx.param_desc = tag.text;
          s_template_ctx.param_decl = param_decl;
          s_template_ctx.has_param_decl = has_param_decl;
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
      s_template_ctx.params_items = x_slice_init(items_str, (size_t)items->length);
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
static void s_render_index_for_type(
    DoxterProject *proj,
    u32 source_index,
    DoxterType type,
    XStrBuilder *out)
{
  const DoxterSourceInfo* source = &(proj->sources[source_index]);

  const u32 first = source->first_symbol_index;
  const u32 end   = first + source->num_symbols;

  u32 capacity = end - first;
  DoxterSymbol **tmp = malloc(sizeof(*tmp) * (capacity ? capacity : 1));
  if (!tmp)
  {
    return;
  }
  u32 count = 0;

  for (u32 i = first; i < end; ++i)
  {
    DoxterSymbol *sym = (DoxterSymbol*)x_array_get(proj->symbols, i);

    if (sym->name.length == 0) continue;
    if (sym->type != type) continue;

    tmp[count++] = sym;
  }

  qsort(tmp, count, sizeof(tmp[0]), s_symbol_name_compare);

  for (u32 i = 0; i < count; ++i)
  {
    DoxterSymbol *sym = tmp[i];

    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_INDEX_ITEM;
    s_template_ctx.local_symbol_links = true;
    s_template_ctx.symbol = sym;

    s_render_template(
        proj->templates.index_item_html,
        proj,
        source_index,
        out);

    s_template_ctx_pop(&backup);
  }
  free(tmp);
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

/* Scope groups using stable numeric IDs; never retain XArray element pointers across pushes. */
static bool s_symbol_matches_scope(const DoxterSymbol *sym, DoxterTemplateRole role, u32 group_id)
{
  if (sym->name.length == 0 || sym->type == DOXTER_FILE)
  {
    return false;
  }
  if (role == DOX_TMPL_ROLE_ALL_SYMBOLS)
  {
    return true;
  }
  if (role == DOX_TMPL_ROLE_UNGROUPED)
  {
    return sym->group_id == 0;
  }
  return role == DOX_TMPL_ROLE_GROUP_INDEX && sym->group_id == group_id;
}

static u32 s_project_collect_scope(DoxterProject *proj, DoxterTemplateRole role,
    u32 group_id, DoxterSymbol ***out_syms)
{
  u32 capacity = x_array_count(proj->symbols);
  DoxterSymbol **syms = malloc(sizeof(*syms) * (capacity ? capacity : 1));
  if (!syms)
  {
    *out_syms = NULL;
    return 0;
  }

  u32 count = 0;
  for (u32 i = 0; i < capacity; ++i)
  {
    DoxterSymbol *sym = x_array_get(proj->symbols, i);
    if (s_symbol_matches_scope(sym, role, group_id))
    {
      syms[count++] = sym;
    }
  }
  qsort(syms, count, sizeof(*syms), s_symbol_name_compare);
  *out_syms = syms;
  return count;
}

static void s_render_scope_index(DoxterProject *proj, DoxterTemplateRole role,
    u32 group_id, XStrBuilder *out)
{
  DoxterSymbol **syms;
  u32 count = s_project_collect_scope(proj, role, group_id, &syms);
  for (u32 i = 0; i < count; ++i)
  {
    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_INDEX_ITEM;
    s_template_ctx.symbol = syms[i];
    s_render_template(proj->templates.index_item_html, proj, syms[i]->source_index, out);
    s_template_ctx_pop(&backup);
  }
  free(syms);
}

static int s_group_pointer_compare(const void *a, const void *b)
{
  const DoxterGroup *ga = *(const DoxterGroup *const *)a;
  const DoxterGroup *gb = *(const DoxterGroup *const *)b;
  size_t n = ga->name.length < gb->name.length ? ga->name.length : gb->name.length;
  int cmp = memcmp(ga->name.ptr, gb->name.ptr, n);
  if (cmp != 0)
  {
    return cmp;
  }
  return (ga->name.length > gb->name.length) - (ga->name.length < gb->name.length);
}

static void s_append_group_name(XStrBuilder *out, XSlice name)
{
  if (name.length == 0)
  {
    return;
  }
  char ch = name.ptr[0];
  if (ch >= 'a' && ch <= 'z')
  {
    x_strbuilder_append_char(out, ch - 'a' + 'A');
  }
  else
  {
    x_strbuilder_append_char(out, ch);
  }
  if (name.length > 1)
  {
    x_strbuilder_append_substring(out, name.ptr + 1, name.length - 1);
  }
}

static u32 s_collect_children(DoxterProject *proj, u32 parent_id, DoxterGroup ***out_children)
{
  u32 capacity = x_array_count(proj->groups);
  DoxterGroup **children = malloc(sizeof(*children) * (capacity ? capacity : 1));
  if (!children)
  {
    *out_children = NULL;
    return 0;
  }
  u32 count = 0;
  for (u32 i = 0; i < capacity; ++i)
  {
    DoxterGroup *g = x_array_get(proj->groups, i);
    if (g->parent_id == parent_id)
    {
      children[count++] = g;
    }
  }
  qsort(children, count, sizeof(*children), s_group_pointer_compare);
  *out_children = children;
  return count;
}

/* Function-style macros participate in the API tree; constants and types do not. */
static bool s_symbol_is_api_function(const DoxterSymbol *sym)
{
  if (sym->name.length == 0)
  {
    return false;
  }
  if (sym->type == DOXTER_FUNCTION)
  {
    return sym->group_id != 0 || sym->comment.length > 0;
  }
  if (sym->type == DOXTER_MACRO)
  {
    return sym->group_id != 0 && sym->stmt.macro.args_ts.count > 0;
  }
  return false;
}

static u32 s_collect_api_functions(DoxterProject *proj, u32 group_id,
    DoxterSymbol ***out_syms)
{
  u32 capacity = x_array_count(proj->symbols);
  DoxterSymbol **syms = malloc(sizeof(*syms) * (capacity ? capacity : 1));
  if (!syms)
  {
    *out_syms = NULL;
    return 0;
  }
  u32 count = 0;
  for (u32 i = 0; i < capacity; ++i)
  {
    DoxterSymbol *sym = x_array_get(proj->symbols, i);
    if (sym->group_id == group_id && s_symbol_is_api_function(sym))
    {
      syms[count++] = sym;
    }
  }
  qsort(syms, count, sizeof(*syms), s_symbol_name_compare);
  *out_syms = syms;
  return count;
}

static bool s_group_contains_api(DoxterProject *proj, u32 group_id)
{
  u32 capacity = x_array_count(proj->symbols);
  for (u32 i = 0; i < capacity; ++i)
  {
    const DoxterSymbol *sym = x_array_get(proj->symbols, i);
    if (!s_symbol_is_api_function(sym))
    {
      continue;
    }
    u32 current_id = sym->group_id;
    while (current_id != 0)
    {
      if (current_id == group_id)
      {
        return true;
      }
      const DoxterGroup *group = s_group_by_id(proj, current_id);
      current_id = group ? group->parent_id : 0;
    }
  }
  return false;
}

static bool s_group_contains_source_api(DoxterProject *proj, u32 group_id,
    u32 source_index)
{
  u32 count = x_array_count(proj->symbols);
  for (u32 i = 0; i < count; ++i)
  {
    const DoxterSymbol *sym = x_array_get(proj->symbols, i);
    if (sym->source_index != source_index || !s_symbol_is_api_function(sym))
    {
      continue;
    }
    u32 current_id = sym->group_id;
    while (current_id != 0)
    {
      if (current_id == group_id)
      {
        return true;
      }
      const DoxterGroup *group = s_group_by_id(proj, current_id);
      current_id = group ? group->parent_id : 0;
    }
  }
  return false;
}

static void s_render_api_nav_items(DoxterProject *proj, u32 group_id,
    XStrBuilder *out)
{
  DoxterSymbol **syms = NULL;
  u32 count = s_collect_api_functions(proj, group_id, &syms);
  for (u32 i = 0; i < count; ++i)
  {
    x_strbuilder_append(out, "<li class=\"api-function\"><a href=\"");
    x_strbuilder_append(out, syms[i]->anchor.buf);
    x_strbuilder_append(out, "\">");
    x_strbuilder_append_substring(out, syms[i]->name.ptr, syms[i]->name.length);
    x_strbuilder_append(out, "</a></li>");
  }
  free(syms);
}

static void s_render_group_tree(DoxterProject *proj, u32 parent_id, XStrBuilder *out)
{
  DoxterGroup **children = NULL;
  u32 count = s_collect_children(proj, parent_id, &children);
  for (u32 i = 0; i < count; ++i)
  {
    const DoxterGroup *group = children[i];
    u32 child_id = (u32)(group - (DoxterGroup *)x_array_data(proj->groups)) + 1;
    if (!s_group_contains_api(proj, child_id))
    {
      continue;
    }
    x_strbuilder_append(out,
        "<li class=\"group-node\"><details class=\"group-branch\"");
    if (s_template_ctx.role != DOX_TMPL_ROLE_PROJECT_INDEX &&
        s_group_contains_source_api(proj, child_id, s_template_ctx.source_index))
    {
      x_strbuilder_append(out, " open");
    }
    x_strbuilder_append(out, "><summary>");
    s_append_group_name(out, group->name);
    x_strbuilder_append(out, "</summary><ul class=\"group-children\">");
    s_render_api_nav_items(proj, child_id, out);
    s_render_group_tree(proj, child_id, out);
    x_strbuilder_append(out, "</ul></details></li>");
  }
  free(children);
}

static bool s_placeholder_match(const char *a, u32 hash_a, const char *b, u32 hash_b)
{
  return hash_a == hash_b && strcmp(a, b) == 0;
}

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

  if (s_placeholder_match(placeholder, hash, DOX_PH_GROUP_TREE_STR, DOX_PH_GROUP_TREE_HASH))
  {
    s_render_group_tree(project, 0, out);
    DoxterSymbol **ungrouped = NULL;
    u32 count = s_collect_api_functions(project, 0, &ungrouped);
    free(ungrouped);
    if (count > 0)
    {
      x_strbuilder_append(out,
          "<li class=\"group-node\"><details class=\"group-branch\"><summary>Ungrouped</summary><ul class=\"group-children\">");
      s_render_api_nav_items(project, 0, out);
      x_strbuilder_append(out, "</ul></details></li>");
    }
    return;
  }
  if (s_placeholder_match(placeholder, hash, DOX_PH_GROUP_NAME_STR, DOX_PH_GROUP_NAME_HASH))
  {
    if (s_template_ctx.group)
    {
      s_append_group_name(out, s_template_ctx.group->name);
    }
    return;
  }
  if (s_placeholder_match(placeholder, hash, DOX_PH_PAGE_TITLE_STR, DOX_PH_PAGE_TITLE_HASH))
  {
    x_strbuilder_append(out, s_template_ctx.role == DOX_TMPL_ROLE_UNGROUPED ? "Ungrouped" : "All Symbols");
    return;
  }
  if (s_placeholder_match(placeholder, hash, DOX_PH_SCOPE_INDEX_STR, DOX_PH_SCOPE_INDEX_HASH))
  {
    DoxterTemplateRole role = s_template_ctx.role;
    u32 group_id = 0;
    if (role == DOX_TMPL_ROLE_GROUP_INDEX && s_template_ctx.group)
    {
      group_id = (u32)(s_template_ctx.group - (DoxterGroup *)x_array_data(project->groups)) + 1;
    }
    s_render_scope_index(project, role, group_id, out);
    return;
  }
  if (s_placeholder_match(placeholder, hash, DOX_PH_SYMBOL_HREF_STR, DOX_PH_SYMBOL_HREF_HASH))
  {
    if (s_template_ctx.symbol)
    {
      if (s_template_ctx.local_symbol_links)
      {
        x_strbuilder_append_char(out, '#');
        s_append_anchor(s_template_ctx.symbol, out);
      }
      else
      {
        x_strbuilder_append(out, s_template_ctx.symbol->anchor.buf);
      }
    }
    return;
  }
  if (s_placeholder_match(placeholder, hash, DOX_PH_SOURCE_NAME_STR, DOX_PH_SOURCE_NAME_HASH))
  {
    if (s_template_ctx.symbol && s_template_ctx.symbol->source_index < project->source_count)
    {
      x_strbuilder_append(out, project->sources[s_template_ctx.symbol->source_index].base_name);
    }
    return;
  }
  if (s_placeholder_match(placeholder, hash, DOX_PH_SOURCE_HREF_STR, DOX_PH_SOURCE_HREF_HASH))
  {
    if (s_template_ctx.symbol && s_template_ctx.symbol->source_index < project->source_count)
    {
      x_strbuilder_append(out, s_template_ctx.symbol->anchor.buf);
    }
    return;
  }

  /* Param item */
  if (s_placeholder_match(placeholder, hash, DOX_PH_PARAM_NAME_STR,
        DOX_PH_PARAM_NAME_HASH))
  {
    if (s_template_ctx.has_param_decl)
    {
      doxter_pretty_format_span(
          s_template_ctx.project,
          s_template_ctx.param_decl,
          out);
    }
    else
    {
      x_strbuilder_append_substring(out,
          s_template_ctx.param_name.ptr,
          s_template_ctx.param_name.length);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_PARAM_DESC_STR, DOX_PH_PARAM_DESC_HASH))
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.param_desc.ptr,
        s_template_ctx.param_desc.length);
    return;
  }

  /* Params block */
  if (s_placeholder_match(placeholder, hash,
        DOX_PH_PARAMS_ITEMS_STR, DOX_PH_PARAMS_ITEMS_HASH))
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.params_items.ptr,
        s_template_ctx.params_items.length);
    return;
  }

  /* Return block */
  if (s_placeholder_match(placeholder, hash,
        DOX_PH_RETURN_DESC_STR, DOX_PH_RETURN_DESC_HASH))
  {
    x_strbuilder_append_substring(out,
        s_template_ctx.return_desc.ptr,
        s_template_ctx.return_desc.length);
    return;
  }

  /* File item (side bar / lists) */
  if (s_placeholder_match(placeholder, hash,
        DOX_PH_MODULE_NAME_STR, DOX_PH_MODULE_NAME_HASH))
  {
    if (s_template_ctx.source && s_template_ctx.source->base_name)
    {
      x_strbuilder_append(out, s_template_ctx.source->base_name);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_MODULE_HREF_STR, DOX_PH_MODULE_HREF_HASH))
  {
    if (s_template_ctx.source && s_template_ctx.source->output_name)
    {
      x_strbuilder_append(out, s_template_ctx.source->output_name);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_FILE_NAME_STR, DOX_PH_FILE_NAME_HASH))
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

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_TITLE_STR, DOX_PH_TITLE_HASH))
  {
    if (s_template_ctx.role == DOX_TMPL_ROLE_PROJECT_INDEX)
    {
      x_strbuilder_append(out, "Symbols");
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_FILE_BRIEF_STR, DOX_PH_FILE_BRIEF_HASH))
  {
    if (!source)
      return;

    DoxterSymbol* file_comment =
      (DoxterSymbol*) x_array_get(project->symbols, source->first_symbol_index);

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

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_INDEX_ITEMS_STR, DOX_PH_INDEX_ITEMS_HASH))
  {
    s_render_index_for_type(project, source_index, DOXTER_FUNCTION, out);
    s_render_index_for_type(project, source_index, DOXTER_MACRO,    out);
    s_render_index_for_type(project, source_index, DOXTER_STRUCT,   out);
    s_render_index_for_type(project, source_index, DOXTER_ENUM,     out);
    s_render_index_for_type(project, source_index, DOXTER_TYPEDEF,  out);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_INDEX_FUNCTIONS_STR, DOX_PH_INDEX_FUNCTIONS_HASH))
  {
    s_render_index_for_type(project, source_index, DOXTER_FUNCTION, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_INDEX_MACROS_STR, DOX_PH_INDEX_MACROS_HASH))
  {
    s_render_index_for_type(project, source_index, DOXTER_MACRO, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_INDEX_STRUCTS_STR, DOX_PH_INDEX_STRUCTS_HASH))
  {
    s_render_index_for_type(project, source_index, DOXTER_STRUCT, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_INDEX_ENUMS_STR, DOX_PH_INDEX_ENUMS_HASH))
  {
    s_render_index_for_type(project, source_index, DOXTER_ENUM, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_INDEX_TYPEDEFS_STR, DOX_PH_INDEX_TYPEDEFS_HASH))
  {
    s_render_index_for_type(project, source_index, DOXTER_TYPEDEF, out);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_SYMBOLS_STR, DOX_PH_SYMBOLS_HASH))
  {
    if (!source || !project->templates.symbol_html)
      return;

    const u32 first = source->first_symbol_index;
    const u32 count = first + source->num_symbols;
    for (u32 symbol_i = first; symbol_i < count; symbol_i++)
    {
      DoxterSymbol *sym = (DoxterSymbol *) x_array_get(project->symbols, symbol_i);

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

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_MODULE_LIST_STR, DOX_PH_MODULE_LIST_HASH))
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
  if (s_placeholder_match(placeholder, hash,
        DOX_PH_ANCHOR_STR, DOX_PH_ANCHOR_HASH))
  {
    if (s_template_ctx.symbol)
    {
      s_append_anchor(s_template_ctx.symbol, out);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_NAME_STR, DOX_PH_NAME_HASH))
  {
    if (s_template_ctx.symbol)
    {
      x_strbuilder_append_substring(out,
          s_template_ctx.symbol->name.ptr,
          s_template_ctx.symbol->name.length);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_KIND_STR, DOX_PH_KIND_HASH))
  {
    if (s_template_ctx.symbol)
    {
      x_strbuilder_append(out, s_type_to_string(s_template_ctx.symbol->type));
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_LINE_STR, DOX_PH_LINE_HASH))
  {
    if (s_template_ctx.symbol)
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "%u", (unsigned) s_template_ctx.symbol->line);
      x_strbuilder_append(out, buf);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_DECL_STR, DOX_PH_DECL_HASH))
  {
    if (s_template_ctx.symbol)
    {
      doxter_pretty_format_symbol(project, s_template_ctx.symbol, out);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_BRIEF_STR, DOX_PH_BRIEF_HASH))
  {
    if (s_template_ctx.symbol)
    {
      XSlice b = x_slice_trim(s_symbol_doc_brief(s_template_ctx.symbol));
      x_strbuilder_append_substring(out, b.ptr, b.length);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_PARAMS_BLOCK_STR, DOX_PH_PARAMS_BLOCK_HASH))
  {
    if (s_template_ctx.symbol)
    {
      s_render_params_block(s_template_ctx.symbol, &project->templates, out);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_RETURN_BLOCK_STR, DOX_PH_RETURN_BLOCK_HASH))
  {
    if (s_template_ctx.symbol)
    {
      s_render_return_block(s_template_ctx.symbol, &project->templates, out);
    }
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_PROJECT_NAME_STR, DOX_PH_PROJECT_NAME_HASH))
  {
    x_strbuilder_append(out, project->config.project_name);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_PROJECT_URL_STR, DOX_PH_PROJECT_URL_HASH))
  {
    x_strbuilder_append(out, project->config.project_url);
    return;
  }

  if (s_placeholder_match(placeholder, hash,
        DOX_PH_PROJECT_BRIEF_STR, DOX_PH_PROJECT_BRIEF_HASH))
  {
    x_strbuilder_append(out, project->config.project_brief);
    return;
  }

  /* CSS */
  if (s_template_ctx.role == DOX_TMPL_ROLE_STYLE_CSS && s_template_ctx.config)
  {
    const DoxterConfig *cfg = s_template_ctx.config;

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_PAGE_BACKGROUND_STR, DOX_PH_COLOR_PAGE_BACKGROUND_HASH))
    {
      x_strbuilder_append(out, cfg->color_page_background);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_SIDEBAR_BACKGROUND_STR, DOX_PH_COLOR_SIDEBAR_BACKGROUND_HASH))
    {
      x_strbuilder_append(out, cfg->color_sidebar_background);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_MAIN_TEXT_STR, DOX_PH_COLOR_MAIN_TEXT_HASH))
    {
      x_strbuilder_append(out, cfg->color_main_text);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_SECONDARY_TEXT_STR, DOX_PH_COLOR_SECONDARY_TEXT_HASH))
    {
      x_strbuilder_append(out, cfg->color_secondary_text);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_HIGHLIGHT_STR, DOX_PH_COLOR_HIGHLIGHT_HASH))
    {
      x_strbuilder_append(out, cfg->color_highlight);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_LIGHT_BORDERS_STR, DOX_PH_COLOR_LIGHT_BORDERS_HASH))
    {
      x_strbuilder_append(out, cfg->color_light_borders);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_CODE_BLOCKS_STR, DOX_PH_COLOR_CODE_BLOCKS_HASH))
    {
      x_strbuilder_append(out, cfg->color_code_blocks);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_CODE_BLOCK_BORDER_STR, DOX_PH_COLOR_CODE_BLOCK_BORDER_HASH))
    {
      x_strbuilder_append(out, cfg->color_code_block_border);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_PP_STR, DOX_PH_COLOR_TOK_PP_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_pp);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_KW_STR, DOX_PH_COLOR_TOK_KW_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_kw);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_ID_STR, DOX_PH_COLOR_TOK_ID_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_id);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_NUM_STR, DOX_PH_COLOR_TOK_NUM_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_num);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_STR_STR, DOX_PH_COLOR_TOK_STR_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_str);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_CRT_STR, DOX_PH_COLOR_TOK_CRT_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_crt);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_PUN_STR, DOX_PH_COLOR_TOK_PUN_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_pun);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_COLOR_TOK_DOC_STR, DOX_PH_COLOR_TOK_DOC_HASH))
    {
      x_strbuilder_append(out, cfg->color_tok_doc);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_FONT_UI_STR, DOX_PH_FONT_UI_HASH))
    {
      x_strbuilder_append(out, cfg->font_ui);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_FONT_HEADING_STR, DOX_PH_FONT_HEADING_HASH))
    {
      x_strbuilder_append(out, cfg->font_heading);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_FONT_CODE_STR, DOX_PH_FONT_CODE_HASH))
    {
      x_strbuilder_append(out, cfg->font_code);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_FONT_SYMBOL_STR, DOX_PH_FONT_SYMBOL_HASH))
    {
      x_strbuilder_append(out, cfg->font_symbol);
      return;
    }

    if (s_placeholder_match(placeholder, hash,
          DOX_PH_BORDER_RADIUS_STR, DOX_PH_BORDER_RADIUS_HASH))
    {
      x_strbuilder_append_format(out, "%d", cfg->border_radius);
      return;
    }

    doxter_warning("Unknown key for css file: '%s'\n", placeholder);
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
  cfg->project_brief = x_ini_get(&ini, "project", "brief", cfg->project_brief);

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
  proj->symbol_map                              = x_hashtable_dox_symbol_map_create();
  proj->symbols                                 = x_array_create(sizeof(DoxterSymbol), 256);
  proj->groups                                  = x_array_create(sizeof(DoxterGroup), 32);
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
  proj->config.project_brief                    = "Project documentation";
  proj->config.skip_static_functions     = 1;
  proj->config.markdown_index_page       = 1;
  proj->config.markdown_gobal_comments   = 1;
  proj->config.markdown_index_page       = 1;
  proj->config.skip_undocumented         = 1;

  // If provided, load the doxter file
  if (args->doxter_file && x_fs_is_file(args->doxter_file))
    s_load_doxter_file(args->doxter_file, &proj->config);

  // Load templates
  proj->templates.group_index_html     = (char*) DOX_TMPL_GROUP_INDEX_BYTES;
  proj->templates.symbols_index_html   = (char*) DOX_TMPL_SYMBOLS_INDEX_BYTES;
  proj->templates.project_index_html   = (char*) DOX_TMPL_PROJECT_INDEX_BYTES;
  proj->templates.file_index_html      = (char*) DOX_TMPL_MODULE_INDEX_BYTES;
  proj->templates.symbol_html          = (char*) DOX_TMPL_SYMBOL_BYTES;
  proj->templates.index_item_html      = (char*) DOX_TMPL_INDEX_ITEM_BYTES;
  proj->templates.style_css            = (char*) DOX_TMPL_STYLE_CSS_BYTES ;
  proj->templates.project_file_item_html = (char*) DOX_TMPL_PROJECT_MODULE_ITEM_BYTES;
  proj->templates.params_html          = (char*) DOX_TMPL_PARAMS_BYTES;
  proj->templates.param_item_html      = (char*) DOX_TMPL_PARAM_ITEM_BYTES;
  proj->templates.return_html          = (char*) DOX_TMPL_RETURN_BYTES;
  proj->templates.file_item_html       = (char*) DOX_TMPL_MODULE_ITEM_BYTES;
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
  x_array_destroy(proj->symbols);
  x_array_destroy(proj->groups);
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

  s_project_build_groups(proj);

  // --------------------------------------------------------
  // Save each symbol from each source into the stdx_hashtable
  // for cross links later.
  // --------------------------------------------------------
  for (u32 source_i = 0; source_i < proj->source_count; source_i++)
  {
    DoxterSourceInfo* source_info = &(proj->sources[source_i]);
    u32 start = source_info->first_symbol_index;
    u32 end   = start + source_info->num_symbols;
    XSmallstr key;

    for (u32 i_symbol = start; i_symbol < end; i_symbol++)
    {
      DoxterSymbol* sym = x_array_get(proj->symbols, i_symbol);
      x_smallstr_clear(&sym->anchor);
      if (sym->type == DOXTER_FILE)
      {
        x_smallstr_appendf(&sym->anchor, "%s%c", source_info->output_name, 0);
        memset(&key, 0, sizeof(key));
        x_smallstr_from_slice(sym->name, &key);
        x_hashtable_dox_symbol_map_set(proj->symbol_map, key, *sym);
      }
      else
      {
        u32 occurrence = 1;
        for (u32 previous_i = start; previous_i < i_symbol; ++previous_i)
        {
          const DoxterSymbol *previous = x_array_get(proj->symbols, previous_i);
          if (previous->type != DOXTER_FILE &&
              previous->name.length == sym->name.length &&
              memcmp(previous->name.ptr, sym->name.ptr, sym->name.length) == 0)
          {
            ++occurrence;
          }
        }
        if (occurrence == 1)
        {
          x_smallstr_appendf(&sym->anchor, "%s#%.*s",
              source_info->output_name, (int)sym->name.length, sym->name.ptr);
        }
        else
        {
          x_smallstr_appendf(&sym->anchor, "%s#%.*s-%u",
              source_info->output_name, (int)sym->name.length, sym->name.ptr,
              occurrence);
        }

        memset(&key, 0, sizeof(key));
        x_smallstr_from_slice(sym->name, &key);
        x_hashtable_dox_symbol_map_set(proj->symbol_map, key, *sym);

      }
    }
  }

  for (u32 source_i = 0; source_i < proj->source_count; ++source_i)
  {
    const char *file_name = proj->sources[source_i].output_name;
    if (s_output_name_equal(file_name, "index.html"))
    {
      doxter_error("Reserved output filename: %s\n", file_name);
      s_doxter_project_destroy(proj);
      return 1;
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
  // index.html is navigation only; documentation lives on source pages.
  // --------------------------------------------------------

  // --------------------------------------------------------
  // Generate main project index.html
  // --------------------------------------------------------
  x_strbuilder_clear(sb);
  {
    DoxterTemplateCtx backup;
    s_template_ctx_push(&backup);
    s_template_ctx.role = DOX_TMPL_ROLE_PROJECT_INDEX;
    s_render_template(proj->templates.project_index_html, proj, 0, sb);
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
  x_fs_path(&full_path, args.output_directory, "style.css");

  s_render_template(proj->templates.style_css,
      proj, 0, sb); s_template_ctx_pop(&backup);

  char *css = x_strbuilder_to_string(sb);

  if (!x_io_write_text(full_path.buf, css))
  {
    fprintf(stderr, "Failed to write project css file '%s'\\n", full_path.buf);
    had_error = true;
  }

  // --------------------------------------------------------
  // Generate builtin Fonts
  // --------------------------------------------------------
  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Bold.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, DOX_FONT_JETBRAINS_MONO_BOLD_BYTES,
        DOX_FONT_JETBRAINS_MONO_BOLD_FILE_SIZE);
    x_io_close(f);
  }

  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Italic.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, DOX_FONT_JETBRAINS_MONO_ITALIC_BYTES,
        DOX_FONT_JETBRAINS_MONO_ITALIC_FILE_SIZE);
    x_io_close(f);
  }

  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Medium.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, DOX_FONT_JETBRAINS_MONO_MEDIUM_BYTES,
        DOX_FONT_JETBRAINS_MONO_MEDIUM_FILE_SIZE);
    x_io_close(f);
  }

  x_fs_path(&full_path, args.output_directory, "JetBrainsMono-Regular.woff2");
  if (!x_fs_is_file(full_path.buf))
  {
    XFile* f = x_io_open(full_path.buf, "wb+");
    x_io_write(f, DOX_FONT_JETBRAINS_MONO_REGULAR_BYTES,
        DOX_FONT_JETBRAINS_MONO_REGULAR_FILE_SIZE);
    x_io_close(f);
  }

  x_strbuilder_destroy(sb);
  s_doxter_project_destroy(proj);

  return had_error ? 1 : 0;
}

