#include <stdx_common.h>
#define X_IMPL_ARENA
#include <stdx_arena.h>
#define X_IMPL_STRING
#include <stdx_string.h>
#define X_IMPL_STRBUILDER
#include <stdx_strbuilder.h>
#define X_IMPL_IO
#include <stdx_io.h>

#include "tcl.h"
#include "tcl_expr.h"

static XSlice tcl_cmd_puts(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  XSlice line = tcl_eval_words_join(C, 1, E, A);
  fwrite(line.ptr ? line.ptr : "", 1, line.length, stdout);
  fputc('\n', stdout);
  return (XSlice){NULL, 0};
}

static XSlice tcl_cmd_incr(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  if (C->word_count < 2)
    //return sv_from_cstr(A, "0");
    return x_slice_from_cstr(x_arena_strdup(A, "0"));
  XSlice name = tcl_eval_word(C->words[1], E, A);
  XSlice curv = tcl_env_get(E, name);
  double base = 0.0;
  if (curv.ptr)
    base = strtod(curv.ptr, NULL);
  double inc = 1.0;
  if (C->word_count >= 3)
  {
    XSlice iv = tcl_eval_word(C->words[2], E, A);
    inc = strtod(iv.ptr ? iv.ptr : "0", NULL);
  }
  double nv = base + inc;
  char buf[64];
  int m = snprintf(buf, sizeof(buf), "%.15g", nv);
  XSlice newv = x_slice_from_cstr(x_arena_slicedup(A, buf, (size_t)m, true));
  tcl_env_set(E, name, newv);
  return newv;
}

static XSlice tcl_cmd_set(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  if (C->word_count < 2)
    return x_slice_from_cstr(x_arena_strdup(A, ""));
  //return sv_from_cstr(A, "");
  XSlice name = tcl_eval_word(C->words[1], E, A);
  if (C->word_count == 2)
  {
    return tcl_env_get(E, name);
  }
  XSlice value = tcl_eval_word(C->words[2], E, A);
  return tcl_env_set(E, name, value);
}

int main(int argc, char** argv)
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
  tcl_command_register(&env, "set",   tcl_cmd_set);
  tcl_command_register(&env, "puts",  tcl_cmd_puts);
  tcl_command_register(&env, "incr",  tcl_cmd_incr);
  tcl_command_register(&env, "expr",  tcl_cmd_expr);
  
  (void)tcl_eval_script(pr.script, &env, arena);


  free(src);
  x_arena_destroy(arena);
  return 0;
}
