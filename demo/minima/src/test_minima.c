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

#define X_IMPL_FILESYSTEM
#include <stdx_filesystem.h>

#define X_IMPL_ARRAY
#include <stdx_array.h>

#define X_IMPL_TEST
#include <stdx_test.h>

#include "minima.h"

#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE (1024 * 8)
static void print_usage(const char *exe)
{
  fprintf(stderr, "usage: %s <file.mi>\n", exe);
}

static int test_mi_value_constructors(void)
{
  MiValue v0;
  MiValue v1;
  MiValue v2;

  v0 = mi_value_null();
  v1 = mi_value_string(x_slice_from_cstr("hello"));
  v2 = mi_value_raw(x_slice_from_cstr("foo"));

  ASSERT_TRUE(v0.kind == MI_VAL_NULL);
  ASSERT_TRUE(v1.kind == MI_VAL_STRING);
  ASSERT_TRUE(v2.kind == MI_VAL_RAW);
  ASSERT_TRUE(x_slice_cmp(v1.as.string, x_slice_from_cstr("hello")) == 0);

  return 0;
}

static int test_mi_scope_define_and_get(void)
{
  XArena* arena;
  MiScope *scope;
  MiValue value;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);
  scope = mi_scope_create(arena, NULL);
  ASSERT_TRUE(scope != NULL);

  ok = mi_scope_define(scope, x_slice_from_cstr("x"), mi_value_string(x_slice_from_cstr("10")));
  ASSERT_TRUE(ok);

  ok = mi_scope_get(scope, x_slice_from_cstr("x"), &value);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(value.kind == MI_VAL_STRING);
  ASSERT_TRUE(x_slice_cmp(value.as.string, x_slice_from_cstr("10")) == 0);

  x_arena_destroy(arena);
  return 0;
}

static int test_mi_scope_get_walks_parent_chain(void)
{
  XArena* arena;
  MiScope *global_scope;
  MiScope *child;
  MiValue value;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  global_scope = mi_scope_create(arena, NULL);
  child = mi_scope_create(arena, global_scope);

  ASSERT_TRUE(global_scope != NULL);
  ASSERT_TRUE(child != NULL);

  ok = mi_scope_define(global_scope, x_slice_from_cstr("name"), mi_value_string(x_slice_from_cstr("Marcio")));
  ASSERT_TRUE(ok);

  ok = mi_scope_get(child, x_slice_from_cstr("name"), &value);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(value.kind == MI_VAL_STRING);
  ASSERT_TRUE(x_slice_cmp(value.as.string, x_slice_from_cstr("Marcio")) == 0);

  x_arena_destroy(arena);
  return 0;
}

static int test_mi_scope_set_updates_nearest_existing_binding(void)
{
  XArena* arena;
  MiScope *global_scope;
  MiScope *child;
  MiValue value;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  global_scope = mi_scope_create(arena, NULL);
  child = mi_scope_create(arena, global_scope);

  ASSERT_TRUE(global_scope != NULL);
  ASSERT_TRUE(child != NULL);

  ok = mi_scope_define(global_scope, x_slice_from_cstr("x"), mi_value_string(x_slice_from_cstr("1")));
  ASSERT_TRUE(ok);

  ok = mi_scope_set(child, x_slice_from_cstr("x"), mi_value_string(x_slice_from_cstr("2")));
  ASSERT_TRUE(ok);

  ok = mi_scope_get(global_scope, x_slice_from_cstr("x"), &value);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(x_slice_cmp(value.as.string, x_slice_from_cstr("2")) == 0);

  x_arena_destroy(arena);
  return 0;
}

static int test_mi_scope_set_creates_in_current_scope_when_missing(void)
{
  XArena* arena;
  MiScope *global_scope;
  MiScope *child;
  MiValue value;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  global_scope = mi_scope_create(arena, NULL);
  child = mi_scope_create(arena, global_scope);

  ASSERT_TRUE(global_scope != NULL);
  ASSERT_TRUE(child != NULL);

  ok = mi_scope_set(child, x_slice_from_cstr("y"), mi_value_string(x_slice_from_cstr("20")));
  ASSERT_TRUE(ok);

  ok = mi_scope_get(child, x_slice_from_cstr("y"), &value);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(x_slice_cmp(value.as.string, x_slice_from_cstr("20")) == 0);

  ok = mi_scope_get(global_scope, x_slice_from_cstr("y"), &value);
  ASSERT_TRUE(ok == false);

  x_arena_destroy(arena);
  return 0;
}

