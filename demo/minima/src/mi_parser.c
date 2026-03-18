#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "minima.h"

typedef enum MiTokenKind
{
  MI_TOKEN_EOF,
  MI_TOKEN_NEWLINE,
  MI_TOKEN_SEMICOLON,
  MI_TOKEN_LPAREN,
  MI_TOKEN_RPAREN,
  MI_TOKEN_LBRACE,
  MI_TOKEN_RBRACE,
  MI_TOKEN_STRING,
  MI_TOKEN_RAW,
  MI_TOKEN_VARIABLE,
  MI_TOKEN_ERROR,
} MiTokenKind;

typedef struct MiToken
{
  MiTokenKind kind;
  XSlice text;
  int line;
  int column;
} MiToken;

typedef struct MiParser
{
  XArena *arena;
  XSlice source;
  size_t index;
  int line;
  int column;

  bool has_peek;
  MiToken peek;

  bool has_error;
  const char *error_message;
  int error_line;
  int error_column;
} MiParser;

static XSlice mi_slice_make(const char *ptr, size_t length)
{
  XSlice s;

  s.ptr = ptr;
  s.length = length;
  return s;
}

static bool mi_is_space(char c)
{
  return c == ' ' || c == '\t' || c == '\r';
}

static bool mi_is_name_start(char c)
{
  return (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         c == '_';
}

static bool mi_is_name_continue(char c)
{
  return mi_is_name_start(c) || (c >= '0' && c <= '9');
}

static char mi_parser_curr(MiParser *p)
{
  if (p->index >= p->source.length)
  {
    return '\0';
  }

  return p->source.ptr[p->index];
}

static char mi_parser_peek_char(MiParser *p, size_t offset)
{
  size_t i;

  i = p->index + offset;

  if (i >= p->source.length)
  {
    return '\0';
  }

  return p->source.ptr[i];
}

static void mi_parser_advance(MiParser *p)
{
  char c;

  if (p->index >= p->source.length)
  {
    return;
  }

  c = p->source.ptr[p->index];
  p->index += 1;

  if (c == '\n')
  {
    p->line += 1;
    p->column = 1;
  }
  else
  {
    p->column += 1;
  }
}

static void mi_parser_set_error(MiParser *p, int line, int column, const char *message)
{
  size_t len;
  char *copy;

  if (p->has_error)
  {
    return;
  }

  p->has_error = true;
  p->error_line = line;
  p->error_column = column;

  len = strlen(message);
  copy = (char *)x_arena_alloc(p->arena, len + 1);

  if (copy)
  {
    memcpy(copy, message, len + 1);
    p->error_message = copy;
  }
  else
  {
    p->error_message = "out of memory";
  }
}

static MiNode *mi_node_new(MiParser *p, MiNodeKind kind, XSlice text, int line, int column)
{
  MiNode *node;

  node = (MiNode *)x_arena_alloc_zero(p->arena, sizeof(MiNode));

  if (!node)
  {
    mi_parser_set_error(p, line, column, "out of memory");
    return NULL;
  }

  node->kind = kind;
  node->text = text;
  node->line = line;
  node->column = column;
  return node;
}

static void mi_node_append_child(MiNode *parent, MiNode *child)
{
  if (!parent || !child)
  {
    return;
  }

  if (!parent->first_child)
  {
    parent->first_child = child;
    parent->last_child = child;
    return;
  }

  parent->last_child->next_sibling = child;
  parent->last_child = child;
}

static void mi_parser_skip_comment(MiParser *p)
{
  while (true)
  {
    char c;

    c = mi_parser_curr(p);

    if (c == '\0' || c == '\n')
    {
      break;
    }

    mi_parser_advance(p);
  }
}

static void mi_parser_skip_horizontal_ws(MiParser *p)
{
  while (mi_is_space(mi_parser_curr(p)))
  {
    mi_parser_advance(p);
  }
}

static void mi_parser_skip_ignored(MiParser *p)
{
  while (true)
  {
    mi_parser_skip_horizontal_ws(p);

    if (mi_parser_curr(p) == '#')
    {
      mi_parser_skip_comment(p);
      continue;
    }

    break;
  }
}

static MiToken mi_parser_next_token_raw(MiParser *p)
{
  MiToken token;
  char c;
  const char *start;
  size_t start_index;
  int line;
  int column;

  token.kind = MI_TOKEN_EOF;
  token.text = mi_slice_make(NULL, 0);
  token.line = p->line;
  token.column = p->column;

  mi_parser_skip_ignored(p);

  c = mi_parser_curr(p);
  line = p->line;
  column = p->column;

  if (c == '\0')
  {
    token.kind = MI_TOKEN_EOF;
    token.line = line;
    token.column = column;
    return token;
  }

  if (c == '\n')
  {
    token.kind = MI_TOKEN_NEWLINE;
    token.text = mi_slice_make(p->source.ptr + p->index, 1);
    token.line = line;
    token.column = column;
    mi_parser_advance(p);
    return token;
  }

  if (c == ';')
  {
    token.kind = MI_TOKEN_SEMICOLON;
    token.text = mi_slice_make(p->source.ptr + p->index, 1);
    token.line = line;
    token.column = column;
    mi_parser_advance(p);
    return token;
  }

  if (c == '(')
  {
    token.kind = MI_TOKEN_LPAREN;
    token.text = mi_slice_make(p->source.ptr + p->index, 1);
    token.line = line;
    token.column = column;
    mi_parser_advance(p);
    return token;
  }

  if (c == ')')
  {
    token.kind = MI_TOKEN_RPAREN;
    token.text = mi_slice_make(p->source.ptr + p->index, 1);
    token.line = line;
    token.column = column;
    mi_parser_advance(p);
    return token;
  }

  if (c == '{')
  {
    token.kind = MI_TOKEN_LBRACE;
    token.text = mi_slice_make(p->source.ptr + p->index, 1);
    token.line = line;
    token.column = column;
    mi_parser_advance(p);
    return token;
  }

  if (c == '}')
  {
    token.kind = MI_TOKEN_RBRACE;
    token.text = mi_slice_make(p->source.ptr + p->index, 1);
    token.line = line;
    token.column = column;
    mi_parser_advance(p);
    return token;
  }

  if (c == '[' || c == ']')
  {
    token.kind = MI_TOKEN_RAW;
    token.text = mi_slice_make(p->source.ptr + p->index, 1);
    token.line = line;
    token.column = column;
    mi_parser_advance(p);
    return token;
  }

  if (c == '"')
  {
    start = p->source.ptr + p->index;
    start_index = p->index;
    mi_parser_advance(p);

    while (true)
    {
      c = mi_parser_curr(p);

      if (c == '\0')
      {
        token.kind = MI_TOKEN_ERROR;
        token.text = mi_slice_make(start, p->index - start_index);
        token.line = line;
        token.column = column;
        mi_parser_set_error(p, line, column, "unterminated string literal");
        return token;
      }

      //if (c == '\n')
      //{
      //  token.kind = MI_TOKEN_ERROR;
      //  token.text = mi_slice_make(start, p->index - start_index);
      //  token.line = line;
      //  token.column = column;
      //  mi_parser_set_error(p, line, column, "newline in string literal");
      //  return token;
      //}

      if (c == '\\')
      {
        mi_parser_advance(p);

        if (mi_parser_curr(p) == '\0')
        {
          token.kind = MI_TOKEN_ERROR;
          token.text = mi_slice_make(start, p->index - start_index);
          token.line = line;
          token.column = column;
          mi_parser_set_error(p, line, column, "unterminated string escape");
          return token;
        }

        mi_parser_advance(p);
        continue;
      }

      if (c == '"')
      {
        mi_parser_advance(p);
        token.kind = MI_TOKEN_STRING;
        token.text = mi_slice_make(start, p->index - start_index);
        token.line = line;
        token.column = column;
        return token;
      }

      mi_parser_advance(p);
    }
  }

  if (c == '$')
  {
    if (!mi_is_name_start(mi_parser_peek_char(p, 1)))
    {
      token.kind = MI_TOKEN_ERROR;
      token.text = mi_slice_make(p->source.ptr + p->index, 1);
      token.line = line;
      token.column = column;
      mi_parser_set_error(p, line, column, "expected variable name after '$'");
      mi_parser_advance(p);
      return token;
    }

    start = p->source.ptr + p->index;
    start_index = p->index;
    mi_parser_advance(p);

    while (mi_is_name_continue(mi_parser_curr(p)))
    {
      mi_parser_advance(p);
    }

    token.kind = MI_TOKEN_VARIABLE;
    token.text = mi_slice_make(start, p->index - start_index);
    token.line = line;
    token.column = column;
    return token;
  }

  start = p->source.ptr + p->index;
  start_index = p->index;

  while (true)
  {
    c = mi_parser_curr(p);

    if (c == '\0')
    {
      break;
    }

    if (mi_is_space(c) ||
        c == '\n' ||
        c == ';' ||
        c == '(' ||
        c == ')' ||
        c == '{' ||
        c == '}' ||
        c == '[' ||
        c == ']' ||
        c == '"' ||
        c == '#')
    {
      break;
    }

    mi_parser_advance(p);
  }

  token.kind = MI_TOKEN_RAW;
  token.text = mi_slice_make(start, p->index - start_index);
  token.line = line;
  token.column = column;
  return token;
}

static MiToken mi_parser_peek_token(MiParser *p)
{
  if (!p->has_peek)
  {
    p->peek = mi_parser_next_token_raw(p);
    p->has_peek = true;
  }

  return p->peek;
}

static MiToken mi_parser_next_token(MiParser *p)
{
  MiToken token;

  if (p->has_peek)
  {
    p->has_peek = false;
    return p->peek;
  }

  token = mi_parser_next_token_raw(p);
  return token;
}

static bool mi_parser_match(MiParser *p, MiTokenKind kind, MiToken *out_token)
{
  MiToken token;

  token = mi_parser_peek_token(p);

  if (token.kind != kind)
  {
    return false;
  }

  token = mi_parser_next_token(p);

  if (out_token)
  {
    *out_token = token;
  }

  return true;
}

static void mi_parser_skip_separators(MiParser *p)
{
  MiToken token;

  while (true)
  {
    token = mi_parser_peek_token(p);

    if (token.kind != MI_TOKEN_NEWLINE && token.kind != MI_TOKEN_SEMICOLON)
    {
      break;
    }

    mi_parser_next_token(p);
  }
}

static MiNode *mi_parse_command(MiParser *p);
static MiNode *mi_parse_argument(MiParser *p);
static MiNode *mi_parse_block(MiParser *p);
static MiNode *mi_parse_subcommand(MiParser *p);

static bool mi_token_starts_argument(MiTokenKind kind)
{
  return kind == MI_TOKEN_STRING ||
         kind == MI_TOKEN_RAW ||
         kind == MI_TOKEN_VARIABLE ||
         kind == MI_TOKEN_LPAREN ||
         kind == MI_TOKEN_LBRACE;
}

static MiNode *mi_parse_argument(MiParser *p)
{
  MiToken token;
  MiNode *node;
  XSlice text;

  token = mi_parser_peek_token(p);

  if (p->has_error)
  {
    return NULL;
  }

  switch (token.kind)
  {
    case MI_TOKEN_STRING:
      token = mi_parser_next_token(p);
      text = token.text;

      if (text.length >= 2)
      {
        text.ptr += 1;
        text.length -= 2;
      }
      else
      {
        text = mi_slice_make(NULL, 0);
      }

      node = mi_node_new(p, MI_NODE_STRING, text, token.line, token.column);
      return node;

    case MI_TOKEN_RAW:
      token = mi_parser_next_token(p);
      node = mi_node_new(p, MI_NODE_RAW, token.text, token.line, token.column);
      return node;

    case MI_TOKEN_VARIABLE:
      token = mi_parser_next_token(p);
      text = token.text;

      if (text.length >= 1)
      {
        text.ptr += 1;
        text.length -= 1;
      }
      else
      {
        text = mi_slice_make(NULL, 0);
      }

      node = mi_node_new(p, MI_NODE_VARIABLE, text, token.line, token.column);
      return node;

    case MI_TOKEN_LPAREN:
      return mi_parse_subcommand(p);

    case MI_TOKEN_LBRACE:
      return mi_parse_block(p);

    default:
      mi_parser_set_error(p, token.line, token.column, "expected argument");
      return NULL;
  }
}

static MiNode *mi_parse_command(MiParser *p)
{
  MiToken token;
  MiNode *command;
  MiNode *arg;

  token = mi_parser_peek_token(p);

  if (!mi_token_starts_argument(token.kind))
  {
    return NULL;
  }

  command = mi_node_new(p, MI_NODE_COMMAND, mi_slice_make(NULL, 0), token.line, token.column);

  if (!command)
  {
    return NULL;
  }

  while (true)
  {
    token = mi_parser_peek_token(p);

    if (!mi_token_starts_argument(token.kind))
    {
      break;
    }

    arg = mi_parse_argument(p);

    if (!arg)
    {
      return NULL;
    }

    mi_node_append_child(command, arg);
  }

  return command;
}

static MiNode *mi_parse_subcommand(MiParser *p)
{
  MiToken open_tok;
  MiToken close_tok;
  MiToken token;
  MiNode *sub;
  MiNode *command;

  if (!mi_parser_match(p, MI_TOKEN_LPAREN, &open_tok))
  {
    token = mi_parser_peek_token(p);
    mi_parser_set_error(p, token.line, token.column, "expected '('");
    return NULL;
  }

  sub = mi_node_new(p, MI_NODE_SUBCOMMAND, mi_slice_make(NULL, 0), open_tok.line, open_tok.column);

  if (!sub)
  {
    return NULL;
  }

  mi_parser_skip_separators(p);
  command = mi_parse_command(p);

  if (!command)
  {
    token = mi_parser_peek_token(p);

    if (token.kind == MI_TOKEN_RPAREN)
    {
      mi_parser_set_error(p, token.line, token.column, "subcommand cannot be empty");
    }
    else
    {
      mi_parser_set_error(p, token.line, token.column, "expected exactly one command inside '()'");
    }

    return NULL;
  }

  mi_node_append_child(sub, command);
  mi_parser_skip_separators(p);
  token = mi_parser_peek_token(p);

  if (token.kind != MI_TOKEN_RPAREN)
  {
    if (mi_token_starts_argument(token.kind))
    {
      mi_parser_set_error(p, token.line, token.column, "subcommand must contain exactly one command");
    }
    else
    {
      mi_parser_set_error(p, token.line, token.column, "expected ')' to close subcommand");
    }

    return NULL;
  }

  mi_parser_match(p, MI_TOKEN_RPAREN, &close_tok);
  (void)close_tok;
  return sub;
}

static MiNode *mi_parse_block(MiParser *p)
{
  MiToken open_tok;
  MiToken token;
  MiNode *block;
  MiNode *command;

  if (!mi_parser_match(p, MI_TOKEN_LBRACE, &open_tok))
  {
    token = mi_parser_peek_token(p);
    mi_parser_set_error(p, token.line, token.column, "expected '{'");
    return NULL;
  }

  block = mi_node_new(p, MI_NODE_BLOCK, mi_slice_make(NULL, 0), open_tok.line, open_tok.column);

  if (!block)
  {
    return NULL;
  }

  mi_parser_skip_separators(p);

  while (true)
  {
    token = mi_parser_peek_token(p);

    if (token.kind == MI_TOKEN_RBRACE)
    {
      mi_parser_next_token(p);
      return block;
    }

    if (token.kind == MI_TOKEN_EOF)
    {
      mi_parser_set_error(p, open_tok.line, open_tok.column, "unterminated block");
      return NULL;
    }

    command = mi_parse_command(p);

    if (!command)
    {
      mi_parser_set_error(p, token.line, token.column, "expected command inside block");
      return NULL;
    }

    mi_node_append_child(block, command);
    mi_parser_skip_separators(p);
  }
}

static MiNode *mi_parse_program(MiParser *p)
{
  MiNode *root;
  MiNode *command;
  MiToken token;

  root = mi_node_new(p, MI_NODE_BLOCK, mi_slice_make(NULL, 0), 1, 1);

  if (!root)
  {
    return NULL;
  }

  mi_parser_skip_separators(p);

  while (true)
  {
    token = mi_parser_peek_token(p);

    if (token.kind == MI_TOKEN_EOF)
    {
      return root;
    }

    if (token.kind == MI_TOKEN_RPAREN)
    {
      mi_parser_set_error(p, token.line, token.column, "unexpected ')' at top level");
      return NULL;
    }

    if (token.kind == MI_TOKEN_RBRACE)
    {
      mi_parser_set_error(p, token.line, token.column, "unexpected '}' at top level");
      return NULL;
    }

    command = mi_parse_command(p);

    if (!command)
    {
      mi_parser_set_error(p, token.line, token.column, "expected command");
      return NULL;
    }

    mi_node_append_child(root, command);
    mi_parser_skip_separators(p);
  }
}

MiParseResult mi_parse(XArena *arena, XSlice source)
{
  MiParser p;
  MiParseResult result;
  MiNode *root;

  memset(&p, 0, sizeof(p));
  p.arena = arena;
  p.source = source;
  p.index = 0;
  p.line = 1;
  p.column = 1;

  memset(&result, 0, sizeof(result));
  result.ok = false;
  result.root = NULL;
  result.error_message = NULL;
  result.error_line = 0;
  result.error_column = 0;

  root = mi_parse_program(&p);

  if (p.has_error)
  {
    result.ok = false;
    result.root = NULL;
    result.error_message = p.error_message ? p.error_message : "parse error";
    result.error_line = p.error_line;
    result.error_column = p.error_column;
    return result;
  }

  if (!root)
  {
    result.ok = false;
    result.root = NULL;
    result.error_message = "parse error";
    result.error_line = p.line;
    result.error_column = p.column;
    return result;
  }

  result.ok = true;
  result.root = root;
  result.error_message = NULL;
  result.error_line = 0;
  result.error_column = 0;
  return result;
}

/*
  Optional tiny debug printer.
  Useful while validating the parser before writing the executor.
*/

static const char *mi_node_kind_name(MiNodeKind kind)
{
  switch (kind)
  {
    case MI_NODE_COMMAND:
      return "COMMAND";
    case MI_NODE_STRING:
      return "STRING";
    case MI_NODE_RAW:
      return "RAW";
    case MI_NODE_VARIABLE:
      return "VARIABLE";
    case MI_NODE_SUBCOMMAND:
      return "SUBCOMMAND";
    case MI_NODE_BLOCK:
      return "BLOCK";
    default:
      return "?";
  }
}

void mi_debug_print_ast(MiNode *node, int indent)
{
  MiNode *child;
  int i;

  if (!node)
  {
    return;
  }

  for (i = 0; i < indent; i++)
  {
    printf("  ");
  }

  printf("%s", mi_node_kind_name(node->kind));

  if (node->text.ptr && node->text.length > 0)
  {
    printf(" : %.*s", (int)node->text.length, node->text.ptr);
  }

  printf("\n");

  child = node->first_child;

  while (child)
  {
    mi_debug_print_ast(child, indent + 1);
    child = child->next_sibling;
  }
}
