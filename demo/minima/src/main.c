#include <stdx_common.h>

#define X_IMPL_ARENA
#include <stdx_arena.h>

#define X_IMPL_IO
#include <stdx_io.h>

#define X_IMPL_STRING
#include <stdx_string.h>

#define X_IMPL_STRBUILDER
#include <stdx_strbuilder.h>

#define X_IMPL_HASHTABLE
#include <stdx_hashtable.h>

#define X_IMPL_TEST
#include <stdx_test.h>

#define X_IMPL_ARRAY
#include <stdx_array.h>

#include "mi_parser.h"
#include "mi_runtime.h"
#include "mi_builtins.h"

#include <stdio.h>
#include <stdlib.h>

static void print_usage(const char *exe)
{
  fprintf(stderr, "usage: %s <file.mi>\n", exe);
}

int main(int argc, char** argv)
{
  const char *filename;
  XArena *arena;
  XSlice source;
  MiParseResult parse_result;
  MiExecResult exec_result;
  MiContext ctx;
  size_t text_size;
  char *text;
  bool ok;

  if (argc != 2)
  {
    print_usage(argv[0]);
    return 1;
  }

  filename = argv[1];

  text_size = 0;
  text = x_io_read_text(filename, &text_size);

  if (!text)
  {
    fprintf(stderr, "error: failed to read file: %s\n", filename);
    return 1;
  }

  source.ptr = text;
  source.length = text_size;

  arena = x_arena_create(1024 * 8);

  if (!arena)
  {
    fprintf(stderr, "error: failed to create arena\n");
    free(text);
    return 1;
  }

  parse_result = mi_parse(arena, source);

  if (!parse_result.ok)
  {
    fprintf(
      stderr,
      "%s:%d:%d: %s\n",
      filename,
      parse_result.error_line,
      parse_result.error_column,
      parse_result.error_message ? parse_result.error_message : "parse error"
    );

    x_arena_destroy(arena);
    free(text);
    return 1;
  }

  ok = mi_context_init(&ctx, arena, NULL, NULL);

  if (!ok)
  {
    fprintf(stderr, "error: failed to initialize runtime\n");
    x_arena_destroy(arena);
    free(text);
    return 1;
  }

  if (! mi_register_builtins(&ctx))
  {
    fprintf(stderr, "error: failed to initialize builtin commands\n");
    x_arena_destroy(arena);
    free(text);
    return 1;
  }

  exec_result = mi_exec_block(&ctx, parse_result.root, false);

  if (exec_result.signal == MI_SIGNAL_ERROR)
  {
    fprintf(
      stderr,
      "%s:%d:%d: %s\n",
      filename,
      ctx.error_line,
      ctx.error_column,
      ctx.error_message ? ctx.error_message : "runtime error"
    );

    x_arena_destroy(arena);
    free(text);
    return 1;
  }

  if (exec_result.signal != MI_SIGNAL_NONE)
  {
    fprintf(stderr, "%s: unexpected control-flow signal at top level\n", filename);
    x_arena_destroy(arena);
    free(text);
    return 1;
  }

  x_arena_destroy(arena);
  free(text);
  return 0;
}
