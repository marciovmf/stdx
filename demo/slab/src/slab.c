#include <stdx_common.h>
#include <stdx_string.h>
#include <stdx_filesystem.h>
#include <stdx_ini.h>
#include <stdx_io.h>
#include <stdx_log.h>
#include <stdio.h>
#include <stdlib.h>

#include "slab.h"
#include "stdx_arena.h"

//
// Helpers
//

static void s_parse_date_yyyy_mm_dd(const char *s, int *year, int *month, int *day)
{
  int y = 0;
  int m = 0;
  int d = 0;
  int digits = 0;

  const char *p = s;


  //
  // Parse year
  //
  while (*p && isdigit((unsigned char)*p))
  {
    y = y * 10 + (*p - '0');
    digits++;
    p++;
  }

  if (digits != 4)
  {
    x_log_warning("Date format '%s': year must have 4 digits, using 1900\n", s);
    y = 1900;
  }

  /* Skip separator */
  while (*p && !isdigit((unsigned char)*p))
  {
    p++;
  }


  //
  // Parse month
  //

  digits = 0;

  while (*p && isdigit((unsigned char)*p))
  {
    m = m * 10 + (*p - '0');
    digits++;
    p++;
  }

  if (m <= 0 || m > 12)
  {
    x_log_warning("Date format '%s': month out of range (%d), using 1\n", s, m);
    m = 1;
  }

  /* Skip separator */
  while (*p && !isdigit((unsigned char)*p))
  {
    p++;
  }

  //
  // Parse day
  //

  digits = 0;

  while (*p && isdigit((unsigned char)*p))
  {
    d = d * 10 + (*p - '0');
    digits++;
    p++;
  }

  if (d <= 0 || d > 31)
  {
    x_log_warning("Date format '%s': day out of range (%d), using 1\n", s, d);
    d = 1;
  }

  *year  = y;
  *month = m;
  *day   = d;
}

static i32 s_slab_site_add_page(SlabSite* site, const SlabPage* page, XArena* arena)
{
  if (!site || !page)
  {
    return 0;
  }

  if (site->page_count == site->page_capacity)
  {
    size_t new_cap;
    size_t bytes;
    SlabPage* new_pages;

    new_cap = (site->page_capacity == 0) ? 16 : site->page_capacity * 2;
    bytes = new_cap * sizeof(SlabPage);

    new_pages = (SlabPage*)x_arena_alloc(arena, bytes);
    if (!new_pages)
    {
      return 0;
    }

    if (site->pages && site->page_count > 0)
    {
      memcpy(new_pages, site->pages, site->page_count * sizeof(SlabPage));
    }

    site->pages = new_pages;
    site->page_capacity = new_cap;
  }

  SlabPage* dest = &site->pages[site->page_count++];
  *dest = *page; // l-value specifies const object

  return 1;
}

static void s_slab_page_property_set(SlabPage* page, XArena* arena, XSlice* key, XSlice* val)
{
#define MATCH(lit) (key->length == sizeof(lit) - 1 && \
    memcmp(key->ptr, lit, key->length) == 0)

  char* copied;

  copied = x_arena_slicedup(arena, val->ptr, val->length, true);
  if (!copied)
  {
    return;
  }

  if (MATCH("type"))
  {
    if (strncmp(copied, "post", 4) == 0)
    {
      page->type = SLAB_PAGE_TYPE_POST;
    }
    else
    {
      page->type = SLAB_PAGE_TYPE_STATIC;
    }
  }
  else if (MATCH("title"))
  {
    page->title = copied;
  }
  else if (MATCH("slug"))
  {
    /* For posts, slug usually comes from filename; you can decide
     * whether to allow overriding it here. For now, override. */
    page->slug = copied;
  }
  else if (MATCH("template"))
  {
    page->template_name = copied;
  }
  else if (MATCH("date"))
  {
    /* Same story as slug: can override filename if desired. */
    s_parse_date_yyyy_mm_dd(copied, 
        (i32*) &page->year,
        (i32*) &page->month,
        (i32*) &page->day);
  }
  else if (MATCH("author"))
  {
    page->author = copied;
  }
  else if (MATCH("tags"))
  {
    page->tags = copied;
  }
  else if (MATCH("category"))
  {
    page->category = copied;
  }
  else
  {
    printf("#### %.*s = %s\n", (int)key->length, key->ptr, copied);
  }

