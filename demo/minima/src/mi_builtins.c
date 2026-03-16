
#include "mi_builtins.h"
#include "mi_parser.h"
#include "mi_runtime.h"

#include <stdx_array.h>
#include <stdx_filesystem.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//
// EXPR
//

typedef struct MiExprParser
{
  MiContext *ctx;
  i32 argc;
  MiNode **argv;
  i32 index;
  MiExecResult pending;
  bool failed;
} MiExprParser;

static bool mi_expr_parse_or(MiExprParser *p, double *out_value);

static bool mi_slice_to_double(XSlice slice, double *out_value)
{
  size_t i = 0;
  bool seen_digit;
  double value;
  double frac_scale;
  bool negative;

  if (!out_value)
  {
    return false;
  }

  if (!slice.ptr || slice.length == 0)
  {
    return false;
  }

  negative = false;
  seen_digit = false;
  value = 0.0;
  frac_scale = 0.1;

  if (slice.ptr[i] == '+' || slice.ptr[i] == '-')
  {
    negative = (slice.ptr[i] == '-');
    i += 1;

    if (i >= slice.length)
    {
      return false;
    }
  }

  while (i < slice.length && slice.ptr[i] >= '0' && slice.ptr[i] <= '9')
  {
    seen_digit = true;
    value = value * 10.0 + (double)(slice.ptr[i] - '0');
    i += 1;
  }

  if (i < slice.length && slice.ptr[i] == '.')
  {
    i += 1;

    while (i < slice.length && slice.ptr[i] >= '0' && slice.ptr[i] <= '9')
    {
      seen_digit = true;
      value += (double)(slice.ptr[i] - '0') * frac_scale;
      frac_scale *= 0.1;
      i += 1;
    }
  }

  if (!seen_digit || i != slice.length)
  {
    return false;
  }

  *out_value = negative ? -value : value;
  return true;
}

static bool mi_value_is_truthy(MiValue value)
{
  XSlice slice;
  double number;

  switch (value.kind)
  {
    case MI_VAL_NULL:
      return false;

    case MI_VAL_NUMBER:
      return value.as.number != 0.0;

    case MI_VAL_USER:
      return true;

    case MI_VAL_STRING:
      slice = value.as.string;
      break;

    case MI_VAL_RAW:
      slice = value.as.raw;
      break;

    default:
      return false;
  }

  if (!slice.ptr || slice.length == 0)
  {
    return false;
  }

  if (mi_slice_to_double(slice, &number))
  {
    return number != 0.0;
  }

  return true;
}

static bool mi_node_is_raw_op(MiNode *node, const char *op)
{
  if (!node)
  {
    return false;
  }

  if (node->kind != MI_NODE_RAW)
  {
    return false;
  }

  return x_slice_cmp(node->text, x_slice_from_cstr(op)) == 0;
}

static MiNode *mi_expr_peek(MiExprParser *p)
{
  if (p->index >= p->argc)
  {
    return NULL;
  }

  return p->argv[p->index];
}

static MiNode *mi_expr_next(MiExprParser *p)
{
  MiNode *node;

  if (p->index >= p->argc)
  {
    return NULL;
  }

  node = p->argv[p->index];
  p->index += 1;
  return node;
}

static bool mi_expr_match(MiExprParser *p, const char *op)
{
  MiNode *node;

  node = mi_expr_peek(p);

  if (!node)
  {
    return false;
  }

  if (node->kind != MI_NODE_RAW)
  {
    return false;
  }

  if (x_slice_cmp(node->text, x_slice_from_cstr(op)) != 0)
  {
    return false;
  }

  p->index += 1;
  return true;
}

static bool mi_expr_value_to_double(MiExprParser *p, MiValue value, double *out_value)
{
  XSlice slice;
  char *text;
  char *end;
  double v;

  if (!out_value)
  {
    return false;
  }

  switch (value.kind)
  {
    case MI_VAL_NUMBER:
      *out_value = value.as.number;
      return true;

    case MI_VAL_RAW:
      slice = value.as.raw;
      break;

    case MI_VAL_STRING:
      slice = value.as.string;
      break;

    case MI_VAL_NULL:
      *out_value = 0.0;
      return true;

    default:
      mi_context_set_error(p->ctx, "expr expects numeric/string/raw operands", 0, 0);
      p->pending = mi_exec_error();
      p->failed = true;
      return false;
  }

  text = x_arena_slicedup(p->ctx->arena, slice.ptr, slice.length, true);

  if (!text)
  {
    mi_context_set_error(p->ctx, "expr failed to allocate temporary string", 0, 0);
    p->pending = mi_exec_error();
    p->failed = true;
    return false;
  }

  end = NULL;
  v = strtod(text, &end);

  if (!end || *end != '\0')
  {
    mi_context_set_error(p->ctx, "expr operand is not a valid number", 0, 0);
    p->pending = mi_exec_error();
    p->failed = true;
    return false;
  }

  *out_value = v;
  return true;
}

static bool mi_expr_eval_operand_node(MiExprParser *p, MiNode *node, double *out_value)
{
  MiExecResult r;

  r = mi_eval_node(p->ctx, node);

  if (r.signal != MI_SIGNAL_NONE)
  {
    p->pending = r;
    p->failed = true;
    return false;
  }

  return mi_expr_value_to_double(p, r.value, out_value);
}

static bool mi_expr_parse_primary(MiExprParser *p, double *out_value)
{
  MiNode *node;

  node = mi_expr_peek(p);

  if (!node)
  {
    mi_context_set_error(p->ctx, "unexpected end of expression", 0, 0);
    p->pending = mi_exec_error();
    p->failed = true;
    return false;
  }

  if (mi_expr_match(p, "-"))
  {
    double v;

    if (!mi_expr_parse_primary(p, &v))
    {
      return false;
    }

    *out_value = -v;
    return true;
  }

  if (mi_expr_match(p, "!"))
  {
    double v;

    if (!mi_expr_parse_primary(p, &v))
    {
      return false;
    }

    *out_value = (v == 0.0) ? 1.0 : 0.0;
    return true;
  }

  if (mi_expr_match(p, "["))
  {
    double grouped_value;

    if (!mi_expr_parse_or(p, &grouped_value))
    {
      return false;
    }

    if (!mi_expr_match(p, "]"))
    {
      MiNode *bad;

      bad = mi_expr_peek(p);
      mi_context_set_error(
          p->ctx,
          "expected ']' to close grouped expression",
          bad ? bad->line : 0,
          bad ? bad->column : 0
          );
      p->pending = mi_exec_error();
      p->failed = true;
      return false;
    }

    *out_value = grouped_value;
    return true;
  }

  /*
     Normal operand:
     - RAW number
     - STRING number
     - VARIABLE
     - SUBCOMMAND (normal function/command call)
     */
  node = mi_expr_next(p);
  return mi_expr_eval_operand_node(p, node, out_value);
}

