
#include <stdx_common.h>
#define X_IMPL_STRING
#include <stdx_string.h>
#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_IO
#include <stdx_io.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This structure describes the types of
 * Target a given comment block can apply to.
 */
typedef enum
{
  DOXTER_FUNCTION,
  DOXTER_MACRO,
  DOXTER_STRUCT,
  DOXTER_ENUM,
  DOXTER_TYPEDEF
} DoxterType;

typedef struct
{
  uint32_t line;
  uint32_t column;
  XSlice declaration;
  XSlice target;      // C code of whatever is being documente (without code block). 
  XSlice name;        // identifier extracted from target containing only the name of what is being documented.
  XSlice doc_brief;
  XSlice doc_return;
  XSlice doc_arg[16];
  DoxterType type;
} DoxterComment;

typedef struct
{
  XSlice input;
  uint32_t line;
  uint32_t column;
  char* p;
} Parser;

static void parser_init(Parser* parser, XSlice input)
{
  parser->input = input;
  parser->p = (char*) input.ptr;
  parser->line = 1;
  parser->column = 1;
}

static bool parser_advance(Parser* parser)
{
  if ((parser->p+1) >= (parser->input.ptr + parser->input.length))
    return false;

  parser->p++;
  if (*parser->p == '\n')
  {
    parser->line++;
    parser->column = 0;
  }
  else parser->column++;
  return true;
}

static inline bool parser_is_eof(Parser* parser)
{
  X_ASSERT(parser->input.length > 0);
  return parser->p >= (parser->input.ptr + parser->input.length);
}

static void parser_skip_literal_string(Parser* parser)
{
  if (*parser->p != '"' || parser->p-1 < parser->input.ptr || *(parser->p-1) == '\\')
    return;

  while(parser_advance(parser))
  {
    if (*parser->p == '"' && *(parser->p-1) != '\\')
    {
      parser_advance(parser);
      break;
    }
  }
}

static void parser_skip_single_line_comments(Parser* parser)
{
  if (*(parser->p-1) == '/' && *parser->p == '/')
  {
    while(parser_advance(parser))
    {
      if (*parser->p == '\n')
        break;
    }
  }
}

static bool parser_skip_to_comment_start(Parser* parser)
{
  do
  {
    if (*parser->p == '"' && *(parser->p-1) != '\'')
      parser_skip_literal_string(parser);

    else if (*(parser->p-1) == '/' && *parser->p == '/')
      parser_skip_single_line_comments(parser);

    if (*parser->p == '/')
    {
      if (x_cstr_starts_with(parser->p, "/**"))
      {
        return true;
      }
    }
  } while(parser_advance(parser));
  return false;
}

static bool parser_skip_to_comment_end(Parser* parser)
{
  while (1)
  {
    if (*parser->p == '/' && *(parser->p-1) == '*')
    {
      parser_advance(parser);
      return true;
    }
    parser_advance(parser);
  } 

  return false;
}

static void parser_skip_white_space(Parser* parser)
{
  while (!parser_is_eof(parser) &&
      (*parser->p == ' '   ||
       *parser->p == '\t'  ||
       *parser->p == '\r'  ||
       *parser->p == '\n'))
  {
    parser_advance(parser);
  }
}

static void __parser_skip_macro_body(Parser* parser)
{
  while (*parser->p == '\n' && *(parser->p - 1) != '\\')
  {
    if (!parser_advance(parser))
      return;
  }
}


// Helpers — keep all derefs safe and CRLF-tolerant.

static inline bool parser_peek_prev(Parser* ps, char* out)
{
  if (ps->p > ps->input.ptr)
  {
    *out = *(ps->p - 1);
    return true;
  }
  return false;
}

static void parser_consume_to_eol(Parser* ps)
{
  while (!parser_is_eof(ps))
  {
    char c = *ps->p;
    if (c == '\n')
    {
      parser_advance(ps); // step onto the next line
      break;
    }
    parser_advance(ps);
  }
}

// Returns true if the just-consumed line (ending at '\n' or EOF) was backslash-continued.
// Assumes parser->p is positioned at the '\n' of the line we just finished,
// or at EOF (points to end sentinel).
static bool last_line_was_continued(Parser* ps, const char* line_start)
{
  const char* cur = ps->p;

  // cur is at '\n' or end; back up one if at '\n'
  if (cur > line_start && *(cur - 1) == '\n') cur--;

  // handle optional '\r'
  if (cur > line_start && *(cur - 1) == '\r') cur--;

  // skip trailing spaces/tabs
  while (cur > line_start && ((*(cur - 1) == ' ') || (*(cur - 1) == '\t')))
    cur--;

  return (cur > line_start && *(cur - 1) == '\\');
}

