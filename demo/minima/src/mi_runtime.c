#include "minima.h"

#include <stdlib.h>
#include <string.h>


static char *mi_keydup_arena(XArena *arena, XSlice name)
{
  if (!name.ptr)
  {
    return NULL;
  }

  return x_arena_slicedup(arena, name.ptr, name.length, true);
}

static char *mi_keydup_heap(XSlice name)
{
  char *key;

  if (!name.ptr)
  {
    return NULL;
  }

  key = (char *)malloc(name.length + 1);

  if (!key)
  {
    return NULL;
  }

  memcpy(key, name.ptr, name.length);
  key[name.length] = '\0';
  return key;
}

MiValue mi_value_null(void)
{
  MiValue value;

  memset(&value, 0, sizeof(value));
  value.kind = MI_VAL_NULL;
  return value;
}

MiValue mi_value_string(XSlice s)
{
  MiValue value;

  memset(&value, 0, sizeof(value));
  value.kind = MI_VAL_STRING;
  value.as.string = s;
  return value;
}

MiValue mi_value_raw(XSlice s)
{
  MiValue value;

  memset(&value, 0, sizeof(value));
  value.kind = MI_VAL_RAW;
  value.as.raw = s;
  return value;
}

MiValue mi_value_user(const MiUserType *type, void* payload)
{
  MiValue value;

  memset(&value, 0, sizeof(value));
  value.kind = MI_VAL_USER;
  value.as.user.type = type;
  value.as.user.data = payload;
  return value;
}

MiValue mi_value_number(double v)
{
  MiValue r;
  r.kind = MI_VAL_NUMBER;
  r.as.number = v;
  return r;
}

MiExecResult mi_exec_ok(MiValue value)
{
  MiExecResult result;

  result.signal = MI_SIGNAL_NONE;
  result.value = value;
  return result;
}

MiExecResult mi_exec_null(void)
{
  return mi_exec_ok(mi_value_null());
}

MiExecResult mi_exec_error(void)
{
  MiExecResult result;

  result.signal = MI_SIGNAL_ERROR;
  result.value = mi_value_null();
  return result;
}

MiExecResult mi_exec_return(MiValue value)
{
  MiExecResult result;

  result.signal = MI_SIGNAL_RETURN;
  result.value = value;
  return result;
}

MiExecResult mi_exec_break(void)
{
  MiExecResult result;

  result.signal = MI_SIGNAL_BREAK;
  result.value = mi_value_null();
  return result;
}

MiExecResult mi_exec_continue(void)
{
  MiExecResult result;

  result.signal = MI_SIGNAL_CONTINUE;
  result.value = mi_value_null();
  return result;
}

bool mi_value_is_null(MiValue value)
{
  return value.kind == MI_VAL_NULL;
}

bool mi_value_is_string(MiValue value)
{
  return value.kind == MI_VAL_STRING;
}

bool mi_value_is_raw(MiValue value)
{
  return value.kind == MI_VAL_RAW;
}

bool mi_value_is_user(MiValue value)
{
  return value.kind == MI_VAL_USER;
}

MiScope *mi_scope_create(XArena *arena, MiScope *parent)
{
  MiScope *scope;

  scope = (MiScope *)x_arena_alloc_zero(arena, sizeof(MiScope));

  if (!scope)
  {
    return NULL;
  }

  scope->arena = arena;
  scope->parent = parent;
  scope->vars = x_hashtable_mi_value_table_create();

  if (!scope->vars)
  {
    return NULL;
  }

  return scope;
}

bool mi_scope_get(MiScope *scope, XSlice name, MiValue *out_value)
{
  MiScope *it;
  char *key;
  bool found;

  if (!scope || !out_value)
  {
    return false;
  }

  key = mi_keydup_heap(name);

  if (!key)
  {
    return false;
  }

  found = false;
  it = scope;

  while (it)
  {
    if (x_hashtable_mi_value_table_get(it->vars, key, out_value))
    {
      found = true;
      break;
    }

    it = it->parent;
  }

  free(key);
  return found;
}