static bool mi_expr_parse_mul(MiExprParser *p, double *out_value)
{
  double lhs;

  if (!mi_expr_parse_primary(p, &lhs))
  {
    return false;
  }

  while (true)
  {
    double rhs;

    if (mi_expr_match(p, "*"))
    {
      if (!mi_expr_parse_primary(p, &rhs))
      {
        return false;
      }

      lhs = lhs * rhs;
      continue;
    }

    if (mi_expr_match(p, "/"))
    {
      if (!mi_expr_parse_primary(p, &rhs))
      {
        return false;
      }

      if (rhs == 0.0)
      {
        mi_context_set_error(p->ctx, "division by zero", 0, 0);
        p->pending = mi_exec_error();
        p->failed = true;
        return false;
      }

      lhs = lhs / rhs;
      continue;
    }

    if (mi_expr_match(p, "%"))
    {
      if (!mi_expr_parse_primary(p, &rhs))
      {
        return false;
      }

      if (rhs == 0.0)
      {
        mi_context_set_error(p->ctx, "modulo by zero", 0, 0);
        p->pending = mi_exec_error();
        p->failed = true;
        return false;
      }

      lhs = fmod(lhs, rhs);
      continue;
    }

    break;
  }

  *out_value = lhs;
  return true;
}

static bool mi_expr_parse_add(MiExprParser *p, double *out_value)
{
  double lhs;

  if (!mi_expr_parse_mul(p, &lhs))
  {
    return false;
  }

  while (true)
  {
    double rhs;

    if (mi_expr_match(p, "+"))
    {
      if (!mi_expr_parse_mul(p, &rhs))
      {
        return false;
      }

      lhs = lhs + rhs;
      continue;
    }

    if (mi_expr_match(p, "-"))
    {
      if (!mi_expr_parse_mul(p, &rhs))
      {
        return false;
      }

      lhs = lhs - rhs;
      continue;
    }

    break;
  }

  *out_value = lhs;
  return true;
}

static bool mi_expr_parse_rel(MiExprParser *p, double *out_value)
{
  double lhs;

  if (!mi_expr_parse_add(p, &lhs))
  {
    return false;
  }

  while (true)
  {
    double rhs;

    if (mi_expr_match(p, ">="))
    {
      if (!mi_expr_parse_add(p, &rhs))
      {
        return false;
      }

      lhs = (lhs >= rhs) ? 1.0 : 0.0;
      continue;
    }

    if (mi_expr_match(p, "<="))
    {
      if (!mi_expr_parse_add(p, &rhs))
      {
        return false;
      }

      lhs = (lhs <= rhs) ? 1.0 : 0.0;
      continue;
    }

    if (mi_expr_match(p, ">"))
    {
      if (!mi_expr_parse_add(p, &rhs))
      {
        return false;
      }

      lhs = (lhs > rhs) ? 1.0 : 0.0;
      continue;
    }

    if (mi_expr_match(p, "<"))
    {
      if (!mi_expr_parse_add(p, &rhs))
      {
        return false;
      }

      lhs = (lhs < rhs) ? 1.0 : 0.0;
      continue;
    }

    break;
  }

  *out_value = lhs;
  return true;
}

static bool mi_expr_parse_eq(MiExprParser *p, double *out_value)
{
  double lhs;

  if (!mi_expr_parse_rel(p, &lhs))
  {
    return false;
  }

  while (true)
  {
    double rhs;

    if (mi_expr_match(p, "=="))
    {
      if (!mi_expr_parse_rel(p, &rhs))
      {
        return false;
      }

      lhs = (lhs == rhs) ? 1.0 : 0.0;
      continue;
    }

    if (mi_expr_match(p, "!="))
    {
      if (!mi_expr_parse_rel(p, &rhs))
      {
        return false;
      }

      lhs = (lhs != rhs) ? 1.0 : 0.0;
      continue;
    }

    break;
  }

  *out_value = lhs;
  return true;
}

static bool mi_expr_parse_and(MiExprParser *p, double *out_value)
{
  double lhs;

  if (!mi_expr_parse_eq(p, &lhs))
  {
    return false;
  }

  while (mi_expr_match(p, "&&"))
  {
    double rhs;

    if (!mi_expr_parse_eq(p, &rhs))
    {
      return false;
    }

    lhs = ((lhs != 0.0) && (rhs != 0.0)) ? 1.0 : 0.0;
  }

  *out_value = lhs;
  return true;
}

static bool mi_expr_parse_or(MiExprParser *p, double *out_value)
{
  double lhs;

  if (!mi_expr_parse_and(p, &lhs))
  {
    return false;
  }

  while (mi_expr_match(p, "||"))
  {
    double rhs;

    if (!mi_expr_parse_and(p, &rhs))
    {
      return false;
    }

    lhs = ((lhs != 0.0) || (rhs != 0.0)) ? 1.0 : 0.0;
  }

  *out_value = lhs;
  return true;
}

static MiExecResult mi_cmd_expr(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiExprParser p;
  double value;

  if (!ctx)
  {
    return mi_exec_error();
  }

  if (argc <= 0)
  {
    mi_context_set_error(ctx, "expr expects at least one argument", 0, 0);
    return mi_exec_error();
  }

  memset(&p, 0, sizeof(p));
  p.ctx = ctx;
  p.argc = argc;
  p.argv = argv;
  p.index = 0;
  p.pending = mi_exec_null();
  p.failed = false;

  if (!mi_expr_parse_or(&p, &value))
  {
    return (p.pending.signal == MI_SIGNAL_NONE) ? mi_exec_error() : p.pending;
  }

  if (p.index != p.argc)
  {
    MiNode *node;

    node = p.argv[p.index];
    mi_context_set_error(
        ctx,
        "unexpected token at end of expression",
        node ? node->line : 0,
        node ? node->column : 0
        );
    return mi_exec_error();
  }

  return mi_exec_ok(mi_value_number(value));
}


