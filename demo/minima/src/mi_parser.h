#ifndef MI_PARSER_H
#define MI_PARSER_H

#include "stdx_arena.h"
#include "stdx_string.h"

typedef enum MiNodeKind
{
  MI_NODE_COMMAND,
  MI_NODE_STRING,
  MI_NODE_RAW,
  MI_NODE_VARIABLE,
  MI_NODE_SUBCOMMAND,
  MI_NODE_BLOCK,
} MiNodeKind;

typedef struct MiNode MiNode;

struct MiNode
{
  MiNodeKind kind;
  XSlice text;
  int line;
  int column;
  MiNode *first_child;
  MiNode *last_child;
  MiNode *next_sibling;
};

typedef struct MiParseResult
{
  int ok;
  MiNode *root;
  const char *error_message;
  int error_line;
  int error_column;
} MiParseResult;

MiParseResult mi_parse(XArena *arena, XSlice source);
void mi_debug_print_ast(MiNode *node, int indent);

#endif /* MI_PARSER_H */