#undef MATCH
}

//
// SLAB Public API
//

i32 slab_process_directory_metadata(SlabSite* site, const char* path)
{
  XFSDireEntry dir_entry;
  XArena* site_arena = site->arena;
  XFSDireHandle* handle = x_fs_find_first_file(path, &dir_entry);
  if (!handle)
  {
    x_log_error("Failed to scan posts directory %s\n", path);
    return 1;
  }

  do
  {
    if (! (x_cstr_ends_with(dir_entry.name, ".md")  ||
          x_cstr_ends_with(dir_entry.name, ".html") ||
          x_cstr_ends_with(dir_entry.name, ".htm")))
      continue;

    SlabPage page = {0};
    page.type = SLAB_PAGE_TYPE_STATIC;
    size_t buf_size;

    { // Fill page attributes

      XFSPath full_path, basename;

      // Source path 
      x_fs_path(&full_path, path, dir_entry.name);
      page.source_path = (char*) x_arena_strdup(site_arena, full_path.buf);

      // Slug
      if (!page.slug)
      {
        x_fs_path(&basename, page.source_path);
        x_fs_path_stem(&basename, &basename);
        x_fs_path_basename(&basename, &basename);
        page.slug = (char*) x_arena_strdup(site_arena, basename.buf);
      }

      // Output path
      x_fs_path(&full_path, site->config.output_dir, page.slug);
      x_fs_path_change_extension(&full_path, ".html");
      page.output_path = (char*) x_arena_strdup(site_arena, full_path.buf);


      // URL
      {
        XSmallstr url;
        x_smallstr_format(&url, "%s.html", page.slug);
        page.url = (char*) x_arena_strdup(site_arena, url.buf);
      }


      // Date
      if (!page.date)
      {
        XSmallstr buf;
        XFSTime t = x_fs_time_from_epoch(dir_entry.last_modified);
        x_smallstr_format(&buf, "%d-%d-%d", t.year, t.month, t.day);
        page.date = x_arena_strdup(site_arena, buf.buf);
      }
    }

    // read source
    char* buf = x_io_read_text(page.source_path, &buf_size);
    if (! buf)
    {
      x_log_error("Failed to read from file %s\n", page.source_path);
      return 1;
    }

    x_log_info("Processing %s", page.source_path);

    SlabFrontmatter* meta = &page.meta;
    XSlice input = x_slice_init(buf, buf_size);
    SlabFrontmatterParseResult status = slab_frontammter_parse(&input, meta);
    if (status != SLAB_FRONTMATTER_SUCCESS)
    {
      x_log_error("Error %d parsing meta block: %s\n", dir_entry.name, status);
      continue;
    }

    for (u32 i = 0; i < meta->entry_count; i++)
    {
      XSlice* key = &meta->entry_key[i];
      XSlice* value = &meta->entry_value[i];
      s_slab_page_property_set(&page, site_arena, key, value);
    }

    if (!s_slab_site_add_page(site, &page, site_arena))
    {
      x_log_error("Failed to register page %s\n", dir_entry.name);
      continue;
    }
  }
  while (x_fs_find_next_file(handle, &dir_entry));
  return 0;
}