//
// SET
//

MiExecResult mi_cmd_set(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue value;
  MiExecResult r;
  MiNode *name_node;

  if (argc != 2)
  {
    mi_context_set_error(ctx, "set expects 2 arguments", 0, 0);
    return mi_exec_error();
  }

  name_node = argv[0];

  if (name_node->kind != MI_NODE_RAW)
  {
    mi_context_set_error(ctx, "set name must be raw", name_node->line, name_node->column);
    return mi_exec_error();
  }

  r = mi_eval_node(ctx, argv[1]);

  if (r.signal != MI_SIGNAL_NONE)
  {
    return r;
  }

  value = r.value;

  if (!mi_scope_set(ctx->current_scope, name_node->text, value))
  {
    mi_context_set_error(ctx, "failed to set variable", name_node->line, name_node->column);
    return mi_exec_error();
  }

  return mi_exec_ok(value);
}

//
// PRINT
//

static XSlice mi_unescape_to_arena(XArena *arena, XSlice src)
{
  XSlice dst;
  char *ptr;
  size_t si;
  size_t di;

  ptr = (char *)x_arena_alloc(arena, src.length + 1);

  if (!ptr)
  {
    dst.ptr = NULL;
    dst.length = 0;
    return dst;
  }

  di = 0;

  for (si = 0; si < src.length; si++)
  {
    char c;

    c = src.ptr[si];

    if (c == '\\' && (si + 1) < src.length)
    {
      si++;

      switch (src.ptr[si])
      {
        case 'n':
          ptr[di++] = '\n';
          break;

        case 't':
          ptr[di++] = '\t';
          break;

        case 'r':
          ptr[di++] = '\r';
          break;

        case '\\':
          ptr[di++] = '\\';
          break;

        case '"':
          ptr[di++] = '"';
          break;

        default:
          ptr[di++] = '\\';
          ptr[di++] = src.ptr[si];
          break;
      }
    }
    else
    {
      ptr[di++] = c;
    }
  }

  ptr[di] = 0;

  dst.ptr = ptr;
  dst.length = di;
  return dst;
}

MiExecResult mi_cmd_format(MiContext *ctx, int argc, MiNode **argv)
{
  static XArena *format_arena = NULL;
  MiExecResult fmt_res;
  XSlice fmt_src;
  XSlice fmt;
  XStrBuilder *sb;
  size_t i;

  if (!ctx)
  {
    return mi_exec_error();
  }

  if (argc != 1)
  {
    mi_context_set_error(ctx, "format expects one string argument", 0, 0);
    return mi_exec_error();
  }

  if (!format_arena)
  {
    format_arena = x_arena_create(2048);
  }

  if (!format_arena)
  {
    mi_context_set_error(ctx, "format failed to create scratch arena", 0, 0);
    return mi_exec_error();
  }

  x_arena_reset(format_arena);

  fmt_res = mi_eval_node(ctx, argv[0]);

  if (fmt_res.signal != MI_SIGNAL_NONE)
  {
    return fmt_res;
  }

  if (fmt_res.value.kind == MI_VAL_STRING)
  {
    fmt_src = fmt_res.value.as.string;
  }
  else if (fmt_res.value.kind == MI_VAL_RAW)
  {
    fmt_src = fmt_res.value.as.raw;
  }
  else
  {
    mi_context_set_error(ctx, "format argument must be string or raw", 0, 0);
    return mi_exec_error();
  }

  fmt = mi_unescape_to_arena(format_arena, fmt_src);

  sb = x_strbuilder_create();

  if (!sb)
  {
    mi_context_set_error(ctx, "format failed to create string builder", 0, 0);
    return mi_exec_error();
  }

  i = 0;

  while (i < fmt.length)
  {
    char c;

    c = fmt.ptr[i];

    if (c == '$')
    {
      if ((i + 1) < fmt.length && fmt.ptr[i + 1] == '$')
      {
        x_strbuilder_append_char(sb, '$');
        i += 2;
        continue;
      }

      if ((i + 1) < fmt.length && fmt.ptr[i + 1] == '{')
      {
        size_t start;
        size_t len;
        XSlice name;
        MiValue value;

        i += 2;
        start = i;

        while (i < fmt.length && fmt.ptr[i] != '}')
        {
          i++;
        }

        if (i >= fmt.length)
        {
          x_strbuilder_destroy(sb);
          mi_context_set_error(ctx, "format missing }", 0, 0);
          return mi_exec_error();
        }

        len = i - start;
        name = x_slice_init(fmt.ptr + start, len);

        if (!mi_scope_get(ctx->current_scope, name, &value))
        {
          x_strbuilder_destroy(sb);
          mi_context_set_error(ctx, "undefined variable", 0, 0);
          return mi_exec_error();
        }

        switch (value.kind)
        {
          case MI_VAL_STRING:
            x_strbuilder_append_substring(sb, value.as.string.ptr, value.as.string.length);
            break;

          case MI_VAL_RAW:
            x_strbuilder_append_substring(sb, value.as.raw.ptr, value.as.raw.length);
            break;

          case MI_VAL_NUMBER:
            {
              char numbuf[64];
              int written;

              written = snprintf(numbuf, sizeof(numbuf), "%.17g", value.as.number);

              if (written > 0)
              {
                x_strbuilder_append_substring(sb, numbuf, (size_t)written);
              }
            } break;

          case MI_VAL_NULL:
            x_strbuilder_append_cstr(sb, "null");
            break;

          default:
            x_strbuilder_append_cstr(sb, "<user>");
            break;
        }

        i += 1;
        continue;
      }

      if ((i + 1) < fmt.length && fmt.ptr[i + 1] == '(')
      {
        size_t start;
        size_t expr_len;
        int depth;
        XSlice expr_src;
        MiParseResult p_res;
        MiExecResult r;
        XArena *expr_arena;
        XSlice text;

        i += 2;
        start = i;
        depth = 1;

        while (i < fmt.length && depth > 0)
        {
          if (fmt.ptr[i] == '(')
          {
            depth++;
          }
          else if (fmt.ptr[i] == ')')
          {
            depth--;
          }

          i++;
        }

        if (depth != 0)
        {
          x_strbuilder_destroy(sb);
          mi_context_set_error(ctx, "format missing )", 0, 0);
          return mi_exec_error();
        }

        expr_len = (i - 1) - start;
        expr_src = x_slice_init(fmt.ptr + start, expr_len);

        expr_arena = x_arena_create(1024);

        if (!expr_arena)
        {
          x_strbuilder_destroy(sb);
          mi_context_set_error(ctx, "format failed to create expression arena", 0, 0);
          return mi_exec_error();
        }

        p_res = mi_parse(expr_arena, expr_src);

        if (!p_res.ok || !p_res.root)
        {
          x_arena_destroy(expr_arena);
          x_strbuilder_destroy(sb);
          mi_context_set_error(
              ctx,
              p_res.error_message ? p_res.error_message : "format expression parse error",
              p_res.error_line,
              p_res.error_column
              );
          return mi_exec_error();
        }

        r = mi_exec_block(ctx, p_res.root, false);
        x_arena_destroy(expr_arena);

        if (r.signal != MI_SIGNAL_NONE)
        {
          x_strbuilder_destroy(sb);
          return r;
        }

        switch (r.value.kind)
        {
          case MI_VAL_STRING:
            text = mi_unescape_to_arena(format_arena, r.value.as.string);
            x_strbuilder_append_substring(sb, text.ptr, text.length);
            break;

          case MI_VAL_RAW:
            text = mi_unescape_to_arena(format_arena, r.value.as.raw);
            x_strbuilder_append_substring(sb, text.ptr, text.length);
            break;

          case MI_VAL_NUMBER:
            {
              char numbuf[64];
              int written;

              written = snprintf(numbuf, sizeof(numbuf), "%.17g", r.value.as.number);

              if (written > 0)
              {
                x_strbuilder_append_substring(sb, numbuf, (size_t)written);
              }
            } break;

          case MI_VAL_NULL:
            x_strbuilder_append_cstr(sb, "null");
            break;

          default:
            x_strbuilder_append_cstr(sb, "<user>");
            break;
        }

        continue;
      }
    }

    x_strbuilder_append_char(sb, c);
    i++;
  }

  {
    size_t sb_len = x_strbuilder_length(sb);
    char *owned = x_arena_strdup(ctx->arena, x_strbuilder_to_string(sb));
    x_strbuilder_destroy(sb);

    if (!owned)
    {
      mi_context_set_error(ctx, "format failed to allocate result", 0, 0);
      return mi_exec_error();
    }

    return mi_exec_ok(mi_value_string(x_slice_init(owned, sb_len)));
  }
}

