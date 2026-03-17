#ifndef SLAB_H
#define SLAB_H

#include <stdx_common.h>
#include <stdx_arena.h>
#include <stdx_string.h>
#include <stdx_filesystem.h>

#include <mi_parser.h>
#include <mi_runtime.h>
#include <mi_builtins.h>

#define SLAB_FRONTMATTER_MAX_ENTRIES 32

typedef enum SlabFrontmatterParseResult
{
  SLAB_FRONTMATTER_SUCCESS          = 0,
  SLAB_FRONTMATTER_UNTERMINATED     = 1,
  SLAB_FRONTMATTER_MALFORMED_ENTRY  = 2,
  SLAB_FRONTMATTER_MISSING          = 3,
  SLAB_FRONTMATTER_TOO_MANY_ENTRIES = 4,
} SlabFrontmatterParseResult;

typedef struct SlabFrontmatter 
{
  SlabFrontmatterParseResult result;
  u32 error_line;
  u32 entry_count;
  u32 past_meta_offset;
  XSlice entry_key[SLAB_FRONTMATTER_MAX_ENTRIES];
  XSlice entry_value[SLAB_FRONTMATTER_MAX_ENTRIES];
} SlabFrontmatter;

typedef struct SlabConfig
{
  const char* site_url;
  const char* site_name;
  XFSPath output_dir;
  XFSPath content_dir;
  XFSPath template_dir;
  void* data;
} SlabConfig;

typedef enum SlabPageType
{
  SLAB_PAGE_TYPE_STATIC = 0,
  SLAB_PAGE_TYPE_POST   = 1,
}
SlabPageType;

typedef struct SlabPage
{
  char* source_path;    // full path to source file
  char* output_path;    // where to write final HTML, optional for now
  char* url;            // Page url
  SlabPageType type;          // post / static / unknown
 
  char* title;
  char* template_name;
  char* date;
  i32 year;
  i32 month;
  i32 day;
  char* author;
  char* tags;
  char* category;
  char* slug;
  SlabFrontmatter meta;              // Raw parsed meta block
  bool draft;
} SlabPage;

typedef struct SlabSite
{
  XArena*   arena;      // this arena provides memory for everything necessary
                        // during site metadata collection and config loading.
  SlabPage* pages;
  size_t    page_count;
  size_t    page_capacity;
  SlabConfig config;
}
SlabSite;

SlabSite* slab_site_create(size_t arena_size, SlabConfig* config);                        // Create a Site
void slab_site_destroy(SlabSite* site);                                                   // Destroy the Site
bool slab_config_load(const char* site_root, SlabConfig* out_config);                     // Load configuration from site root
void slab_config_unload(SlabConfig* config);                                              // Unload configuration
i32 slab_process_directory_metadata(SlabSite* site, const char* path);                    // Processes pages/posts from a directory
SlabFrontmatterParseResult slab_frontammter_parse(XSlice* input, SlabFrontmatter* out);   // Parses front matter from start of buffer

//
// Template 2 minima code expansion
//

typedef struct SlabTemplateResult
{
  i32       error_code;    // 0 = sucess, 1 = error
  i32       error_line;    // error line
  i32       error_column;  // error column
  XSmallstr error_message; // error message
} SlabTemplateResult;

SlabTemplateResult slab_expand_minima_template(XSlice input, XStrBuilder* out);

//
// Custom minima commands registration
//

MI_IMPORT_TYPE(Page);
bool slab_register_mi_commands(MiContext *ctx);

#endif // SLAB_H