SlabFrontmatterParseResult slab_frontammter_parse(XSlice* input, SlabFrontmatter* out)
{
  if (!out)
    return false;

  const char* start = input->ptr;

  i32 line_nr = 0;
  XSlice token;
  i32 meta_marker = 0;

  out->result = SLAB_FRONTMATTER_SUCCESS;
  out->entry_count = 0;
  out->error_line = 0;
  out->past_meta_offset = 0;

  while (x_slice_next_token(input, '\n', &token))
  {
    XSlice line = x_slice_trim(token);

    // ignore empty lines
    if (x_slice_is_empty(line))
      continue;

    // ignore lines starting with #
    if (meta_marker == 1 && x_slice_starts_with_cstr(line, "#"))
      continue;

    // count meta delimiters
    if (x_slice_eq_cstr(line, "---"))
    {
      meta_marker++;

      // stop if we find a second delimiter
      if (meta_marker == 2)
      {
        out->past_meta_offset = (i32) ((line.ptr + line.length) - start);
        return out->result;
      }
      continue;
    }

    // stop if we find anything before a meta delimiter
    if (meta_marker == 0)
    {
      out->result = SLAB_FRONTMATTER_MISSING;
      out->error_line = line_nr;
      return out->result;
    }

    XSlice key, value;
    if (!x_slice_split_at(line, ':', &key, &value))
    {
      printf("Malformed meta entry at line %d\n", line_nr);
      printf("> %.*s\n", (i32) line.length, line.ptr);
      out->result = SLAB_FRONTMATTER_MALFORMED_ENTRY;
      out->error_line = line_nr;
      return out->result;
    }

    if ((out->entry_count + 1) >= (SLAB_FRONTMATTER_MAX_ENTRIES))
    {
      out->result = SLAB_FRONTMATTER_TOO_MANY_ENTRIES;
      out->error_line = line_nr;
      return out->result;
    }

    u32 index = out->entry_count++;
    out->entry_key[index] = x_slice_trim(key);
    out->entry_value[index] = x_slice_trim(value);
  }

  out->result = SLAB_FRONTMATTER_UNTERMINATED;
  out->error_line = line_nr;
  return out->result;
}

/**
 * Load site config
 */
bool slab_config_load(const char* site_root, SlabConfig* out_config)
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

  s = x_ini_get(&ini, "site", "output_dir", "out");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->output_dir, s);
  else
    x_fs_path(&out_config->output_dir, site_root, s);
  x_fs_path_normalize(&out_config->output_dir);

  s = x_ini_get(&ini, "site", "pages_dir", ".");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->pages_dir, s);
  else
    x_fs_path(&out_config->pages_dir, site_root, s);
  x_fs_path_normalize(&out_config->pages_dir);

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

  x_log_info("Site config:\n"
      "\tname = %s\n"
      "\turl = %s\n"
      "\tassets_dir = %s\n"
      "\toutput_dir = %s\n"
      "\tposts_dir = %s\n"
      "\tpages_dir = %s\n"
      "\ttemplate_dir = %s\n",
      out_config->site_name,
      out_config->site_url,
      x_fs_path_cstr(&out_config->assets_dir),
      x_fs_path_cstr(&out_config->output_dir),
      x_fs_path_cstr(&out_config->posts_dir),
      x_fs_path_cstr(&out_config->pages_dir),
      x_fs_path_cstr(&out_config->template_dir));
  return true;
}

/**
 * Unload site config
 */
void slab_config_unload(SlabConfig* config)
{
  if (!config)
    return;

  free(config->data);
  config->site_name     = NULL;
  config->site_url      = NULL;
  config->data          = NULL;

}

/**
 * Site creation
 */
SlabSite* slab_site_create(size_t arena_size, SlabConfig* config)
{
  if (arena_size <= 0)
    return NULL;

  // The main struct lives inside the actual arena
  XArena* arena = x_arena_create(1024 * 1024);
  if (!arena)
    return NULL;

  SlabSite* site = x_arena_alloc_zero(arena, sizeof(SlabSite));
  site->arena = arena;
  site->config = *config;
  return site;
}

/**
 * Site destruction
 */
void slab_site_destroy(SlabSite* site)
{
  if (!site)
    return;

  x_arena_destroy(site->arena);
}

