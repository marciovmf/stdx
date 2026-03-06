#include <stdx_common.h>
#define X_IMPL_IO
#include <stdx_io.h>
#define X_IMPL_LOG
#include <stdx_log.h>
#define X_IMPL_STRING
#define X_IMPL_STRBUILDER
#define MD_IMPL
#include "markdown.h"

#define log_info(msg, ...)     x_log_raw(stdout, XLOG_LEVEL_INFO, XLOG_COLOR_WHITE, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define log_error(msg, ...)    x_log_raw(stderr, XLOG_LEVEL_INFO, XLOG_COLOR_RED, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    log_info("usage:\n md2html <input> <output>\n", 0);
    return 1;
  }

  const char* in_file = argv[1];
  const char* out_file = argv[2];

  size_t buf_size;
  const char* buf = x_io_read_text(in_file, &buf_size);
  if (!buf)
  {
    log_error("Failed to read from file '%s'\n", in_file);
    return 1;
  }

  char* html = md_to_html(buf, buf_size);
  if (! x_io_write_text(out_file, html))
  {
    log_error("Failed to write to file '%s'\n", out_file);
    return 1;
  }

  md_free(html);
  return 0;
}