static int test_mi_context_init(void)
{
  XArena* arena;
  MiContext ctx;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  ok = mi_context_init(&ctx, arena, NULL, NULL);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(ctx.global_scope != NULL);
  ASSERT_TRUE(ctx.current_scope == ctx.global_scope);
  ASSERT_TRUE(ctx.commands != NULL);
  ASSERT_TRUE(ctx.funcs != NULL);
  ASSERT_TRUE(ctx.output != NULL);

  x_arena_destroy(arena);
  return 0;
}

static MiExecResult test_cmd_noop(MiContext *ctx, int argc, MiNode **argv)
{
  (void)ctx;
  (void)argc;
  (void)argv;
  return mi_exec_null();
}

static int test_mi_register_and_lookup_command(void)
{
  XArena* arena;
  MiContext ctx;
  MiCommandFn fn;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  ok = mi_context_init(&ctx, arena, NULL, NULL);
  ASSERT_TRUE(ok);

  ok = mi_register_command(&ctx, "noop", test_cmd_noop);
  ASSERT_TRUE(ok);

  ok = mi_lookup_command(&ctx, x_slice_from_cstr("noop"), &fn);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(fn == test_cmd_noop);

  x_arena_destroy(arena);
  return 0;
}

static int test_mi_register_and_lookup_func(void)
{
  XArena* arena;
  MiContext ctx;
  MiFunc func;
  MiFunc found;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  ok = mi_context_init(&ctx, arena, NULL, NULL);
  ASSERT_TRUE(ok);

  func.name = x_slice_from_cstr("square");
  func.param_count = 1;
  func.params = NULL;
  func.body = NULL;

  ok = mi_register_func_slice(&ctx, x_slice_from_cstr("square"), func);
  ASSERT_TRUE(ok);

  ok = mi_lookup_func(&ctx, x_slice_from_cstr("square"), &found);
  ASSERT_TRUE(ok);
  ASSERT_TRUE(x_slice_cmp(found.name, x_slice_from_cstr("square")) == 0);
  ASSERT_TRUE(found.param_count == 1);

  x_arena_destroy(arena);
  return 0;
}

static MiExecResult cmd_echo(MiContext *ctx, int argc, MiNode **argv)
{
  MiExecResult r;

  (void)ctx;

  if (argc != 1)
  {
    return mi_exec_error();
  }

  r = mi_eval_node(ctx, argv[0]);

  return r;
}

static int test_exec_simple_command(void)
{
  XArena *arena;
  MiContext ctx;
  MiNode cmd;
  MiNode arg;
  MiNode name;
  MiExecResult r;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  ok = mi_context_init(&ctx, arena, NULL, NULL);
  ASSERT_TRUE(ok);

  ok = mi_register_command(&ctx, "echo", cmd_echo);
  ASSERT_TRUE(ok);

  name.kind = MI_NODE_RAW;
  name.text = x_slice_from_cstr("echo");

  arg.kind = MI_NODE_STRING;
  arg.text = x_slice_from_cstr("hello");

  name.next_sibling = &arg;
  arg.next_sibling = NULL;

  cmd.kind = MI_NODE_COMMAND;
  cmd.first_child = &name;

  r = mi_exec_command(&ctx, &cmd);

  ASSERT_TRUE(r.signal == MI_SIGNAL_NONE);
  ASSERT_TRUE(r.value.kind == MI_VAL_STRING);
  ASSERT_TRUE(x_slice_cmp(r.value.as.string, x_slice_from_cstr("hello")) == 0);

  x_arena_destroy(arena);

  return 0;
}

static int test_variable_eval(void)
{
  XArena *arena;
  MiContext ctx;
  MiNode var;
  MiExecResult r;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  ok = mi_context_init(&ctx, arena, NULL, NULL);
  ASSERT_TRUE(ok);

  ok = mi_scope_define(
        ctx.current_scope,
        x_slice_from_cstr("x"),
        mi_value_string(x_slice_from_cstr("10"))
      );

  ASSERT_TRUE(ok);

  var.kind = MI_NODE_VARIABLE;
  var.text = x_slice_from_cstr("x");

  r = mi_eval_node(&ctx, &var);

  ASSERT_TRUE(r.signal == MI_SIGNAL_NONE);
  ASSERT_TRUE(r.value.kind == MI_VAL_STRING);
  ASSERT_TRUE(x_slice_cmp(r.value.as.string, x_slice_from_cstr("10")) == 0);

  x_arena_destroy(arena);

  return 0;
}