// Robust skipper for #define bodies (handles multi-line with backslash continuation).
static void parser_skip_macro_body(Parser* ps)
{
  // 1) Finish the current line that starts with '#define'
  const char* line_start = ps->p;
  parser_consume_to_eol(ps);

  // 2) While the previous logical line ended with a backslash, keep consuming lines
  while (!parser_is_eof(ps))
  {
    if (!last_line_was_continued(ps, line_start))
      break;

    // we are at the start of the *next* continued line; remember its start
    line_start = ps->p;
    parser_consume_to_eol(ps);
  }
}



static void parser_skip_to_matching_char(Parser* parser)
{
  if (parser_is_eof(parser))
    return;

  const char c = *parser->p;
  char matching = '\0';
  if (*parser->p == '(' ) matching = ')';
  else if ( *parser->p == '{' ) matching = '}';
  else if ( *parser->p == '\'' ) matching = '\'';
  else if ( *parser->p == '\"' ) matching = '\"';

  else return;

  if (!parser_advance(parser))
    return;

  while (true)
  {
    // Skip any escaped character
    if (*parser->p == '\\') 
    {
      parser_advance(parser);
      parser_advance(parser);
      continue;
    }

    // Skip single line comment
    if (*(parser->p-1) == '/' && *parser->p == '/')
    {
      parser_skip_single_line_comments(parser);
      continue;
    }

    if (*parser->p == matching)
      break;

    // ignore if the char appears as a string or as a char dfinition
    if ((*parser->p == '\'' && c != '\'') || (*parser->p == '\"' && c != '\"'))
      parser_skip_to_matching_char(parser);
    // If the opening char appears again, skip the nesting
    else if (*parser->p == c)
      parser_skip_to_matching_char(parser);
    else if (!parser_advance(parser))
      return;
  }
  parser_advance(parser);
}

static bool parser_get_block_comment_target(Parser* parser, DoxterComment* out_comment)
{
  // This will break stuff {
  out_comment->name = x_slice_empty();
  parser_skip_white_space(parser);
  XSlice target_name;
  char* name_start = parser->p;
  while(!parser_is_eof(parser))
  {
    parser_skip_white_space(parser);
    if (*parser->p == '(')
    {
      // function declaration, definition or macro
      parser_skip_to_matching_char(parser);
      parser_skip_white_space(parser);
      target_name = x_slice_init(name_start, parser->p - name_start);

      if (*parser->p == ';')
      {
        // function declaration
        out_comment->type = DOXTER_FUNCTION;
        out_comment->target = target_name;
        return true;
      }
      else if (*parser->p == '{')
      {
        // function definition
        parser_skip_to_matching_char(parser);
        out_comment->type = DOXTER_FUNCTION;
        out_comment->target = target_name;
        return true;
      }
      else if (x_slice_starts_with_cstr(target_name, "#define"))
      {
        // It is function like macro.
        parser_skip_macro_body(parser);
        target_name.ptr +=7;
        target_name.length -=7;
        target_name = x_slice_trim(target_name);
        out_comment->type = DOXTER_MACRO;
        out_comment->target = target_name;
        return true;
      }
      else 
      {
        if (!parser_is_eof(parser) && *parser->p == '(')
          continue;
      }

      printf("OOOOPS!\n");

      return false;
    }
    else if (*parser->p == '{')
    {
      parser_skip_to_matching_char(parser);
      parser_skip_white_space(parser);

      while(*parser->p != ';')
      {
        if (!parser_advance(parser))
          return false;
      }
      parser_advance(parser);
      target_name = x_slice_init(name_start, parser->p - name_start);
      target_name = x_slice_trim(target_name);

      if (x_slice_starts_with_cstr(target_name, "typedef"))
      {
        target_name.ptr += 8;
        target_name.length -= 8;
        parser_skip_white_space(parser);
      }

      if (x_slice_starts_with_cstr(target_name, "enum"))
      {
        out_comment->type = DOXTER_ENUM;
        out_comment->target = target_name;
        return true;
      }
      else if (x_slice_starts_with_cstr(target_name, "struct"))
      {
        out_comment->type = DOXTER_STRUCT;
        out_comment->target = target_name;
        return true;
      }

      return false;
    }

    parser_advance(parser);
  }
  return false;
}

static bool parser_get_block_comment(Parser* parser, DoxterComment* out_comment)
{
  if (parser_skip_to_comment_start(parser))
  {
    out_comment->line = parser->line;
    out_comment->column = parser->column;
    char* start = parser->p;

    if (parser_skip_to_comment_end(parser))
    {
      out_comment->declaration = x_slice_init(start, parser->p - start);
      return parser_get_block_comment_target(parser, out_comment);
    }
  }

  return false;
}

void doxter_show_usage(void)
{
  printf("Usage: doxter <source_file>\n");
}

