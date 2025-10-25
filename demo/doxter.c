#include <stdx_common.h>
#define STDX_IMPLEMENTATION_STRING
#include <stdx_string.h>
#define STDX_IMPLEMENTATION_TEST
#include <stdx_test.h>
#define STDX_IMPLEMENTATION_IO
#include <stdx_io.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
  XStrview input;
  uint32_t line;
  uint32_t column;
  char* p;
} Parser;

static void parser_init(Parser* parser, XStrview input)
{
  parser->input = input;
  parser->p = (char*) input.data;
  parser->line = 1;
  parser->column = 1;
}

static bool parser_advance(Parser* parser)
{
  if ((parser->p+1) >= (parser->input.data + parser->input.length))
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

static bool parser_is_eof(Parser* parser)
{
  X_ASSERT(parser->input.length > 0);
  return parser->p < (parser->input.data + parser->input.length - 1);
}

static void parser_skip_literal_string(Parser* parser)
{
  if (*parser->p != '"' || parser->p-1 < parser->input.data || *(parser->p-1) == '\\')
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

static bool parser_skip_to_comment_start(Parser* parser)
{
  do
  {
    if (*parser->p == '"' && *(parser->p-1) != '\'')
      parser_skip_literal_string(parser);

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

typedef struct
{
  uint32_t line;
  uint32_t column;
  XStrview text;
} DoxterComment;

static bool parser_get_block_comment(Parser* parser, DoxterComment* out_comment)
{

  if (parser_skip_to_comment_start(parser))
  {
    out_comment->line = parser->line;
    out_comment->column = parser->column;
    char* start = parser->p;

    if (parser_skip_to_comment_end(parser))
    {
      out_comment->text = x_strview_init(start, parser->p - start);

      /*
       * TODO: Before returning true, need to parse the code that comes after
       * the comment block and make sure it mathces the comment block.
       * For example if it contains @param or @return but ut comes before a
       * struct we should warn. Because the DoxterComment will have a Type
       * member where we gonna say it's a function, macro, enum or whatever.

      return true;
    }
  }

  return false;
}

void doxter_show_usage(void)
{
  printf("Usage: doxter <source_file>\n");
}

/**
 * Main function
 *
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
  char* input = x_io_read_text(argv[1], &file_size, NULL);

  Parser parser;
  parser_init(&parser, x_strview_init(input, file_size));
  DoxterComment comment;
  while (parser_get_block_comment(&parser, &comment))
    printf("Comment start at line %d, column %d\n%.*s\n",
        comment.line,
        comment.column,
        comment.text.length, comment.text.data);

  free(input);
  return 0;
}