static int test_subcommand(void)
{
  XArena *arena;
  MiContext ctx;
  MiNode sub;
  MiNode cmd;
  MiNode name;
  MiNode arg;
  MiExecResult r;
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  ok = mi_context_init(&ctx, arena, NULL, NULL);
  ASSERT_TRUE(ok);

  ok = mi_register_command(&ctx, "echo", cmd_echo);
  ASSERT_TRUE(ok);

  name.kind = MI_NODE_RAW;
  name.text = x_slice_from_cstr("echo");

  arg.kind = MI_NODE_STRING;
  arg.text = x_slice_from_cstr("abc");

  name.next_sibling = &arg;
  arg.next_sibling = NULL;

  cmd.kind = MI_NODE_COMMAND;
  cmd.first_child = &name;

  sub.kind = MI_NODE_SUBCOMMAND;
  sub.first_child = &cmd;

  r = mi_eval_node(&ctx, &sub);

  ASSERT_TRUE(r.signal == MI_SIGNAL_NONE);
  ASSERT_TRUE(r.value.kind == MI_VAL_STRING);
  ASSERT_TRUE(x_slice_cmp(r.value.as.string, x_slice_from_cstr("abc")) == 0);

  x_arena_destroy(arena);

  return 0;
}

static int test_exec_block(void)
{
  XArena *arena;
  MiContext ctx = {0};
  MiNode block = {0};
  MiNode cmd1 = {0};
  MiNode cmd2 = {0};
  MiNode name1 = {0};
  MiNode arg1 = {0};
  MiNode name2 = {0};
  MiNode arg2 = {0};
  MiExecResult r = {0};
  bool ok;

  arena = x_arena_create(CHUNK_SIZE);

  ok = mi_context_init(&ctx, arena, NULL, NULL);
  ASSERT_TRUE(ok);

  ok = mi_register_command(&ctx, "echo", cmd_echo);
  ASSERT_TRUE(ok);

  name1.kind = MI_NODE_RAW;
  name1.text = x_slice_from_cstr("echo");

  arg1.kind = MI_NODE_STRING;
  arg1.text = x_slice_from_cstr("a");

  name1.next_sibling = &arg1;

  cmd1.kind = MI_NODE_COMMAND;
  cmd1.first_child = &name1;

  name2.kind = MI_NODE_RAW;
  name2.text = x_slice_from_cstr("echo");

  arg2.kind = MI_NODE_STRING;
  arg2.text = x_slice_from_cstr("b");

  name2.next_sibling = &arg2;

  cmd2.kind = MI_NODE_COMMAND;
  cmd2.first_child = &name2;

  cmd1.next_sibling = &cmd2;
  cmd2.next_sibling = NULL;

  block.kind = MI_NODE_BLOCK;
  block.first_child = &cmd1;

  r = mi_exec_block(&ctx, &block, true);

  ASSERT_TRUE(r.signal == MI_SIGNAL_NONE);

  x_arena_destroy(arena);

  return 0;
}