static void mi_write_value(FILE *out, XArena *arena, MiValue value)
{
  XSlice s;

  switch (value.kind)
  {
    case MI_VAL_STRING:
      s.ptr = (char *)value.as.string.ptr;
      s.length = value.as.string.length;
      s = mi_unescape_to_arena(arena, s);
      fwrite(s.ptr, 1, s.length, out);
      break;

    case MI_VAL_RAW:
      s.ptr = (char *)value.as.raw.ptr;
      s.length = value.as.raw.length;
      s = mi_unescape_to_arena(arena, s);
      fwrite(s.ptr, 1, s.length, out);
      break;

    case MI_VAL_NUMBER:
      fprintf(out, "%.17g", value.as.number);
      break;

    case MI_VAL_NULL:
      fputs("null", out);
      break;

    default:
      fputs("<user>", out);
      break;
  }
}

MiExecResult mi_cmd_print(MiContext *ctx, i32 argc, MiNode **argv)
{
  static XArena *print_arena = NULL;

  if (!ctx)
  {
    return mi_exec_error();
  }

  if (!print_arena)
  {
    print_arena = x_arena_create(1024);
  }

  x_arena_reset(print_arena);

  for (i32 i = 0; i < argc; i++)
  {
    MiExecResult r;

    r = mi_eval_node(ctx, argv[i]);

    if (r.signal != MI_SIGNAL_NONE)
    {
      return r;
    }

    if (i > 0)
    {
      fputc(' ', ctx->out);
    }

    mi_write_value(ctx->out, print_arena, r.value);
  }

  return mi_exec_null();
}


//
// IF
//


MiExecResult mi_cmd_if(MiContext *ctx, i32 argc, MiNode **argv)
{
  if (argc < 2)
  {
    mi_context_set_error(ctx, "if expects at least a condition and a block", 0, 0);
    return mi_exec_error();
  }

  i32 i = 0;

  while (i < argc)
  {
    MiNode *node;

    node = argv[i];

    if (node->kind == MI_NODE_RAW && x_slice_cmp(node->text, x_slice_from_cstr("else")) == 0)
    {
      MiNode *else_block;

      if (i + 1 >= argc)
      {
        mi_context_set_error(ctx, "else expects a block", node->line, node->column);
        return mi_exec_error();
      }

      if (i + 2 != argc)
      {
        mi_context_set_error(ctx, "else must be the last branch", node->line, node->column);
        return mi_exec_error();
      }

      else_block = argv[i + 1];

      if (!else_block || else_block->kind != MI_NODE_BLOCK)
      {
        mi_context_set_error(
            ctx,
            "else expects a block",
            else_block ? else_block->line : node->line,
            else_block ? else_block->column : node->column
            );
        return mi_exec_error();
      }

      return mi_exec_block(ctx, else_block, true);
    }

    if (node->kind == MI_NODE_RAW && x_slice_cmp(node->text, x_slice_from_cstr("elseif")) == 0)
    {
      MiExecResult cond_result;
      MiNode *cond_node;
      MiNode *block_node;

      if (i + 2 >= argc)
      {
        mi_context_set_error(ctx, "elseif expects a condition and a block", node->line, node->column);
        return mi_exec_error();
      }

      cond_node = argv[i + 1];
      block_node = argv[i + 2];

      if (!block_node || block_node->kind != MI_NODE_BLOCK)
      {
        mi_context_set_error(
            ctx,
            "elseif expects a block",
            block_node ? block_node->line : node->line,
            block_node ? block_node->column : node->column
            );
        return mi_exec_error();
      }

      cond_result = mi_eval_node(ctx, cond_node);

      if (cond_result.signal != MI_SIGNAL_NONE)
      {
        return cond_result;
      }

      if (mi_value_is_truthy(cond_result.value))
      {
        return mi_exec_block(ctx, block_node, true);
      }

      i += 3;
      continue;
    }

    {
      MiExecResult cond_result;
      MiNode *cond_node;
      MiNode *block_node;

      if (i + 1 >= argc)
      {
        mi_context_set_error(ctx, "if branch expects a condition and a block", node->line, node->column);
        return mi_exec_error();
      }

      cond_node = argv[i];
      block_node = argv[i + 1];

      if (!block_node || block_node->kind != MI_NODE_BLOCK)
      {
        mi_context_set_error(
            ctx,
            "if expects a block after condition",
            block_node ? block_node->line : cond_node->line,
            block_node ? block_node->column : cond_node->column
            );
        return mi_exec_error();
      }

      cond_result = mi_eval_node(ctx, cond_node);

      if (cond_result.signal != MI_SIGNAL_NONE)
      {
        return cond_result;
      }

      if (mi_value_is_truthy(cond_result.value))
      {
        return mi_exec_block(ctx, block_node, true);
      }

      i += 2;
    }
  }

  return mi_exec_null();
}

