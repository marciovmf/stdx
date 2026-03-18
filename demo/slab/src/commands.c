#include "slab.h"
#include <stdlib.h>

static MiExecResult mi_page_eval_arg(MiContext *ctx, MiNode *node, MiValue *out_value)
{
  MiExecResult result;

  result = mi_eval_node(ctx, node);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  *out_value = result.value;
  return mi_exec_null();
}

static MiExecResult mi_page_eval_name(MiContext *ctx, MiNode *node, XSlice *out_name)
{
  MiExecResult result;

  result = mi_eval_node(ctx, node);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  if (result.value.kind == MI_VAL_STRING)
  {
    *out_name = result.value.as.string;
    return mi_exec_null();
  }

  if (result.value.kind == MI_VAL_RAW)
  {
    *out_name = result.value.as.raw;
    return mi_exec_null();
  }

  mi_context_set_error(ctx, "page property name must be string or raw", 0, 0);
  return mi_exec_error();
}

static MiValue mi_page_prop_value(const char *s)
{
  if (!s)
  {
    return mi_value_null();
  }

  return mi_value_string(x_slice_from_cstr(s));
}

static SlabPage* mi_page_from_value(MiValue v)
{
  if (v.kind != MI_VAL_USER)
  {
    return NULL;
  }

  if (v.as.user.type != &Page_type)
  {
    return NULL;
  }

  return (SlabPage*) v.as.user.data;
}

static bool mi_page_slice_equals_cstr(XSlice slice, const char *cstr)
{
  size_t len;

  if (!cstr)
  {
    return false;
  }

  len = strlen(cstr);

  if (slice.length != len)
  {
    return false;
  }

  if (len == 0)
  {
    return true;
  }

  return memcmp(slice.ptr, cstr, len) == 0;
}

//
// Custom Minima commands
//

