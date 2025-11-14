#include <stdx_common.h>
#define X_IMPL_STRING
#define X_IMPL_STRINGBUILDER
#define X_IMPL_ARENA
#define X_IMPL_FILESYSTEM
#define X_IMPL_IO
#define X_IMPL_LOG
#define X_IMPL_INI
#include <stdx_string.h>
#include <stdx_filesystem.h>
#include <stdx_ini.h>
#include <stdx_io.h>
#include <stdx_log.h>
#include <stdx_hashtable.h>
#include <stdio.h>
#include <stdlib.h>

#include "tcl/tcl.h"
#include "tcl/tcl_runtime.h"

#define MD_IMPL
#include "markdown.h"

typedef struct SSGPost
{
  XSmallstr name;
  u32 year;
  u32 month;
  u32 day;
} SSGPost;

typedef struct SSGConfig
{
  const char* site_name;
  const char* site_url;
  XFSPath posts_dir;
  XFSPath template_dir;
  XFSPath assets_dir;
  void* data;
} SSGConfig;

typedef struct SSGTemplate
{
  XSmallstr name;
  XFSPath   path;
} SSGTemplate;

static inline bool s_site_config_load(const char* site_root, SSGConfig* out_config)
{
  if (!site_root || !out_config)
    return false;

  XFSPath site_config_file;
  x_fs_path(&site_config_file, site_root, "site.ini");
  const char* ini_file_path = x_smallstr_cstr(&site_config_file);
  x_log_info("Loading ini file: %s", ini_file_path);

  size_t file_size;
  char *data = x_io_read_text(ini_file_path, &file_size);
  if (!data)
  {
    x_log_error("Unable to load site configuration file '%s'", ini_file_path);
    return false;
  }

  XIni ini;
  XIniError iniError;
  if (! x_ini_load_mem(data, strlen(data), &ini, &iniError))
  {
    x_log_error("Unable to parse site configuration file '%s'", ini_file_path);
    free(data);
    return false;
  }

  const char *s;
  out_config->data = data;
  out_config->site_name = x_ini_get(&ini, "site", "name", "");
  out_config->site_url  = x_ini_get(&ini, "site", "url", "");

  s = x_ini_get(&ini, "site", "posts_dir", ".");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->posts_dir, s);
  else
    x_fs_path(&out_config->posts_dir, site_root, s);
  x_fs_path_normalize(&out_config->posts_dir);

  s = x_ini_get(&ini, "site", "template_dir", "");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->template_dir, s);
  else
    x_fs_path(&out_config->template_dir, site_root, s);
  x_fs_path_normalize(&out_config->template_dir);

  s = x_ini_get(&ini, "site", "assets_dir", "");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->assets_dir, s);
  else
    x_fs_path(&out_config->assets_dir, site_root, s);
  x_fs_path_normalize(&out_config->assets_dir);

  x_log_info("name = %s\nurl = %s\nassets_dir = %s\nposts_dir = %s\ntemplate_dir = %s\n",
      out_config->site_name,
      out_config->site_url,
      x_fs_path_cstr(&out_config->assets_dir),
      x_fs_path_cstr(&out_config->posts_dir),
      x_fs_path_cstr(&out_config->template_dir));
  return true;
}

static inline void s_site_config_unload(SSGConfig* config)
{
  if (!config)
    return;

  free(config->data);
  config->site_name     = NULL;
  config->site_url      = NULL;
  config->data          = NULL;

}

static inline bool s_template_load(const char* template_dir, SSGTemplate* out_template)
{
  if (!x_fs_path_is_directory_cstr(template_dir))
  {
    x_log_error("Template folder %s does not exist.", template_dir);
    return false;
  }

  XFSPath assets_dir;
  XFSPath include_dir;
  XFSPath layout_dir;
  x_fs_path(&assets_dir,  template_dir, "assets");
  x_fs_path(&include_dir, template_dir, "include");
  x_fs_path(&layout_dir,  template_dir, "layout");

  XFSDireEntry fs_dir_entry;
  XFSDireHandle* fs_dir_handle;
  fs_dir_handle = x_fs_find_first_file(x_fs_path_cstr(&layout_dir), &fs_dir_entry);
  if (!fs_dir_handle)
    return false;


  while (x_fs_find_next_file(fs_dir_handle, &fs_dir_entry))
  {
    if (!fs_dir_entry.is_directory)
      x_log_info("%s", fs_dir_entry.name);
  }
  x_fs_find_close(fs_dir_handle);
  return true;
}

#if 0

i32 main(int argc, char **argv)
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

  SSGConfig site_config;
  s_site_config_load(site_root, &site_config);
  s_template_load(x_fs_path_cstr(&site_config.template_dir), NULL);
  s_site_config_unload(&site_config);

  return 0;
}

#endif

int test_tcl(int argc, char** argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "usage: %s script.tcl\n", argv[0]);
    return 1;
  }
  size_t src_len = 0;
  char* src = x_io_read_text(argv[1], &src_len);
  if (!src)
  {
    fprintf(stderr, "error: cannot read '%s'\n", argv[1]);
    return 1;
  }

  size_t arena_bytes = 2u * 1024u * 1024u; // a somehow large number to start
  XArena* arena = x_arena_create(arena_bytes);
  if (!arena)
  {
    fprintf(stderr, "error: OOM arena\n");
    free(src);
    return 1;
  }

  TclEnv env = {0};
  env.head = NULL;
  env.arena = arena;

  TclParseResult pr = tcl_parse_script(src, src_len, arena);
  if (pr.error != TCL_OK)
  {
    fprintf(stderr, "parse error at offset %zu (err=%d)\n", pr.err_offset, pr.error);
    free(src);
    x_arena_destroy(arena);
    return 1;
  }

  // Register commands
  tcl_command_register(&env, "while", tcl_cmd_while);
  tcl_command_register(&env, "set",   tcl_cmd_set);
  tcl_command_register(&env, "puts",  tcl_cmd_puts);
  tcl_command_register(&env, "incr",  tcl_cmd_incr);
  tcl_command_register(&env, "if",    tcl_cmd_if);
  tcl_command_register(&env, "expr",  tcl_cmd_expr);

  (void)tcl_eval_script(pr.script, &env, arena);


  free(src);
  x_arena_destroy(arena);
  return 0;
}

i32 main(int argc, char **argv)
{
  return test_tcl(argc, argv);
}