//
// WHILE
//

static MiExecResult mi_cmd_while(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiNode *cond_node;
  MiNode *block_node;

  if (argc != 2)
  {
    mi_context_set_error(ctx, "while expects a condition and a block", 0, 0);
    return mi_exec_error();
  }

  cond_node = argv[0];
  block_node = argv[1];

  if (!block_node || block_node->kind != MI_NODE_BLOCK)
  {
    mi_context_set_error(
        ctx,
        "while expects a block",
        block_node ? block_node->line : 0,
        block_node ? block_node->column : 0
        );
    return mi_exec_error();
  }

  while (true)
  {
    MiExecResult cond_result;
    MiExecResult body_result;

    cond_result = mi_eval_node(ctx, cond_node);

    if (cond_result.signal != MI_SIGNAL_NONE)
    {
      return cond_result;
    }

    if (!mi_value_is_truthy(cond_result.value))
    {
      return mi_exec_null();
    }

    body_result = mi_exec_block(ctx, block_node, true);

    if (body_result.signal == MI_SIGNAL_NONE)
    {
      continue;
    }

    if (body_result.signal == MI_SIGNAL_CONTINUE)
    {
      continue;
    }

    if (body_result.signal == MI_SIGNAL_BREAK)
    {
      return mi_exec_null();
    }

    return body_result;
  }
}

//
// BREAK
//

MiExecResult mi_cmd_break(MiContext *ctx, i32 argc, MiNode **argv)
{
  (void)ctx;
  (void)argv;

  if (argc != 0)
  {
    if (ctx)
    {
      mi_context_set_error(ctx, "break expects no arguments", 0, 0);
    }
    return mi_exec_error();
  }

  return mi_exec_break();
}


//
// CONTINUE
//

MiExecResult mi_cmd_continue(MiContext *ctx, i32 argc, MiNode **argv)
{
  (void)ctx;
  (void)argv;

  if (argc != 0)
  {
    if (ctx)
    {
      mi_context_set_error(ctx, "continue expects no arguments", 0, 0);
    }
    return mi_exec_error();
  }

  return mi_exec_continue();
}


//
// LIST
//


X_ARRAY_TYPE(MiValue);
MI_DEFINE_TYPE(List);

typedef struct MiList
{
  uint32_t magic;
  bool destroyed;
  XArray_MiValue *items;
} MiList;

#define MI_LIST_MAGIC 0x4D494C53u

MiList *mi_list_create(size_t capacity)
{
  MiList *list;

  list = (MiList*)malloc(sizeof(MiList));
  if (!list)
  {
    return NULL;
  }

  list->magic = MI_LIST_MAGIC;
  list->destroyed = false;
  list->items = x_array_MiValue_create(capacity);

  if (!list->items)
  {
    free(list);
    return NULL;
  }

  return list;
}

MiValue mi_value_list(i32 capacity)
{
  MiList *list;

  list = mi_list_create(capacity > 0 ? (size_t)capacity : 4);
  if (!list)
  {
    return mi_value_null();
  }

  return mi_value_user(&List_type, list);
}

static MiExecResult mi_list_cmd_create(MiContext *ctx, i32 argc, MiNode **argv);

static bool mi_list_slice_equals_cstr(XSlice slice, const char *cstr)
{
  size_t len;

  if (!cstr)
  {
    return false;
  }

  len = strlen(cstr);

  if (slice.length != len)
  {
    return false;
  }

  if (len == 0)
  {
    return true;
  }

  return memcmp(slice.ptr, cstr, len) == 0;
}

static bool mi_list_value_equals(MiValue a, MiValue b)
{
  if (a.kind != b.kind)
  {
    return false;
  }

  switch (a.kind)
  {
    case MI_VAL_NULL:
    {
      return true;
    }

    case MI_VAL_STRING:
    {
      if (a.as.string.length != b.as.string.length)
      {
        return false;
      }

      if (a.as.string.length == 0)
      {
        return true;
      }

      return memcmp(a.as.string.ptr, b.as.string.ptr, a.as.string.length) == 0;
    }

    case MI_VAL_RAW:
    {
      if (a.as.raw.length != b.as.raw.length)
      {
        return false;
      }

      if (a.as.raw.length == 0)
      {
        return true;
      }

      return memcmp(a.as.raw.ptr, b.as.raw.ptr, a.as.raw.length) == 0;
    }

    case MI_VAL_NUMBER:
    {
      return a.as.number == b.as.number;
    }

    case MI_VAL_USER:
    {
      return a.as.user.type == b.as.user.type
          && a.as.user.data == b.as.user.data;
    }
  }

  return false;
}

static bool mi_list_is_valid(const MiList *list)
{
  if (!list)
  {
    return false;
  }

  if (list->magic != MI_LIST_MAGIC)
  {
    return false;
  }

  if (list->destroyed)
  {
    return false;
  }

  if (!list->items)
  {
    return false;
  }

  return true;
}

static MiList *mi_list_from_value(MiValue value)
{
  MiList *list;

  if (value.kind != MI_VAL_USER)
  {
    return NULL;
  }

  if (value.as.user.type != &List_type)
  {
    return NULL;
  }

  list = (MiList*)value.as.user.data;

  if (!mi_list_is_valid(list))
  {
    return NULL;
  }

  return list;
}

static MiExecResult mi_list_eval_arg(MiContext *ctx, MiNode *node, MiValue *out_value)
{
  MiExecResult result;

  result = mi_eval_node(ctx, node);

  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  *out_value = result.value;
  return mi_exec_null();
}