const char* doxter_type_to_string(DoxterType type)
{
  switch(type)
  {
    case DOXTER_FUNCTION: return "Function";
    case DOXTER_MACRO: return "Macro";
    case DOXTER_STRUCT: return "Struct";
    case DOXTER_ENUM: return "Enum";
    case DOXTER_TYPEDEF: return "Typedef";
    default: return "Unknown";
  }
}

bool doxter_comment_parse_next(XSlice* comment)
{
  char buf[1024];

  if (comment->length == 0)
    return false;

  *comment = x_slice_trim(*comment);

  // Skip comment block start
  if (x_slice_starts_with_cstr(*comment, "/**"))
  {
    comment->ptr += 3;
    comment->length -= 3;
    comment->length -= 2; // remove ending '*/'
  }

  // Skip possible empty space at the start of a line
  *comment = x_slice_trim(*comment);

  // Skip '*' at the start of a line
  while (x_slice_starts_with_cstr(*comment, "*"))
  {
    comment->ptr++;
    comment->length--;
  }


  // Skip possible empty space at the start of a line
  *comment = x_slice_trim(*comment);

  if ((!x_slice_starts_with_cstr(*comment, "@")) || x_slice_starts_with_cstr(*comment, "@text ") || x_slice_starts_with_cstr(*comment, "@text\t"))
  {
    XSlice tag = x_slice("@text");
    snprintf(buf, sizeof(buf), "'%.*s' ", (uint32_t) tag.length, tag.ptr);
  }
  else if (x_slice_starts_with_cstr(*comment, "@param ") || x_slice_starts_with_cstr(*comment, "@param\t"))
  {
    XSlice tag = x_slice("@text");
    XSlice name = x_slice_empty();
    x_slice_next_token_white_space(comment, &tag);
    x_slice_next_token_white_space(comment, &name);
    snprintf(buf, sizeof(buf), "'%.*s' - '%.*s' ",
        (uint32_t) tag.length, tag.ptr,
        (uint32_t) name.length, name.ptr);
  }
  else if (x_slice_starts_with_cstr(*comment, "@return ") || x_slice_starts_with_cstr(*comment, "@return\t"))
  {
    XSlice tag = x_slice("@text");
    x_slice_next_token_white_space(comment, &tag);
    snprintf(buf, sizeof(buf), "'%.*s' ", (int32_t) tag.length, tag.ptr);
  }
  else
  {
    //unknown tag
    XSlice tag = x_slice("UNKNOWN");
    snprintf(buf, sizeof(buf), "'%.*s' ", (uint32_t) tag.length, tag.ptr);
    return false;
  }

  *comment = x_slice_trim_left(*comment);

  char* desc = (char*) comment->ptr;
  char* end = (char*) comment->ptr + comment->length;
  char* p = (char*) desc;

  bool start_of_line = false;
  while (*p != '@' && p < end)
  {
    if (*p == '\n')
    {
      start_of_line = true;
    }
    else if (start_of_line && (*p == '\r' || *p == '\t' || *p == ' ' || *p == '*'))
    {
      *p = ' ';
    }
    else if (start_of_line && *p == '@')
    {
      break;
    }
    else
    {
      start_of_line = false;
    }
    p++;
  }

  size_t desc_size = (p - desc);
  comment->ptr = p;
  comment->length -= desc_size;

  printf("%s -> %.*s\n", buf, (uint32_t) desc_size, desc);
  return true;
}

/**
 * Main function
 * @param argc number or arguments.
 * @param argv array of arguments
 * @return 0 on success, non zero otherwise
 */
int main(int argc, char** argv)
{
  if (argc == 0)
  {
    doxter_show_usage();
    return 0;
  }
  else if (argc != 2)
  {
    fprintf(stderr, "Invalid arguments");
    doxter_show_usage();
    return 1;
  }

  size_t file_size;
  char* input = x_io_read_text(argv[1], &file_size);

  Parser parser;
  parser_init(&parser, x_slice_init(input, file_size));
  DoxterComment comment = {0};
  while (parser_get_block_comment(&parser, &comment))
  {
    printf("Comment start at line %d, column %d\n%.*s\n\n",
        comment.line, comment.column,
        (uint32_t) comment.declaration.length, comment.declaration.ptr);

    while (doxter_comment_parse_next(&comment.declaration)){}

    printf("Comment start at line %d, column %d\nType = %s\nComment =\n%.*s\nName =\n%.*s\nTarget =\n%.*s\n\n",
        comment.line, comment.column,
        doxter_type_to_string(comment.type),
        (uint32_t) comment.declaration.length, comment.declaration.ptr,
        (uint32_t) comment.name.length, comment.name.ptr,
        (uint32_t) comment.target.length, comment.target.ptr);
  }

  free(input);
  return 0;
}