bool mi_scope_define(MiScope *scope, XSlice name, MiValue value)
{
  char *key;

  if (!scope || !scope->vars)
  {
    return false;
  }

  key = mi_keydup_arena(scope->arena, name);

  if (!key)
  {
    return false;
  }

  return x_hashtable_mi_value_table_set(scope->vars, key, value);
}

bool mi_scope_set(MiScope *scope, XSlice name, MiValue value)
{
  MiScope *it;
  char *lookup_key;
  char *owned_key;

  if (!scope)
  {
    return false;
  }

  lookup_key = mi_keydup_heap(name);

  if (!lookup_key)
  {
    return false;
  }

  it = scope;

  while (it)
  {
    if (x_hashtable_mi_value_table_has(it->vars, lookup_key))
    {
      bool ok;

      ok = x_hashtable_mi_value_table_set(it->vars, lookup_key, value);
      free(lookup_key);
      return ok;
    }

    it = it->parent;
  }

  free(lookup_key);

  owned_key = mi_keydup_arena(scope->arena, name);

  if (!owned_key)
  {
    return false;
  }

  return x_hashtable_mi_value_table_set(scope->vars, owned_key, value);
}

bool mi_context_init(MiContext *ctx, XArena *arena, FILE* out_stream, FILE* err_stream)
{
  if (!ctx || !arena)
  {
    return false;
  }

  memset(ctx, 0, sizeof(*ctx));
  ctx->arena = arena;

  ctx->commands = x_hashtable_mi_command_table_create();
  ctx->funcs = x_hashtable_mi_func_table_create();
  ctx->output = x_strbuilder_create();
  ctx->out = out_stream ? out_stream : stdout;
  ctx->err = err_stream ? err_stream : stderr;
  ctx->source_stack = x_array_create(sizeof(MiSourceFrame), 8);

  if (!ctx->commands || !ctx->funcs || !ctx->output)
  {
    return false;
  }

  ctx->global_scope = mi_scope_create(arena, NULL);

  if (!ctx->global_scope)
  {
    return false;
  }

  ctx->current_scope = ctx->global_scope;
  ctx->error_message = NULL;
  ctx->error_line = 0;
  ctx->error_column = 0;
  return true;
}

void mi_context_reset(MiContext *ctx)
{
  if (!ctx)
  {
    return;
  }

  ctx->current_scope = ctx->global_scope;
  ctx->error_message = NULL;
  ctx->error_line = 0;
  ctx->error_column = 0;
  x_array_clear(ctx->source_stack);
  ctx->error_source_file.length = 0;

  if (ctx->output)
  {
    x_strbuilder_clear(ctx->output);
  }
}

void mi_context_set_error(MiContext *ctx, const char *message, int line, int column)
{
  if (!ctx)
  {
    return;
  }

  if (!ctx->error_message)
  {
    ctx->error_message = x_arena_strdup(ctx->arena, message ? message : "error");
    ctx->error_line = line;
    ctx->error_column = column;
    x_fs_path_set_slice(&ctx->error_source_file, mi_context_current_source(ctx));
  }
}

bool mi_register_command_slice(MiContext *ctx, XSlice name, MiCommandFn fn)
{
  char *key;

  if (!ctx || !ctx->commands || !fn)
  {
    return false;
  }

  key = mi_keydup_arena(ctx->arena, name);

  if (!key)
  {
    return false;
  }

  return x_hashtable_mi_command_table_set(ctx->commands, key, fn);
}

bool mi_register_command(MiContext *ctx, const char *name, MiCommandFn fn)
{
  if (!name)
  {
    return false;
  }

  return mi_register_command_slice(ctx, x_slice_from_cstr(name), fn);
}

bool mi_lookup_command(MiContext *ctx, XSlice name, MiCommandFn *out_fn)
{
  char *key;
  bool found;

  if (!ctx || !ctx->commands || !out_fn)
  {
    return false;
  }

  key = mi_keydup_heap(name);

  if (!key)
  {
    return false;
  }

  found = x_hashtable_mi_command_table_get(ctx->commands, key, out_fn);
  free(key);
  return found;
}