static MiExecResult mi_list_eval_subcommand(MiContext *ctx, MiNode *node, XSlice *out_name)
{
  MiExecResult result;

  result = mi_eval_node(ctx, node);

  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  if (result.value.kind == MI_VAL_STRING)
  {
    *out_name = result.value.as.string;
    return mi_exec_null();
  }

  if (result.value.kind == MI_VAL_RAW)
  {
    *out_name = result.value.as.raw;
    return mi_exec_null();
  }

  mi_context_set_error(ctx, "list subcommand must evaluate to string or raw", 0, 0);
  return mi_exec_error();
}

static MiExecResult mi_list_expect_list(MiContext *ctx, MiValue value, MiList **out_list)
{
  MiList *list;

  list = mi_list_from_value(value);
  if (!list)
  {
    mi_context_set_error(ctx, "expected list", 0, 0);
    return mi_exec_error();
  }

  *out_list = list;
  return mi_exec_null();
}

static MiExecResult mi_list_expect_index(MiContext *ctx, MiValue value, uint32_t *out_index)
{
  double number;
  char buffer[64];
  size_t n;
  char *end;

  if (value.kind == MI_VAL_NUMBER)
  {
    if (value.as.number < 0.0)
    {
      mi_context_set_error(ctx, "list index must be >= 0", 0, 0);
      return mi_exec_error();
    }

    *out_index = (uint32_t)value.as.number;
    return mi_exec_null();
  }

  if (value.kind == MI_VAL_STRING || value.kind == MI_VAL_RAW)
  {
    XSlice slice;

    slice = (value.kind == MI_VAL_STRING) ? value.as.string : value.as.raw;

    n = slice.length;
    if (n == 0 || n >= sizeof(buffer))
    {
      mi_context_set_error(ctx, "list index must be numeric", 0, 0);
      return mi_exec_error();
    }

    memcpy(buffer, slice.ptr, n);
    buffer[n] = '\0';

    number = strtod(buffer, &end);
    if (end == buffer || *end != '\0')
    {
      mi_context_set_error(ctx, "list index must be numeric", 0, 0);
      return mi_exec_error();
    }

    if (number < 0.0)
    {
      mi_context_set_error(ctx, "list index must be >= 0", 0, 0);
      return mi_exec_error();
    }

    *out_index = (uint32_t)number;
    return mi_exec_null();
  }

  mi_context_set_error(ctx, "list index must be a number", 0, 0);
  return mi_exec_error();
}

MiExecResult mi_call_cmd_list_push_value(MiValue list_value, MiValue value)
{
  MiList *list;

  list = mi_list_from_value(list_value);
  if (!list)
  {
    return mi_exec_error();
  }

  XArrayError err = x_array_MiValue_push(list->items, value);
  if (err != XARRAY_OK)
  {
    x_array_MiValue_destroy(list->items);
    free(list);
    return mi_exec_error();
  }
  return mi_exec_ok(mi_value_number(0));
}

MiExecResult mi_list_cmd_create(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiList *list;
  i32 i;

  list = mi_list_create((size_t)(argc > 0 ? argc : 0));
  if (!list)
  {
    mi_context_set_error(ctx, "failed to create list", 0, 0);
    return mi_exec_error();
  }

  for (i = 0; i < argc; i++)
  {
    MiValue value;
    MiExecResult eval_result;
    XArrayError err;

    eval_result = mi_list_eval_arg(ctx, argv[i], &value);
    if (eval_result.signal != MI_SIGNAL_NONE)
    {
      if (list->items)
      {
        x_array_MiValue_destroy(list->items);
      }

      free(list);
      return eval_result;
    }

    err = x_array_MiValue_push(list->items, value);
    if (err != XARRAY_OK)
    {
      x_array_MiValue_destroy(list->items);
      free(list);
      mi_context_set_error(ctx, "failed to push item into list", 0, 0);
      return mi_exec_error();
    }
  }

  {
    MiUser user;

    user.type = &List_type;
    user.data = list;

    return mi_exec_ok(mi_value_user(&List_type, list));
  }
}