static int test_mi_parser_basic_ast(void)
{
  const char *text;
  XArena *arena;
  XSlice source;
  MiParseResult result;
  MiNode *block;
  MiNode *cmd1;
  MiNode *cmd2;
  MiNode *arg;
  MiNode *sub;

  text =
    "print \"hello\"\n"
    "print (concat \"a\" \"b\")\n";

  source = x_slice_from_cstr(text);

  arena = x_arena_create(CHUNK_SIZE);
  ASSERT_TRUE(arena != NULL);

  result = mi_parse(arena, source);
  ASSERT_TRUE(result.ok);
  ASSERT_TRUE(result.root != NULL);

  block = result.root;
  ASSERT_TRUE(block->kind == MI_NODE_BLOCK);

  cmd1 = block->first_child;
  ASSERT_TRUE(cmd1 != NULL);
  ASSERT_TRUE(cmd1->kind == MI_NODE_COMMAND);

  arg = cmd1->first_child;
  ASSERT_TRUE(arg != NULL);
  ASSERT_TRUE(arg->kind == MI_NODE_RAW);
  ASSERT_TRUE(x_slice_cmp(arg->text, x_slice_from_cstr("print")) == 0);

  arg = arg->next_sibling;
  ASSERT_TRUE(arg != NULL);
  ASSERT_TRUE(arg->kind == MI_NODE_STRING);
  ASSERT_TRUE(x_slice_cmp(arg->text, x_slice_from_cstr("hello")) == 0);

  ASSERT_TRUE(arg->next_sibling == NULL);

  cmd2 = cmd1->next_sibling;
  ASSERT_TRUE(cmd2 != NULL);
  ASSERT_TRUE(cmd2->kind == MI_NODE_COMMAND);

  arg = cmd2->first_child;
  ASSERT_TRUE(arg != NULL);
  ASSERT_TRUE(arg->kind == MI_NODE_RAW);
  ASSERT_TRUE(x_slice_cmp(arg->text, x_slice_from_cstr("print")) == 0);

  sub = arg->next_sibling;
  ASSERT_TRUE(sub != NULL);
  ASSERT_TRUE(sub->kind == MI_NODE_SUBCOMMAND);
  ASSERT_TRUE(sub->first_child != NULL);
  ASSERT_TRUE(sub->first_child->kind == MI_NODE_COMMAND);

  arg = sub->first_child->first_child;
  ASSERT_TRUE(arg != NULL);
  ASSERT_TRUE(arg->kind == MI_NODE_RAW);
  ASSERT_TRUE(x_slice_cmp(arg->text, x_slice_from_cstr("concat")) == 0);

  arg = arg->next_sibling;
  ASSERT_TRUE(arg != NULL);
  ASSERT_TRUE(arg->kind == MI_NODE_STRING);
  ASSERT_TRUE(x_slice_cmp(arg->text, x_slice_from_cstr("a")) == 0);

  arg = arg->next_sibling;
  ASSERT_TRUE(arg != NULL);
  ASSERT_TRUE(arg->kind == MI_NODE_STRING);
  ASSERT_TRUE(x_slice_cmp(arg->text, x_slice_from_cstr("b")) == 0);

  ASSERT_TRUE(arg->next_sibling == NULL);
  ASSERT_TRUE(cmd2->next_sibling == NULL);

  x_arena_destroy(arena);
  return 0;
}

int run_tests()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_mi_value_constructors),
    X_TEST(test_mi_scope_define_and_get),
    X_TEST(test_mi_scope_get_walks_parent_chain),
    X_TEST(test_mi_scope_set_updates_nearest_existing_binding),
    X_TEST(test_mi_scope_set_creates_in_current_scope_when_missing),
    X_TEST(test_mi_context_init),
    X_TEST(test_mi_register_and_lookup_command),
    X_TEST(test_mi_register_and_lookup_func),
    X_TEST(test_exec_simple_command),
    X_TEST(test_variable_eval),
    X_TEST(test_subcommand),
    X_TEST(test_exec_block),
    X_TEST(test_mi_parser_basic_ast)
  };

  return x_tests_run(tests, sizeof(tests) / sizeof(tests[0]), NULL);
}

#if 0 

int test_parser(int argc, char **argv)
{
  const char *filename;
  XArena* arena;
  XSlice source;
  MiParseResult result;

  if (argc != 2)
  {
    print_usage(argv[0]);
    return 1;
  }

  filename = argv[1];

  size_t text_size = 0;
  char *text = x_io_read_text(filename, &text_size);

  source.ptr = text;
  source.length = text_size;


  if (!text)
  {
    fprintf(stderr, "error: failed to read file: %s\n", filename);
    return 1;
  }

  arena = x_arena_create(1024 * 8);
  result = mi_parse(arena, source);

  if (!result.ok)
  {
    fprintf(
        stderr,
        "%s:%d:%d: %s\n",
        filename,
        result.error_line,
        result.error_column,
        result.error_message ? result.error_message : "parse error"
        );

    x_arena_destroy(arena);
    free(text);
    return 1;
  }

  mi_debug_print_ast(result.root, 0);

  x_arena_destroy(arena);
  free(text);
  return 0;
}
#endif

int main(int argc, char** argv)
{
  return run_tests();
}