bool mi_register_func_slice(MiContext *ctx, XSlice name, MiFunc func)
{
  char *key;
  char *owned_name;

  if (!ctx || !ctx->funcs)
  {
    return false;
  }

  key = mi_keydup_arena(ctx->arena, name);

  if (!key)
  {
    return false;
  }

  owned_name = mi_keydup_arena(ctx->arena, name);

  if (!owned_name)
  {
    return false;
  }

  func.name = x_slice_from_cstr(owned_name);

  return x_hashtable_mi_func_table_set(ctx->funcs, key, func);
}

bool mi_lookup_func(MiContext *ctx, XSlice name, MiFunc *out_func)
{
  char *key;
  bool found;

  if (!ctx || !ctx->funcs || !out_func)
  {
    return false;
  }

  key = mi_keydup_heap(name);

  if (!key)
  {
    return false;
  }

  found = x_hashtable_mi_func_table_get(ctx->funcs, key, out_func);
  free(key);
  return found;
}

MiScope *mi_push_scope(MiContext *ctx)
{
  MiScope *scope;

  if (!ctx || !ctx->arena || !ctx->current_scope)
  {
    return NULL;
  }

  scope = mi_scope_create(ctx->arena, ctx->current_scope);

  if (!scope)
  {
    mi_context_set_error(ctx, "failed to create scope", 0, 0);
    return NULL;
  }

  ctx->current_scope = scope;
  return scope;
}

void mi_pop_scope(MiContext *ctx)
{
  if (!ctx || !ctx->current_scope)
  {
    return;
  }

  if (ctx->current_scope->parent)
  {
    ctx->current_scope = ctx->current_scope->parent;
  }
}

MiExecResult mi_eval_node(MiContext *ctx, MiNode *node)
{
  MiValue value;

  if (!ctx || !node)
  {
    if (ctx)
    {
      mi_context_set_error(ctx, "invalid node", 0, 0);
    }
    return mi_exec_error();
  }

  switch (node->kind)
  {
    case MI_NODE_STRING:
      return mi_exec_ok(mi_value_string(node->text));

    case MI_NODE_RAW:
      return mi_exec_ok(mi_value_raw(node->text));

    case MI_NODE_VARIABLE:
      if (!mi_scope_get(ctx->current_scope, node->text, &value))
      {
        mi_context_set_error(ctx, "undefined variable", node->line, node->column);
        return mi_exec_error();
      }

      return mi_exec_ok(value);

    case MI_NODE_SUBCOMMAND:
      if (!node->first_child)
      {
        mi_context_set_error(ctx, "empty subcommand", node->line, node->column);
        return mi_exec_error();
      }

      return mi_exec_command(ctx, node->first_child);

    case MI_NODE_BLOCK:
      mi_context_set_error(ctx, "block cannot be evaluated as a value", node->line, node->column);
      return mi_exec_error();

    case MI_NODE_COMMAND:
      return mi_exec_command(ctx, node);

    default:
      mi_context_set_error(ctx, "unknown node kind", node->line, node->column);
      return mi_exec_error();
  }
}

MiExecResult mi_call_func(MiContext *ctx, const MiFunc *func, int argc, MiNode **argv)
{
  MiScope *old_scope;
  MiScope *call_scope;
  MiExecResult result;
  int i;

  if (!ctx || !func)
  {
    if (ctx)
    {
      mi_context_set_error(ctx, "invalid function call", 0, 0);
    }
    return mi_exec_error();
  }

  if ((size_t)argc != func->param_count)
  {
    mi_context_set_error(ctx, "wrong number of arguments", 0, 0);
    return mi_exec_error();
  }

  old_scope = ctx->current_scope;
  call_scope = mi_scope_create(ctx->arena, ctx->global_scope);

  if (!call_scope)
  {
    mi_context_set_error(ctx, "failed to create function scope", 0, 0);
    return mi_exec_error();
  }

  ctx->current_scope = call_scope;

  for (i = 0; i < argc; i++)
  {
    MiExecResult arg_result;

    arg_result = mi_eval_node(ctx, argv[i]);

    if (arg_result.signal != MI_SIGNAL_NONE)
    {
      ctx->current_scope = old_scope;
      return arg_result;
    }

    if (!mi_scope_define(call_scope, func->params[i], arg_result.value))
    {
      mi_context_set_error(ctx, "failed to bind function parameter", 0, 0);
      ctx->current_scope = old_scope;
      return mi_exec_error();
    }
  }

  result = mi_exec_block(ctx, func->body, false);

  ctx->current_scope = old_scope;

  if (result.signal == MI_SIGNAL_RETURN)
  {
    result.signal = MI_SIGNAL_NONE;
    return result;
  }

  return result;
}

