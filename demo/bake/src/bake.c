/*
 * STDX – Bake tool
 *
 * Compile-time string and file interning tool.
 * Reads a .ini description and generates a C header with stable IDs
 * and embedded data.
 */

#include <stdx_common.h>
#define X_IMPL_STRING
#include <stdx_string.h>
#define X_IMPL_INI
#include <stdx_ini.h>
#define X_IMPL_IO
#include <stdx_io.h>
#define X_IMPL_FILESYSTEM
#include <stdx_filesystem.h>
#define X_IMPL_LOG
#include <stdx_log.h>

#define log_info(msg, ...)     x_log_raw(stdout, XLOG_LEVEL_INFO, XLOG_COLOR_WHITE, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define log_error(msg, ...)    x_log_raw(stderr, XLOG_LEVEL_INFO, XLOG_COLOR_RED, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void s_print_usage(const char* exe)
{
  log_info(
      "usage:\n"
      "  %s <input.ini> [-o out.h]\n"
      "\n"
      "The input INI file describes strings and files to bake into a C header.\n"
      "\n"
      "It accepts the following Top-level options (no section):\n"
      "  guard          = DATA_H            # Include guard macro name (required)\n"
      "  output         = out_file_name.h   # Optional; default is <ini filename>.h\n"
      "  no_enum        = 0|1               # If 0, do NOT emit enums for each [section].\n"
      "  no_strings     = 0|1               # If 0, do NOT emit const char* array for each [section].\n"
      "  no_crc         = 0|1               # If 0, do NOT compute CRC32 for embedded file contents.\n"
      "  bytes_per_line = N                 # Bytes per line for byte arrays (default: 8)\n"
      "  comment        = \"text...\"          # Comment emitted at the top of the generated header\n"
      "                                     # Supports \\n and \\t escapes\n"
      "\n"
      "Entries are grouped into [sections].\n"
      "IDs are assigned globally by order of appearance.\n\n"
      "Entry forms inside a [section]:\n"
      "  KEY = \"string\"      # Intern a string (quotes optional)\n"
      "  KEY = @path         # Embed file bytes (path is relative to the .ini file)\n"
      "  KEY = @@path        # Embed file bytes and append a null terminator. (path is relative to the .ini file)\n",
    exe);
}