MiExecResult slab_cmd_page(MiContext *ctx, i32 argc, MiNode **argv)
{
  XSlice prop_name;
  MiValue page_value;
  SlabPage *page;
  MiExecResult result;

  if (argc != 2)
  {
    mi_context_set_error(ctx, "usage: page PROPERTY PAGE", 0, 0);
    return mi_exec_error();
  }

  result = mi_page_eval_name(ctx, argv[0], &prop_name);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_page_eval_arg(ctx, argv[1], &page_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  page = mi_page_from_value(page_value);
  if (!page)
  {
    mi_context_set_error(ctx, "expected page",
        argv[1]->line, argv[1]->column);
    return mi_exec_error();
  }

  if (mi_page_slice_equals_cstr(prop_name, "source_path"))
  {
    return mi_exec_ok(mi_page_prop_value(page->source_path));
  }

  if (mi_page_slice_equals_cstr(prop_name, "output_path"))
  {
    return mi_exec_ok(mi_page_prop_value(page->output_path));
  }

  if (mi_page_slice_equals_cstr(prop_name, "url"))
  {
    return mi_exec_ok(mi_page_prop_value(page->url));
  }

  if (mi_page_slice_equals_cstr(prop_name, "title"))
  {
    return mi_exec_ok(mi_page_prop_value(page->title));
  }

  if (mi_page_slice_equals_cstr(prop_name, "template_name"))
  {
    return mi_exec_ok(mi_page_prop_value(page->template_name));
  }

  if (mi_page_slice_equals_cstr(prop_name, "year"))
  {
    return mi_exec_ok(mi_value_number(page->year));
  }

  if (mi_page_slice_equals_cstr(prop_name, "month"))
  {
    return mi_exec_ok(mi_value_number(page->month));
  }

  if (mi_page_slice_equals_cstr(prop_name, "day"))
  {
    return mi_exec_ok(mi_value_number(page->day));
  }

  if (mi_page_slice_equals_cstr(prop_name, "date"))
  {
    return mi_exec_ok(mi_page_prop_value(page->date));
  }

  if (mi_page_slice_equals_cstr(prop_name, "author"))
  {
    return mi_exec_ok(mi_page_prop_value(page->author));
  }

  if (mi_page_slice_equals_cstr(prop_name, "tags"))
  {
    return mi_exec_ok(mi_page_prop_value(page->tags));
  }

  if (mi_page_slice_equals_cstr(prop_name, "category"))
  {
    return mi_exec_ok(mi_page_prop_value(page->category));
  }

  if (mi_page_slice_equals_cstr(prop_name, "slug"))
  {
    return mi_exec_ok(mi_page_prop_value(page->slug));
  }

  if (mi_page_slice_equals_cstr(prop_name, "draft"))
  {
    return mi_exec_ok(mi_value_number(page->draft ? 1.0 : 0.0));
  }

  mi_context_set_error(ctx, "unknown page property", 
      argv[0]->line, argv[0]->column);
  return mi_exec_error();
}

MiExecResult slab_cmd_template(MiContext *ctx, int argc, MiNode **argv)
{
  MiExecResult eval_result;
  MiValue path_value;
  XSlice path;
  XFSPath path_cstr;
  FILE *f;
  long fsize;
  size_t size;
  char *file_buf;
  XStrBuilder *sb;
  SlabTemplateResult t_res;
  const char *script_src;
  size_t script_len;

  file_buf = NULL;
  sb = NULL;

  if (!ctx)
  {
    return mi_exec_error();
  }

  if (argc < 1 || !argv || !argv[0])
  {
    mi_context_set_error(ctx, "template expects a path argument",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  eval_result = mi_eval_node(ctx, argv[0]);
  if (eval_result.signal == MI_SIGNAL_ERROR)
  {
    return eval_result;
  }

  path_value = eval_result.value;
  if (path_value.kind != MI_VAL_STRING)
  {
    mi_context_set_error(ctx, "template path must evaluate to a string", 
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  path = path_value.as.string;
  x_fs_path_from_slice(path, &path_cstr);

  f = fopen(path_cstr.buf, "rb");
  if (!f)
  {
    mi_context_set_error(ctx, "template could not open file",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  if (fseek(f, 0, SEEK_END) != 0)
  {
    fclose(f);
    mi_context_set_error(ctx, "template failed to seek file",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  fsize = ftell(f);
  if (fsize < 0)
  {
    fclose(f);
    mi_context_set_error(ctx, "template failed to get file size",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  if (fseek(f, 0, SEEK_SET) != 0)
  {
    fclose(f);
    mi_context_set_error(ctx, "template failed to rewind file",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  size = (size_t)fsize;
  file_buf = (char *)malloc(size + 1u);
  if (!file_buf)
  {
    fclose(f);
    free(file_buf);
    mi_context_set_error(ctx, "template failed to allocate file buffer",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  if (fread(file_buf, 1u, size, f) != size)
  {
    fclose(f);
    free(file_buf);
    mi_context_set_error(ctx, "template failed to read file",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  fclose(f);
  file_buf[size] = '\0';

  sb = x_strbuilder_create();
  if (!sb)
  {
    free(file_buf);
    mi_context_set_error(ctx, "template failed to create string builder",
        argv[0]->line, argv[0]->column);
    return mi_exec_error();
  }

  t_res = slab_expand_minima_template(x_slice_init(file_buf, size), sb);
  if (t_res.error_code != 0)
  {
    x_strbuilder_destroy(sb);
    free(file_buf);
    mi_context_set_error(ctx, t_res.error_message.buf, t_res.error_line, t_res.error_column);
    return mi_exec_error();
  }

  script_src = x_strbuilder_to_string(sb);
  script_len = x_strbuilder_length(sb);

  free(file_buf);

  mi_context_push_source(ctx, x_slice_init(path_cstr.buf, path_cstr.length));
  MiExecResult result = mi_call_cmd_eval(ctx, x_slice_init(script_src, script_len));
  mi_context_pop_source(ctx);
  return result;
}

bool slab_register_mi_commands(MiContext *ctx)
{
  if (!mi_register_builtins(ctx))
    return false;

  if (! mi_register_command(ctx, "template", slab_cmd_template))
    return false;

  if (! mi_register_command(ctx, "page", slab_cmd_page))
    return false;

  return true;
}
