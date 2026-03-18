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

#include "slab.h"

#include <stdio.h>

MI_DEFINE_TYPE(Page);

i32 slab_generate_site(const char* site_root)
{
  if (!x_fs_path_is_directory_cstr(site_root))
  {
    log_error("Site path not found: %s\n", site_root);
    return 1;
  }

  SlabConfig site_config;
  slab_config_load(site_root, &site_config);
  SlabSite* site = slab_site_create(1024 * 1024, &site_config);

  XArena* rt_arena = x_arena_create(1024 * 1024 * 2);
  XStrBuilder* sb = x_strbuilder_create();

  // Collect metadata
  x_log_info("Processing site folder...");
  slab_process_site(site);
  x_log_info("Generating pages...");

  i32 return_code = 0;

  // Process each page
  for(u32 i = 0; i < site->page_count; i++)
  {
    XFSPath out_folder;
    SlabPage* page = &site->pages[i]; 
    x_fs_path(&out_folder, page->output_path);
    x_fs_path_dirname(&out_folder, &out_folder);
    x_fs_directory_create_recursive(out_folder.buf);
    FILE* out = fopen(page->output_path, "w");

    if (!out)
    {
      log_error("[FAIL] %s -> Failed to create file '%s'.\n", page->source_path, page->output_path);
      return_code = 1;
      continue;
    }

    MiContext ctx;
    mi_context_init(&ctx, rt_arena, out, stderr);
    if (! slab_register_mi_commands(&ctx) )
    {
      log_error("[FAIL] %s -> Failed to register commands\n", page->source_path);
      return_code = 1;
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
      log_error("[FAIL] %s -> Failed to read contents file\n", page->source_path);
      return_code = 1;
      fclose(out);
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
    // Inject 'all_pages' variable
    //
    {
      MiValue all_pages = mi_value_list((i32)site->page_count);
      for (i32 i = 0; i < site->page_count; i++)
      {
        MiValue mi_value = mi_value_Page(&site->pages[i]);
        mi_call_cmd_list_push_value(all_pages, mi_value);
      }
      mi_scope_define(ctx.global_scope, x_slice_from_cstr("all_pages"), all_pages);
    }

    //
    // Inject 'all_categories' variable
    //
    {
      MiValue all_categories = mi_value_list((i32)site->category_count);
      for (i32 i = 0; i < (i32)site->category_count; i++)
      {
        SlabCategory* category = &site->categories[i];
        MiValue mi_value = mi_value_string(x_slice(category->name ? category->name : ""));
        mi_call_cmd_list_push_value(all_categories, mi_value);
      }
      mi_scope_define(ctx.global_scope, x_slice_from_cstr("all_categories"), all_categories);
    }

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
      log_error(
          "[FAIL] %s -> layout not found '%s'\n",
          page->source_path, template_path.buf);
      return_code = 1;
      free(post_body);
      fclose(out);
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
        log_error("[FAIL] %s -> Parse error at %d:%d: %s\n",
            page->source_path,
            parse_result.error_line,
            parse_result.error_column,
            parse_result.error_message);
        return_code = 1;
        free(post_body);
        fclose(out);
        continue;
      }

      MiExecResult exec = mi_exec_block(&ctx, parse_result.root, false);

      if (exec.signal == MI_SIGNAL_ERROR)
      {
        log_error("[FAIL] %s -> Runtime error at %s %d:%d: %s\n",
            page->source_path,
            ctx.error_source_file.buf,
            ctx.error_line,
            ctx.error_column,
            ctx.error_message);

        return_code = 1;
        free(post_body);
        fclose(out);
        continue;
      }
    }

    free(post_body);
    fclose(out);
    log_info("[ OK ] generate %s -> %s\n", page->source_path, page->url);
  }

  slab_site_destroy(site);
  slab_config_unload(&site_config);

  x_log_info("Done");
  return return_code;
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
    log_error("Site path not found: %s\n", site_root);
    return 1;
  }

  return slab_generate_site(site_root);
}