static int s_is_ident_start(unsigned char c)
{
  return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/* sanitize + uppercase */
static u32 s_to_macro_name(char* dst, u32 dst_cap, const char* s)
{
  u32 w = 0;
  int prev_underscore = 0;

  if (!dst || dst_cap == 0)
  {
    return 0;
  }

  for (const unsigned char* p = (const unsigned char*)s; *p; p++)
  {
    unsigned char c = *p;

    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    {
      c = (unsigned char)toupper(c);
      prev_underscore = 0;
    }
    else if (c >= '0' && c <= '9')
    {
      prev_underscore = 0;
    }
    else if (c == '_')
    {
      if (prev_underscore)
      {
        continue;
      }
      prev_underscore = 1;
    }
    else
    {
      if (prev_underscore)
      {
        continue;
      }
      c = '_';
      prev_underscore = 1;
    }

    if (w + 1 >= dst_cap)
    {
      break;
    }

    dst[w++] = (char)c;
  }

  while (w > 0 && dst[w - 1] == '_')
  {
    w--;
  }

  dst[w] = 0;

  if (w == 0 || !s_is_ident_start((unsigned char)dst[0]))
  {
    const char* prefix = "TAG_";
    const u32 prefix_len = 4;

    if (prefix_len + w + 1 > dst_cap)
    {
      strncpy(dst, "TAG", dst_cap - 1);
      dst[dst_cap - 1] = 0;
      return (u32)strlen(dst);
    }

    memmove(dst + prefix_len, dst, w + 1);
    memcpy(dst, prefix, prefix_len);
    w += prefix_len;
  }

  return w;
}

static void s_emit_c_string(FILE* out, const char* s)
{
  fputc('\"', out);

  for (const unsigned char* p = (const unsigned char*)s; *p; p++)
  {
    unsigned char c = *p;

    if (c == '\\')
    {
      fputs("\\\\", out);
    }
    else if (c == '\"')
    {
      fputs("\\\"", out);
    }
    else if (c == '\n')
    {
      fputs("\\n", out);
    }
    else if (c == '\r')
    {
      fputs("\\r", out);
    }
    else if (c == '\t')
    {
      fputs("\\t", out);
    }
    else if (c < 32 || c == 127)
    {
      fprintf(out, "\\x%02X", (unsigned int)c);
    }
    else
    {
      fputc((int)c, out);
    }
  }

  fputc('\"', out);
}

static char s_x_unescape_char(char c)
{
  switch (c)
  {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case '\\': return '\\';
    case '"': return '"';
    case '0': return '\0';
    default: return 0;
  }
}

static void s_emit_block_comment(FILE* out, const char* comment)
{
  if (!comment || comment[0] == 0)
  {
    return;
  }

  /* Worst-case, unescaped string is <= original length */
  size_t in_len = strlen(comment);
  char* buf = (char*)malloc(in_len + 1);
  if (!buf)
  {
    /* If OOM, just emit the raw comment */
    fprintf(out, "/* %s */\n\n", comment);
    return;
  }

  size_t wi = 0;
  for (size_t i = 0; i < in_len; i++)
  {
    char c = comment[i];

    if (c == '\\' && i + 1 < in_len)
    {
      char next = comment[i + 1];
      char rep = s_x_unescape_char(next);
      if (rep != 0)
      {
        buf[wi++] = rep;
        i++; /* consume escape char */
        continue;
      }

      /* Unknown escape: keep the backslash literally */
      buf[wi++] = c;
      continue;
    }

    buf[wi++] = c;
  }

  buf[wi] = 0;

  fprintf(out, "/* %s */\n\n", buf);

  free(buf);
}

static u32 s_count_total_items(const XIni* ini)
{
  u32 total = 0;

  int scount = x_ini_section_count(ini);
  for (int si = 0; si < scount; si++)
  {
    const char* section = x_ini_section_name(ini, si);
    if (!section || section[0] == 0)
    {
      continue;
    }

    int kcount = x_ini_key_count(ini, si);
    if (kcount > 0)
    {
      total += (u32)kcount;
    }
  }

  return total;
}

static u32 s_crc32_update(u32 crc, const u8* data, size_t size)
{
  u32 c = crc ^ 0xFFFFFFFFu;

  for (u32 i = 0; i < size; i++)
  {
    c ^= (u32)data[i];
    for (u32 b = 0; b < 8; b++)
    {
      u32 mask = (u32)-(int)(c & 1u);
      c = (c >> 1) ^ (0xEDB88320u & mask);
    }
  }

  return c ^ 0xFFFFFFFFu;
}

static void s_emit_baked_bytes(FILE* out, const char* base, const u8* data, size_t size, int null_terminate, u32 bytes_per_line)
{
  fprintf(out, "static const unsigned char %s_BYTES[] =\n{\n", base);

  u32 emitted = 0;
  size_t total = size + (null_terminate ? 1u : 0u);

  while (emitted < total)
  {
    fprintf(out, "  ");

    u32 line_count = 0;
    while (line_count < bytes_per_line && emitted < total)
    {
      u8 b = 0;
      if (emitted < size)
      {
        b = data[emitted];
      }
      else
      {
        b = 0;
      }

      fprintf(out, "0x%02Xu", (unsigned int)b);

      emitted++;
      line_count++;

      if (emitted < total)
      {
        fprintf(out, ", ");
      }
    }

    fprintf(out, "\n");
  }

  fprintf(out, "};\n");
}

static int s_parse_args(int argc, char** argv, const char** out_ini_path, const char** out_out_override)
{
  const char* ini_path = NULL;
  const char* out_override = NULL;

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-o") == 0)
    {
      if (i + 1 >= argc)
      {
        s_print_usage(argv[0]);
        return 0;
      }
      out_override = argv[++i];
    }
    else if (argv[i][0] == '-')
    {
      s_print_usage(argv[0]);
      return 0;
    }
    else
    {
      ini_path = argv[i];
    }
  }

  if (!ini_path)
  {
    s_print_usage(argv[0]);
    return 0;
  }

  *out_ini_path = ini_path;
  *out_out_override = out_override;
  return 1;
}

