#include <stdx_common.h>

#define X_IMPL_ARENA
#include <stdx_arena.h>
#define X_IMPL_IO
#include <stdx_io.h>
#define X_IMPL_STRBUILDER
#include <stdx_strbuilder.h>
#define X_IMPL_FILESYSTEM
#include <stdx_filesystem.h>
#define X_IMPL_INI
#include <stdx_ini.h>
#define X_IMPL_LOG
#include <stdx_log.h>
#define X_IMPL_HASHTABLE
#include <stdx_hashtable.h>
#define X_IMPL_STRING
#include <stdx_string.h>
#define X_IMPL_ARRAY
#include <stdx_array.h>
#define X_IMPL_ARENA
#include <stdx_arena.h>
#define MD_IMPL
#include "markdown.h"

#include <mi_parser.h>
#include <mi_runtime.h>
#include <mi_builtins.h>

#include "slab.h"

#include <stdio.h>

MI_DEFINE_TYPE(Page);

i32 slab_generate_site(const char* site_root)
{
  if (!x_fs_path_is_directory_cstr(site_root))
  {
    x_log_error("Site path not found: %s", site_root);
    return 1;
  }

  SlabConfig site_config;
  slab_config_load(site_root, &site_config);
  SlabSite* site = slab_site_create(1024 * 1024, &site_config);

  XArena* rt_arena = x_arena_create(1024 * 1024 * 2);
  XStrBuilder* sb = x_strbuilder_create();

  // Collect metadata
  x_log_info("Collecting metedata.");

  slab_process_directory_metadata(site, x_fs_path_cstr(&site_config.template_dir));
  slab_process_directory_metadata(site, x_fs_path_cstr(&site_config.content_dir));

  for(u32 i = 0; i < site->page_count; i++)
  {
    SlabPage* page = &site->pages[i]; 
    page->type = SLAB_PAGE_TYPE_POST;

    XFSPath out_folder;
    x_fs_path(&out_folder, page->output_path);
    x_fs_path_dirname(&out_folder, &out_folder);
    x_fs_directory_create_recursive(out_folder.buf);
    FILE* out = fopen(page->output_path, "w");

    if (!out)
    {
      x_log_error("Failed to create file '%s'.", page->output_path);
      continue;
    }

    MiContext ctx;
    mi_context_init(&ctx, rt_arena, out, stderr);
    if (! slab_register_mi_commands(&ctx) )
    {
      x_log_error("Failed to register commands", 0);
      continue;
    }

    // 
    // Exposes Config as variables
    //

    mi_scope_define(ctx.global_scope, x_slice("site_url"),
        mi_value_string(x_slice(site->config.site_url)));

    mi_scope_define(ctx.global_scope, x_slice("site_name"),
        mi_value_string(x_slice(site->config.site_name)));

    mi_scope_define(ctx.global_scope, x_slice("output_dir"),
        mi_value_string(x_slice(site->config.output_dir.buf)));

    mi_scope_define(ctx.global_scope, x_slice("content_dir"),
        mi_value_string(x_slice(site->config.content_dir.buf)));

    mi_scope_define(ctx.global_scope, x_slice("template_dir"),
        mi_value_string(x_slice(site->config.template_dir.buf)));


    //
    // Exposes post frontmatter as variables
    //

    mi_scope_define(ctx.global_scope, x_slice("post_title"),
        mi_value_string(x_slice(page->title ? page->title : "")));

    mi_scope_define(ctx.global_scope, x_slice("post_date"),
        mi_value_string(x_slice(page->date ? page->date : "")));

    mi_scope_define(ctx.global_scope, x_slice("post_slug"),
        mi_value_string(x_slice(page->slug ? page->slug : "")));

    mi_scope_define(ctx.global_scope, x_slice("post_tags"),
        mi_value_string(x_slice(page->tags ? page->tags : "")));

    mi_scope_define(ctx.global_scope, x_slice("post_author"),
        mi_value_string(x_slice(page->author ? page->author : "")));

    size_t post_body_len = 0;
    char* post_body = x_io_read_text(page->source_path, &post_body_len);
    if(!post_body)
    {
      x_log_error("Failed to read post '%s'\n", page->source_path + page->meta.past_meta_offset);
      continue;
    }

    if (x_cstr_ends_with(page->source_path, ".md"))
    {
      char* markdown = md_to_html(post_body + page->meta.past_meta_offset, post_body_len);
      free(post_body);
      post_body = markdown;
    }

    mi_scope_define(ctx.global_scope, x_slice("post_body"),
        mi_value_string(x_slice(post_body)));


    //
    // Post list
    //

#if 1
    MiValue all_pages = mi_value_list((i32)site->page_count);
    for (i32 i = 0; i < site->page_count; i++)
    {
      MiValue p = mi_value_Page(&site->pages[i]);
      mi_call_cmd_list_push_value(all_pages, p);
    }
    mi_scope_define(ctx.global_scope, x_slice_from_cstr("all_pages"), all_pages);
#endif



    // Append .html to template name
    XSmallstr template_name;
    x_smallstr_format(&template_name, "%s.html", page->template_name);

    // Compose full path to template file
    size_t len;
    XFSPath template_path;
    x_fs_path(&template_path, site_config.template_dir, "_layout", template_name.buf);
    XSmallstr full_path;
    x_smallstr_format(&full_path, "%s.html", template_path.buf);

    if (!x_fs_path_is_file(&template_path))
    {
      x_log_error(
          "Invalid layout '%s' requested by '%s'",
          template_path.buf,
          page->source_path);
      continue;
    }

    const char* src = x_io_read_text(template_path.buf, &len);
    x_strbuilder_clear(sb);
    slab_expand_minima_template(x_slice_init(src, len), sb);

    const char* expanded = x_strbuilder_to_string(sb);

    { // Execute page script

      MiParseResult parse_result = mi_parse(rt_arena, x_slice(expanded));

      if (!parse_result.ok)
      {
        fprintf(stderr,
            "Parse error at %d:%d: %s\n",
            parse_result.error_line,
            parse_result.error_column,
            parse_result.error_message);
        return 1;
      }

      MiExecResult exec = mi_exec_block(&ctx, parse_result.root, false);

      if (exec.signal == MI_SIGNAL_ERROR)
      {
        fprintf(stderr,
            "Runtime error at %d:%d: %s\n",
            ctx.error_line,
            ctx.error_column,
            ctx.error_message);
      }
    }

    free(post_body);
    fclose(out);
  }


  // Copy asset folders

  //XFSPath asset_dir_out;
  //x_fs_path(&asset_dir_out, site->config.assets_dir);
  //x_fs_directory_create_recursive(site->config->
  //x_fs_directory_copy


  slab_site_destroy(site);
  slab_config_unload(&site_config);
  return 0;
}

i32 main(i32 argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <site_root>\n", argv[0]);
    return 1;
  }

  const char* site_root = argv[1];
  if (!x_fs_path_is_directory_cstr(site_root))
  {
    x_log_error("Site path not found: %s", site_root);
    return 1;
  }

  return slab_generate_site(site_root);
}