MiExecResult mi_list_cmd_len(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiList *list;
  MiExecResult result;

  if (argc != 1)
  {
    mi_context_set_error(ctx, "usage: list len LIST", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  return mi_exec_ok(mi_value_number((double)x_array_MiValue_count(list->items)));
}

MiExecResult mi_list_cmd_get(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiValue index_value;
  MiList *list;
  uint32_t index;
  MiValue *item;
  MiExecResult result;

  if (argc != 2)
  {
    mi_context_set_error(ctx, "usage: list get LIST INDEX", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_eval_arg(ctx, argv[1], &index_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_index(ctx, index_value, &index);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  item = x_array_MiValue_get(list->items, index);
  if (!item)
  {
    mi_context_set_error(ctx, "list index out of bounds", 0, 0);
    return mi_exec_error();
  }

  return mi_exec_ok(*item);
}

MiExecResult mi_list_cmd_set(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiValue index_value;
  MiValue item_value;
  MiList *list;
  uint32_t index;
  MiValue *slot;
  MiExecResult result;

  if (argc != 3)
  {
    mi_context_set_error(ctx, "usage: list set LIST INDEX VALUE", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_eval_arg(ctx, argv[1], &index_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_eval_arg(ctx, argv[2], &item_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_index(ctx, index_value, &index);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  slot = x_array_MiValue_get(list->items, index);
  if (!slot)
  {
    mi_context_set_error(ctx, "list index out of bounds", 0, 0);
    return mi_exec_error();
  }

  *slot = item_value;
  return mi_exec_null();
}

MiExecResult mi_list_cmd_push(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiValue item_value;
  MiList *list;
  MiExecResult result;
  XArrayError err;

  if (argc != 2)
  {
    mi_context_set_error(ctx, "usage: list push LIST VALUE", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_eval_arg(ctx, argv[1], &item_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  err = x_array_MiValue_push(list->items, item_value);
  if (err != XARRAY_OK)
  {
    mi_context_set_error(ctx, "failed to push item into list", 0, 0);
    return mi_exec_error();
  }

  return mi_exec_null();
}

MiExecResult mi_list_cmd_pop(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiList *list;
  MiValue *top;
  MiValue value;
  MiExecResult result;

  if (argc != 1)
  {
    mi_context_set_error(ctx, "usage: list pop LIST", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  top = x_array_MiValue_top(list->items);
  if (!top)
  {
    return mi_exec_null();
  }

  value = *top;
  x_array_MiValue_pop(list->items);
  return mi_exec_ok(value);
}

MiExecResult mi_list_cmd_clear(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiList *list;
  MiExecResult result;

  if (argc != 1)
  {
    mi_context_set_error(ctx, "usage: list clear LIST", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  x_array_MiValue_clear(list->items);
  return mi_exec_null();
}

MiExecResult mi_list_cmd_destroy(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiList *list;
  MiExecResult result;

  if (argc != 1)
  {
    mi_context_set_error(ctx, "usage: list destroy LIST", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  x_array_MiValue_destroy(list->items);
  list->items = NULL;
  list->destroyed = true;

  return mi_exec_null();
}

MiExecResult mi_list_cmd_append(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue dst_value;
  MiValue src_value;
  MiList *dst;
  MiList *src;
  uint32_t count;
  uint32_t i;
  MiExecResult result;

  if (argc != 2)
  {
    mi_context_set_error(ctx, "usage: list append LIST OTHER", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &dst_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_eval_arg(ctx, argv[1], &src_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, dst_value, &dst);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, src_value, &src);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  count = x_array_MiValue_count(src->items);

  for (i = 0; i < count; i++)
  {
    MiValue *item;
    XArrayError err;

    item = x_array_MiValue_get(src->items, i);
    if (!item)
    {
      mi_context_set_error(ctx, "failed to read source list item", 0, 0);
      return mi_exec_error();
    }

    err = x_array_MiValue_push(dst->items, *item);
    if (err != XARRAY_OK)
    {
      mi_context_set_error(ctx, "failed to append list item", 0, 0);
      return mi_exec_error();
    }
  }

  return mi_exec_null();
}

MiExecResult mi_list_cmd_index_of(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiValue list_value;
  MiValue needle_value;
  MiList *list;
  uint32_t count;
  uint32_t i;
  MiExecResult result;

  if (argc != 2)
  {
    mi_context_set_error(ctx, "usage: list index_of LIST VALUE", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_arg(ctx, argv[0], &list_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_eval_arg(ctx, argv[1], &needle_value);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  result = mi_list_expect_list(ctx, list_value, &list);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  count = x_array_MiValue_count(list->items);

  for (i = 0; i < count; i++)
  {
    MiValue *item;

    item = x_array_MiValue_get(list->items, i);
    if (!item)
    {
      break;
    }

    if (mi_list_value_equals(*item, needle_value))
    {
      return mi_exec_ok(mi_value_number((double)i));
    }
  }

  return mi_exec_ok(mi_value_number(-1.0));
}

MiExecResult mi_cmd_list(MiContext *ctx, i32 argc, MiNode **argv)
{
  XSlice subcommand;
  MiExecResult result;

  if (argc < 1)
  {
    mi_context_set_error(ctx, "list requires a subcommand", 0, 0);
    return mi_exec_error();
  }

  result = mi_list_eval_subcommand(ctx, argv[0], &subcommand);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  argc--;
  argv++;

  if (mi_list_slice_equals_cstr(subcommand, "create"))
  {
    return mi_list_cmd_create(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "len"))
  {
    return mi_list_cmd_len(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "get"))
  {
    return mi_list_cmd_get(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "set"))
  {
    return mi_list_cmd_set(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "push"))
  {
    return mi_list_cmd_push(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "pop"))
  {
    return mi_list_cmd_pop(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "clear"))
  {
    return mi_list_cmd_clear(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "destroy"))
  {
    return mi_list_cmd_destroy(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "append"))
  {
    return mi_list_cmd_append(ctx, argc, argv);
  }

  if (mi_list_slice_equals_cstr(subcommand, "index_of"))
  {
    return mi_list_cmd_index_of(ctx, argc, argv);
  }

  mi_context_set_error(ctx, "unknown list subcommand", 0, 0);
  return mi_exec_error();
}

//
// FOREACH
//

MiExecResult mi_cmd_foreach(MiContext *ctx, i32 argc, MiNode **argv)
{
  XSlice var_name;
  MiExecResult result;
  MiValue list_value;
  MiList *list;
  MiScope *loop_scope;
  uint32_t count;
  uint32_t i;

  if (argc != 3)
  {
    mi_context_set_error(ctx, "usage: foreach VARNAME LIST BLOCK", 0, 0);
    return mi_exec_error();
  }

  /*
     argv[0] = var name
     argv[1] = list
     argv[2] = block
     */

  result = mi_eval_node(ctx, argv[0]);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  if (result.value.kind == MI_VAL_STRING)
  {
    var_name = result.value.as.string;
  }
  else if (result.value.kind == MI_VAL_RAW)
  {
    var_name = result.value.as.raw;
  }
  else
  {
    mi_context_set_error(ctx, "foreach variable name must be string or raw", 0, 0);
    return mi_exec_error();
  }

  result = mi_eval_node(ctx, argv[1]);
  if (result.signal != MI_SIGNAL_NONE)
  {
    return result;
  }

  list_value = result.value;
  list = mi_list_from_value(list_value);
  if (!list)
  {
    mi_context_set_error(ctx, "foreach expects a list", 0, 0);
    return mi_exec_error();
  }

  loop_scope = mi_push_scope(ctx);
  if (!loop_scope)
  {
    mi_context_set_error(ctx, "failed to create foreach scope", 0, 0);
    return mi_exec_error();
  }

  if (!mi_scope_define(loop_scope, var_name, mi_value_null()))
  {
    mi_pop_scope(ctx);
    mi_context_set_error(ctx, "failed to define foreach variable", 0, 0);
    return mi_exec_error();
  }

  count = x_array_MiValue_count(list->items);

  for (i = 0; i < count; i++)
  {
    MiValue *item;

    item = x_array_MiValue_get(list->items, i);
    if (!item)
    {
      mi_pop_scope(ctx);
      mi_context_set_error(ctx, "foreach failed to read list item", 0, 0);
      return mi_exec_error();
    }

    if (!mi_scope_set(loop_scope, var_name, *item))
    {
      mi_pop_scope(ctx);
      mi_context_set_error(ctx, "failed to assign foreach variable", 0, 0);
      return mi_exec_error();
    }

    result = mi_exec_block(ctx, argv[2], false);

    if (result.signal == MI_SIGNAL_CONTINUE)
    {
      continue;
    }

    if (result.signal == MI_SIGNAL_BREAK)
    {
      mi_pop_scope(ctx);
      return mi_exec_null();
    }

    if (result.signal == MI_SIGNAL_RETURN)
    {
      mi_pop_scope(ctx);
      return result;
    }

    if (result.signal == MI_SIGNAL_ERROR)
    {
      mi_pop_scope(ctx);
      return result;
    }
  }

  mi_pop_scope(ctx);
  return mi_exec_null();
}

//
// EVAL
//

MiExecResult mi_cmd_eval(MiContext *ctx, int argc, MiNode **argv)
{
  MiExecResult result = mi_exec_null();
  MiExecResult eval_result;
  MiValue src_value;
  XSlice source;
  XArena *arena;
  MiParseResult p_res;

  if (!ctx)
  {
    return mi_exec_error();
  }

  if (argc < 1 || !argv || !argv[0])
  {
    mi_context_set_error(ctx, "eval expects a source argument", 0, 0);
    return mi_exec_error();
  }

  /*
   * Evaluate first argument.
   */
  eval_result = mi_eval_node(ctx, argv[0]);
  if (eval_result.signal == MI_SIGNAL_ERROR)
  {
    return eval_result;
  }

  src_value = eval_result.value;

  if (src_value.kind != MI_VAL_STRING)
  {
    mi_context_set_error(ctx, "eval source must evaluate to a string", 0, 0);
    return mi_exec_error();
  }

  source = src_value.as.string;

  /*
   * Parse source.
   */
  arena = x_arena_create(4096);
  if (!arena)
  {
    mi_context_set_error(ctx, "eval failed to create parse arena", 0, 0);
    return mi_exec_error();
  }

  p_res = mi_parse(arena, source);
  if (!p_res.ok || !p_res.root)
  {
    mi_context_set_error(
        ctx,
        p_res.error_message ? p_res.error_message : "eval parse error",
        p_res.error_line,
        p_res.error_column);

    return mi_exec_error();
  }

  /*
   * Execute program block.
   */
  result = mi_exec_block(ctx, p_res.root, false);

  return result;
}

MiExecResult mi_call_cmd_eval(MiContext *ctx, XSlice source)
{
  MiExecResult result;
  XArena *arena;
  MiParseResult p_res;

  result = mi_exec_null();

  arena = x_arena_create(4096);
  if (!arena)
  {
    mi_context_set_error(ctx, "failed to create parse arena", 0, 0);
    return mi_exec_error();
  }

  p_res = mi_parse(arena, source);
  if (!p_res.ok || !p_res.root)
  {
    mi_context_set_error(
        ctx,
        p_res.error_message ? p_res.error_message : "parse error",
        p_res.error_line,
        p_res.error_column);

    return mi_exec_error();
  }

  result = mi_exec_block(ctx, p_res.root, false);
  return result;
}


//
// INCLUDE
//
MiExecResult mi_cmd_include(MiContext *ctx, int argc, MiNode **argv)
{
  MiExecResult eval_result;
  MiValue path_value;
  XSlice path;
  XFSPath path_cstr;
  FILE *f;
  long fsize;
  size_t size;
  char *file_buf;
  MiExecResult result;

  result = mi_exec_null();
  file_buf = NULL;

  if (!ctx)
  {
    return mi_exec_error();
  }

  if (argc < 1 || !argv || !argv[0])
  {
    mi_context_set_error(ctx, "include expects a path argument", 0, 0);
    return mi_exec_error();
  }

  eval_result = mi_eval_node(ctx, argv[0]);
  if (eval_result.signal == MI_SIGNAL_ERROR)
  {
    return eval_result;
  }

  path_value = eval_result.value;
  if (path_value.kind != MI_VAL_STRING)
  {
    mi_context_set_error(ctx, "include path must evaluate to a string", 0, 0);
    return mi_exec_error();
  }

  path = path_value.as.string;
  x_fs_path_from_slice(path, &path_cstr);

  f = fopen(path_cstr.buf, "rb");
  if (!f)
  {
    mi_context_set_error(ctx, "include could not open file", 0, 0);
    return mi_exec_error();
  }

  if (fseek(f, 0, SEEK_END) != 0)
  {
    fclose(f);
    mi_context_set_error(ctx, "include failed to seek file", 0, 0);
    return mi_exec_error();
  }

  fsize = ftell(f);
  if (fsize < 0)
  {
    fclose(f);
    mi_context_set_error(ctx, "include failed to get file size", 0, 0);
    return mi_exec_error();
  }

  if (fseek(f, 0, SEEK_SET) != 0)
  {
    fclose(f);
    mi_context_set_error(ctx, "include failed to rewind file", 0, 0);
    return mi_exec_error();
  }

  size = (size_t)fsize;
  file_buf = (char *)malloc(size + 1u);
  if (!file_buf)
  {
    fclose(f);
    mi_context_set_error(ctx, "include failed to allocate file buffer", 0, 0);
    return mi_exec_error();
  }

  if (fread(file_buf, 1u, size, f) != size)
  {
    fclose(f);
    free(file_buf);
    mi_context_set_error(ctx, "include failed to read file", 0, 0);
    return mi_exec_error();
  }

  fclose(f);
  file_buf[size] = '\0';

  result = mi_call_cmd_eval(ctx, x_slice_init(file_buf, size));

  return result;
}


//
// Register all builtin commands
//

bool mi_register_builtins(MiContext *ctx)
{
  if (!mi_register_command(ctx, "set", mi_cmd_set))
  {
    return false;
  }

  if (!mi_register_command(ctx, "format", mi_cmd_format))
  {
    return false;
  }

  if (!mi_register_command(ctx, "print", mi_cmd_print))
  {
    return false;
  }

  if (!mi_register_command(ctx, "if", mi_cmd_if))
  {
    return false;
  }

  if (!mi_register_command(ctx, "expr", mi_cmd_expr))
  {
    return false;
  }

  if (!mi_register_command(ctx, "while", mi_cmd_while))
  {
    return false;
  }

  if (!mi_register_command(ctx, "break", mi_cmd_break))
  {
    return false;
  }

  if (!mi_register_command(ctx, "continue", mi_cmd_continue))
  {
    return false;
  }

  if (!mi_register_command(ctx, "list", mi_cmd_list))
  {
    return false;
  }

  if (!mi_register_command(ctx, "foreach", mi_cmd_foreach))
  {
    return false;
  }

  if (!mi_register_command(ctx, "include", mi_cmd_include))
  {
    return false;
  }

  return true;
}
