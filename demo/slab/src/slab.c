#include <stdx_common.h>
#include <stdx_string.h>
#include <stdx_filesystem.h>
#include <stdx_ini.h>
#include <stdx_io.h>
#include <stdx_log.h>

#include "slab.h"
#include "stdx_arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


SlabFrontmatterParseResult slab_frontammter_parse(XSlice* input, SlabFrontmatter* out);

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
    log_warning("Date format '%s': year must have 4 digits, using 1900\n", s);
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
    log_warning("Date format '%s': month out of range (%d), using 1\n", s, m);
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
    log_warning("Date format '%s': day out of range (%d), using 1\n", s, d);
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

  if (MATCH("title"))
  {
    page->title = copied;
  }
  else if (MATCH("slug"))
  {
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

static i32 s_slab_site_add_category(SlabSite* site, const SlabCategory* category, XArena* arena)
{
  if (!site || !category)
  {
    return 0;
  }

  if (site->category_count == site->category_capacity)
  {
    size_t new_cap;
    size_t bytes;
    SlabCategory* new_categories;

    new_cap = (site->category_capacity == 0) ? 16 : site->category_capacity * 2;
    bytes = new_cap * sizeof(SlabCategory);

    new_categories = (SlabCategory*)x_arena_alloc(arena, bytes);
    if (!new_categories)
    {
      return 0;
    }

    if (site->categories && site->category_count > 0)
    {
      memcpy(new_categories, site->categories, site->category_count * sizeof(SlabCategory));
    }

    site->categories = new_categories;
    site->category_capacity = new_cap;
  }

  {
    SlabCategory* dest = &site->categories[site->category_count++];
    *dest = *category;
  }

  return 1;
}

static bool s_slab_site_has_category(const SlabSite* site, const char* name)
{

  if (!site || !name || !name[0])
  {
    return false;
  }

  size_t name_len = strlen(name);
  for (u32 i = 0; i < site->category_count; i++)
  {
    const SlabCategory* category = &site->categories[i];
    if (category->name && strncmp(category->name, name, name_len) == 0)
    {
      return true;
    }
  }

  return false;
}

static i32 s_slab_collect_categories(SlabSite* site)
{
  size_t i;

  if (!site)
  {
    return 1;
  }

  site->categories = NULL;
  site->category_count = 0;
  site->category_capacity = 0;

  for (i = 0; i < site->page_count; i++)
  {
    SlabPage* page = &site->pages[i];
    SlabCategory category = {0};

    if (!page->category || !page->category[0])
    {
      continue;
    }

    if (s_slab_site_has_category(site, page->category))
    {
      continue;
    }

    category.name = page->category;

    if (!s_slab_site_add_category(site, &category, site->arena))
    {
      return 1;
    }
  }

  return 0;
}

//
// SLAB Public API
//

static bool s_slab_is_processable_content_file(const char* name)
{
  if (x_cstr_ends_with(name, ".md"))
  {
    return true;
  }

  if (x_cstr_ends_with(name, ".htm"))
  {
    return true;
  }

  if (x_cstr_ends_with(name, ".html"))
  {
    return true;
  }

  return false;
}

static void s_slab_path_to_url_separators(XFSPath* path)
{
  char* s = path->buf;

  while (*s)
  {
    if (*s == '\\')
    {
      *s = '/';
    }

    s++;
  }
}

static bool s_slab_copy_file_to_output(
    SlabSite* site,
    const char* root_path,
    const char* full_path
    )
{
  XFSPath relative_path;
  XFSPath output_path;
  XFSPath output_dir;

  if (!x_fs_path_relative_to_cstr(root_path, full_path, &relative_path))
  {
    log_error("[FAIL] copy %s - Failed to compute relative path\n", full_path);
    return false;
  }

  x_fs_path_normalize(&relative_path);
  x_fs_path(&output_path, site->config.output_dir, relative_path.buf);
  x_fs_path_dirname(&output_path, &output_dir);

  if (!x_fs_directory_create_recursive(output_dir.buf))
  {
    log_error("[FAIL] copy %s -> Failed to create output directory %s\n",
        full_path,
        output_dir.buf);
    return false;
  }

  if (!x_fs_file_copy(full_path, output_path.buf))
  {
    log_error("[FAIL] copy %s -> Failed to copy to %s\n", full_path, output_path.buf);
    return false;
  }

  log_info("[ OK ] copy %s -> %s\n", full_path, output_path.buf);
  return true;
}

static bool s_slab_build_page_defaults(
    SlabSite* site,
    SlabPage* page,
    const char* root_path,
    const char* full_path,
    const XFSDireEntry* dir_entry
    )
{
  XArena* site_arena = site->arena;
  XFSPath relative_path;
  XFSPath slug_path;
  XFSPath output_path;
  XSmallstr date_buf;

  page->category = "";
  page->source_path = (char*)x_arena_strdup(site_arena, full_path);

  if (!x_fs_path_relative_to_cstr(root_path, full_path, &relative_path))
  {
    log_error("Failed to compute relative path for %s\n", full_path);
    return false;
  }

  x_fs_path_normalize(&relative_path);

  if (!page->slug)
  {

    x_fs_path(&slug_path, &relative_path);
    x_fs_path_change_extension(&slug_path, "");
    s_slab_path_to_url_separators(&slug_path);
    page->slug = (char*)x_arena_strdup(site_arena, slug_path.buf);
  }

  x_fs_path(&output_path, site->config.output_dir, page->slug);
  x_fs_path_change_extension(&output_path, "html");
  page->output_path = (char*)x_arena_strdup(site_arena, output_path.buf);

  {
    XSmallstr url;
    x_smallstr_format(&url, "%s.html", page->slug);
    page->url = (char*)x_arena_strdup(site_arena, url.buf);
  }

  if (!page->date)
  {
    XFSTime t = x_fs_time_from_epoch(dir_entry->last_modified);
    x_smallstr_format(&date_buf, "%04d-%02d-%02d", t.year, t.month, t.day);
    page->date = (char*)x_arena_strdup(site_arena, date_buf.buf);
  }

  return true;
}

static i32 s_slab_process_content_file(
    SlabSite* site,
    const char* root_path,
    const char* current_path,
    const XFSDireEntry* dir_entry
    )
{
  XArena* site_arena = site->arena;
  XFSPath full_path;
  SlabPage page = {0};
  char* buf;
  size_t buf_size;
  XSlice input;
  SlabFrontmatter* meta;
  SlabFrontmatterParseResult status;
  u32 i;

  x_fs_path(&full_path, current_path, dir_entry->name);
  x_fs_path_normalize(&full_path);

  if (!s_slab_is_processable_content_file(dir_entry->name))
  {
    return s_slab_copy_file_to_output(site, root_path, full_path.buf) ? 0 : 1;
  }

  if (!s_slab_build_page_defaults(site, &page, root_path, full_path.buf, dir_entry))
  {
    return 1;
  }

  buf = x_io_read_text(page.source_path, &buf_size);
  if (!buf)
  {
    log_error("Failed to read file %s\n", page.source_path);
    return 1;
  }

  meta = &page.meta;
  input = x_slice_init(buf, buf_size);
  status = slab_frontammter_parse(&input, meta);

  if (status == SLAB_FRONTMATTER_MISSING)
  {
    return s_slab_copy_file_to_output(site, root_path, full_path.buf) ? 0 : 1;
  }

  if (status != SLAB_FRONTMATTER_SUCCESS)
  {
    log_error("Error parsing meta block in %s: %d\n", dir_entry->name, status);
    return 1;
  }

  for (i = 0; i < meta->entry_count; i++)
  {
    XSlice* key = &meta->entry_key[i];
    XSlice* value = &meta->entry_value[i];
    s_slab_page_property_set(&page, site_arena, key, value);
  }

  {
    XFSPath output_path;

    x_fs_path(&output_path, site->config.output_dir, page.slug);
    x_fs_path_change_extension(&output_path, "html");

    page.output_path = (char*)x_arena_strdup(site_arena, output_path.buf);

    {
      XSmallstr url;
      x_smallstr_format(&url, "%s.html", page.slug);
      page.url = (char*)x_arena_strdup(site_arena, url.buf);
    }
  }

  {
    XFSPath page_dir;

    x_fs_path(&page_dir, page.output_path);
    x_fs_path_dirname(&page_dir, &page_dir);

    if (!x_fs_directory_create_recursive(page_dir.buf))
    {
      log_error("Failed to create page output directory %s\n", page_dir.buf);
      return 1;
    }
  }

  if (!s_slab_site_add_page(site, &page, site_arena))
  {
    log_error("Failed to register page %s\n", dir_entry->name);
    return 1;
  }

  log_info("[ OK ] parse %s -> %s\n", page.source_path, page.output_path);
  return 0;
}

static i32 s_slab_process_directory_metadata_recursive(SlabSite* site, const char* root_path, const char* current_path)
{
  XFSDireEntry dir_entry;
  XFSDireHandle* handle;
  i32 result = 0;

  handle = x_fs_find_first_file(current_path, &dir_entry);
  if (!handle)
  {
    log_error("Failed to scan directory %s\n", current_path);
    return 1;
  }

  do
  {
    if (strncmp(dir_entry.name, ".", 1) == 0 || strncmp(dir_entry.name, "..", 2) == 0 ||
        x_cstr_starts_with(dir_entry.name, "_") // We ignore anything that starts with '_'
       ) 
    {
      continue;
    }

    if (dir_entry.is_directory)
    {
      XFSPath child_path;

      x_fs_path(&child_path, current_path, dir_entry.name);
      x_fs_path_normalize(&child_path);

      if (s_slab_process_directory_metadata_recursive(site, root_path, child_path.buf) != 0)
      {
        result = 1;
      }

      continue;
    }

    if (s_slab_process_content_file(site, root_path, current_path, &dir_entry) != 0)
    {
      result = 1;
    }
  }
  while (x_fs_find_next_file(handle, &dir_entry));

  x_fs_find_close(handle);
  return result;
}

static i32 slab_process_directory_metadata(SlabSite* site, const char* path)
{
  return s_slab_process_directory_metadata_recursive(site, path, path);
}

i32 slab_process_site(SlabSite* site)
{
  i32 result = 0;
  result |= slab_process_directory_metadata(site, x_fs_path_cstr(&site->config.template_dir));
  result |= slab_process_directory_metadata(site, x_fs_path_cstr(&site->config.content_dir));
  result |= s_slab_collect_categories(site);
  return result;
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
  x_fs_path(&site_config_file, site_root, "_site.ini");
  const char* ini_file_path = x_smallstr_cstr(&site_config_file);
  log_info("Loading ini file: %s\n", ini_file_path);

  size_t file_size;
  char *data = x_io_read_text(ini_file_path, &file_size);
  if (!data)
  {
    log_error("Unable to load site configuration file '%s'\n", ini_file_path);
    return false;
  }

  XIni ini;
  XIniError iniError;
  if (! x_ini_load_mem(data, strlen(data), &ini, &iniError))
  {
    log_error("Unable to parse site configuration file '%s'\n", ini_file_path);
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

  s = x_ini_get(&ini, "site", "content_dir", ".");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->content_dir, s);
  else
    x_fs_path(&out_config->content_dir, site_root, s);
  x_fs_path_normalize(&out_config->content_dir);

  s = x_ini_get(&ini, "site", "template_dir", "");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->template_dir, s);
  else
    x_fs_path(&out_config->template_dir, site_root, s);
  x_fs_path_normalize(&out_config->template_dir);

  log_info("Site config:\n"
      "\tname = %s\n"
      "\turl = %s\n"
      "\toutput_dir = %s\n"
      "\tcontent_dir = %s\n"
      "\ttemplate_dir = %s\n",
      out_config->site_name,
      out_config->site_url,
      x_fs_path_cstr(&out_config->output_dir),
      x_fs_path_cstr(&out_config->content_dir),
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