MiExecResult mi_exec_command(MiContext *ctx, MiNode *command)
{
  MiNode *name_node;
  MiNode *arg_node;
  MiCommandFn fn;
  MiFunc func;
  MiNode *argv_buf[64];
  int argc;

  if (!ctx || !command || command->kind != MI_NODE_COMMAND)
  {
    if (ctx)
    {
      mi_context_set_error(ctx, "invalid command node", 0, 0);
    }
    return mi_exec_error();
  }

  name_node = command->first_child;

  if (!name_node)
  {
    mi_context_set_error(ctx, "empty command", command->line, command->column);
    return mi_exec_error();
  }

  if (name_node->kind != MI_NODE_RAW)
  {
    mi_context_set_error(ctx, "command name must be raw", name_node->line, name_node->column);
    return mi_exec_error();
  }

  argc = 0;
  arg_node = name_node->next_sibling;

  while (arg_node)
  {
    if (argc >= (int)(sizeof(argv_buf) / sizeof(argv_buf[0])))
    {
      mi_context_set_error(ctx, "too many command arguments", arg_node->line, arg_node->column);
      return mi_exec_error();
    }

    argv_buf[argc] = arg_node;
    argc += 1;
    arg_node = arg_node->next_sibling;
  }

  if (mi_lookup_command(ctx, name_node->text, &fn))
  {
    return fn(ctx, argc, argv_buf);
  }

  if (mi_lookup_func(ctx, name_node->text, &func))
  {
    return mi_call_func(ctx, &func, argc, argv_buf);
  }

  mi_context_set_error(ctx, "unknown command", name_node->line, name_node->column);
  return mi_exec_error();
}

MiExecResult mi_exec_block(MiContext *ctx, MiNode *block, bool create_scope)
{
  MiExecResult result;
  MiNode *command;
  MiScope *saved_scope;

  if (!ctx || !block || block->kind != MI_NODE_BLOCK)
  {
    if (ctx)
    {
      mi_context_set_error(ctx, "invalid block node", 0, 0);
    }
    return mi_exec_error();
  }

  saved_scope = ctx->current_scope;

  if (create_scope)
  {
    if (!mi_push_scope(ctx))
    {
      return mi_exec_error();
    }
  }

  command = block->first_child;
  result = mi_exec_null();

  while (command)
  {
    result = mi_exec_command(ctx, command);

    if (result.signal != MI_SIGNAL_NONE)
    {
      if (create_scope)
      {
        ctx->current_scope = saved_scope;
      }

      return result;
    }

    command = command->next_sibling;
  }

  if (create_scope)
  {
    ctx->current_scope = saved_scope;
  }

  return result;
}

bool mi_context_push_source(MiContext *ctx, XSlice file_name)
{
  MiSourceFrame frame;

  if (!ctx || !ctx->source_stack)
  {
    return false;
  }

  x_fs_path_from_slice(file_name, &frame.source_name);
  x_array_push(ctx->source_stack, &frame);
  return true;
}

void mi_context_pop_source(MiContext *ctx)
{
  if (!ctx)
  {
    return;
  }

  if (x_array_is_empty(ctx->source_stack))
  {
    return;
  }

  x_array_pop(ctx->source_stack);
}

XSlice mi_context_current_source(MiContext *ctx)
{
  MiSourceFrame *frame;
  
  if (!ctx || x_array_is_empty(ctx->source_stack))
  {
    return x_slice_from_cstr("<unknown>");
  }

  frame = (MiSourceFrame *)x_array_back(ctx->source_stack);
  
  return x_slice_init(frame->source_name.buf, frame->source_name.length);
}