static int s_run(const char* ini_path, const char* out_override)
{
  XIni ini;
  XIniError err;

  if (!x_ini_load_file(ini_path, &ini, &err))
  {
    log_error("error: failed to load ini: %s (%s:%d:%d)\n",
        x_ini_err_str(err.code), ini_path, err.line, err.column);
    return 1;
  }

  const char* include_guard = x_ini_get(&ini, "", "guard", "DATA_H");
  const char* output_file = x_ini_get(&ini, "", "output", NULL);
  const char* comment = x_ini_get(&ini, "", "comment", "DO NOT EDIT THIS FILE");
  int emit_enum = x_ini_get_bool(&ini, "", "no_enum", false) ? 1 : 0;
  int emit_string_array = x_ini_get_bool(&ini, "", "no_strings", false) ? 1 : 0;
  int bake_crc = x_ini_get_bool(&ini, "", "no_crc", false) ? 1 : 0;
  int bake_bytes_line_size_i = x_ini_get_i32(&ini, "", "bytes_per_line", 8);
  u32 bake_bytes_line_size = 8u;

  if (bake_bytes_line_size_i > 0)
  {
    bake_bytes_line_size = (u32)bake_bytes_line_size_i;
  }

  if (bake_bytes_line_size > 64u)
  {
    bake_bytes_line_size = 64u;
  }

  if (!include_guard || include_guard[0] == 0)
  {
    log_error("error: missing global key: guard\n", 0);
    x_ini_free(&ini);
    return 1;
  }

  /* Resolve output path */
  XFSPath out_path = {0};

  if (out_override && out_override[0])
  {
    x_fs_path(&out_path, out_override);
  }
  else if (output_file && output_file[0])
  {
    x_fs_path(&out_path, output_file);
  }
  else
  {
    x_fs_path_join_slice(&out_path,
        x_fs_path_dirname(ini_path),
        x_fs_path_basename(ini_path));
    x_fs_path_change_extension(&out_path, ".h");
    printf("%s\n", out_path.buf);
  }

  FILE* out = fopen(out_path.buf, "wb");
  if (!out)
  {
    log_error("error: failed to open output file: %s\n", out_path);
    x_ini_free(&ini);
    return 1;
  }

  s_emit_block_comment(out, comment);

  fprintf(out, "#ifndef %s\n#define %s\n\n", include_guard, include_guard);

  u32 total_count = s_count_total_items(&ini);
  fprintf(out, "#define %s_COUNT %uu\n\n", include_guard, (unsigned int) total_count);

  /* ini dir for @paths */
  XFSPath ini_dir;
  x_fs_path_from_slice(x_fs_path_dirname(ini_path), &ini_dir);

  // if this is true, the ini file is justa file name, no path.
  if (x_fs_path_is_file(&ini_dir))
    x_fs_path(&ini_dir, ".");

  u32 global_id = 0;
  int scount = x_ini_section_count(&ini);
  for (int si = 0; si < scount; si++)
  {
    const char* section = x_ini_section_name(&ini, si);
    if (!section || section[0] == 0)
    {
      continue;
    }

    int kcount = x_ini_key_count(&ini, si);
    if (kcount <= 0)
    {
      continue;
    }

    char sec_name[256];
    s_to_macro_name(sec_name, (u32)sizeof(sec_name), section);

    u32 section_base_id = global_id;

    /* emit entry macros */
    for (int ki = 0; ki < kcount; ki++)
    {
      const char* key = x_ini_key_name(&ini, si, ki);
      const char* val = x_ini_value_at(&ini, si, ki);

      if (!key || key[0] == 0)
      {
        continue;
      }
      if (!val)
      {
        val = "";
      }

      char key_name[256];
      s_to_macro_name(key_name, (u32)sizeof(key_name), key);

      char base[600];
      snprintf(base, sizeof(base), "%s_%s", sec_name, key_name);

      fprintf(out, "#define %s %uu\n", base, (unsigned int)global_id);

      if (val[0] == '@')
      {
        int null_terminate = 0;
        const char* rel = val + 1;

        if (val[1] == '@')
        {
          null_terminate = 1;
          rel = val + 2;
        }

        while (*rel == ' ' || *rel == '\t')
        {
          rel++;
        }

        u32 path_hash = x_cstr_hash(rel);

        fprintf(out, "#define %s_PATH ", base);
        s_emit_c_string(out, rel);
        fprintf(out, "\n");

        /* For baked files, _HASH always refers to the file path */
        fprintf(out, "#define %s_HASH 0x%08Xu\n", base, (unsigned int)path_hash);

        XFSPath full_path;
        x_fs_path(&full_path, ini_dir.buf, rel);

        const u8* file_data = NULL;
        size_t file_size = 0;

        file_data = (const u8*) x_io_read_text(full_path.buf, &file_size);

        if (!file_data)
        {
          log_error("error: failed to read file for %s: %s\n", base, full_path);
          fclose(out);
          x_ini_free(&ini);
          return 1;
        }

        size_t baked_size = file_size + (null_terminate ? 1u : 0u);

        fprintf(out, "#define %s_FILE_SIZE %uu\n", base, (unsigned int)file_size);
        fprintf(out, "#define %s_SIZE %uu\n", base, (unsigned int)baked_size);
        fprintf(out, "#define %s_LEN %uu\n\n", base, (unsigned int)baked_size);

        s_emit_baked_bytes(out, base, file_data, file_size, null_terminate, bake_bytes_line_size);

        fprintf(out, "#define %s_STR ((const char*)%s_BYTES)\n", base, base);

        if (null_terminate)
        {
          /* Convenience alias for explicit "this is a C-string" use */
          fprintf(out, "#define %s_CSTR ((const char*)%s_BYTES)\n", base, base);
        }

        if (bake_crc)
        {
          u32 crc = s_crc32_update(0u, file_data, file_size);
          if (null_terminate)
          {
            u8 z = 0;
            crc = s_crc32_update(crc, &z, 1);
          }
          fprintf(out, "#define %s_CRC 0x%08Xu\n", base, (unsigned int)crc);
        }

        fprintf(out, "\n");

        free((void*) file_data);
      }
      else
      {
        u32 len = (u32)strlen(val);
        u32 hash = x_cstr_hash(val);

        fprintf(out, "#define %s_STR ", base);
        s_emit_c_string(out, val);
        fprintf(out, "\n");
        fprintf(out, "#define %s_LEN %uu\n", base, (unsigned int)len);
        fprintf(out, "#define %s_HASH 0x%08Xu\n\n", base, (unsigned int)hash);
      }

      global_id++;
    }

    /* per-section string array */
    if (emit_string_array)
    {
      fprintf(out, "static const char* %s_ARR[] =\n{\n", sec_name);

      for (int ki = 0; ki < kcount; ki++)
      {
        const char* key = x_ini_key_name(&ini, si, ki);
        if (!key || key[0] == 0)
        {
          continue;
        }

        char key_name[256];
        s_to_macro_name(key_name, (u32)sizeof(key_name), key);

        char base[600];
        snprintf(base, sizeof(base), "%s_%s", sec_name, key_name);

        u32 id = section_base_id + (u32)ki;
        fprintf(out, "  %s_STR,  /* %uu */\n", base, (unsigned int)id);
      }

      fprintf(out, "};\n\n");
    }

    /* per-section enum */
    if (emit_enum)
    {
      fprintf(out, "typedef enum\n{\n");

      for (int ki = 0; ki < kcount; ki++)
      {
        const char* key = x_ini_key_name(&ini, si, ki);
        if (!key || key[0] == 0)
        {
          continue;
        }

        char key_name[256];
        s_to_macro_name(key_name, (u32)sizeof(key_name), key);

        u32 id = section_base_id + (u32)ki;
        fprintf(out, "  %s_E_%s = %uu,\n", sec_name, key_name, (unsigned int)id);
      }

      fprintf(out, "  %s_E_COUNT = %uu\n", sec_name, (unsigned int)kcount);
      fprintf(out, "} %s;\n\n", sec_name);
    }
  }

  fprintf(out, "#endif /* %s */\n", include_guard);

  fclose(out);
  x_ini_free(&ini);

  return 0;
}

int main(int argc, char** argv)
{
  const char* ini_path = NULL;
  const char* out_override = NULL;

  if (!s_parse_args(argc, argv, &ini_path, &out_override))
  {
    return 1;
  }

  return s_run(ini_path, out_override);
}
