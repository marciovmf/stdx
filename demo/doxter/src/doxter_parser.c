#include "doxter.h"
#include <stdx_array.h>
#include <stdx_string.h>
#include <stdx_io.h>
#include <stdio.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct DoxterTokenizer
{
  XSlice input;
  uint32_t line;
  uint32_t column;
  bool at_bol;                // beginning of line (after newline)
  bool in_pp;                 // currently tokenizing a preprocessor directive
  bool pp_line_continuation;  // last returned token was "\" while in_pp
} DoxterTokenizer;

static XSlice s_cleanup_comment(XSlice comment)
{
  comment = x_slice_trim(comment);
  if (x_slice_starts_with_cstr(comment, "/**"))
  {
    comment.ptr += 3;
    comment.length -= 3;
  }

  if (x_slice_ends_with_cstr(comment, "*/"))
  {
    comment.length -= 2;
  }

  XSlice c = comment;
  XSlice line;

  while (x_slice_next_token(&c, '\n', &line))
  {
    const char* p = line.ptr;
    const char* end = line.ptr + line.length;
    size_t offset = 0;
    while(p < end && (*p == ' ' || *p == '\t' || *p == '\r'))
    {
      offset++;
      p++;
    }

    while(p < end && *p == '*')
    {
      offset++;
      p++;
    }

    if(p < end && *p == ' ')
    {
      offset++;
      p++;
    }

    size_t line_len = line.length - offset;
    memmove((void*) line.ptr, p, line_len);

    {
      char* p = (char*) (line.ptr + line_len);
      const char* end = line.ptr + line.length;
      while (p < end)
      {
        *p = ' ';
        p++;
      }
    }
  }

  return comment;
}

static bool s_char_is_space(char c)
{
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

static bool s_char_is_ident_start(char c)
{
  if (c >= 'a' && c <= 'z') return true;
  if (c >= 'A' && c <= 'Z') return true;
  if (c == '_') return true;
  return false;
}

static bool s_char_is_ident_char(char c)
{
  if (s_char_is_ident_start(c)) return true;
  if (c >= '0' && c <= '9') return true;
  return false;
}

static void s_tokenizer_advance(DoxterTokenizer* s, size_t n)
{
  if (n > s->input.length)
  {
    n = s->input.length;
  }

  for (size_t i = 0; i < n; i++)
  {
    char c = s->input.ptr[i];
    if (c == '\n')
    {
      s->line++;
      s->column = 1;
      s->at_bol = true;
    }
    else
    {
      s->column++;
    }
  }

  s->input.ptr += n;
  s->input.length -= n;
}

/* Skips whitespace and non-doc comments. Updates at_bol/in_pp state. */
static void s_tokenizer_skip_trivia(DoxterTokenizer* s)
{
  for (;;)
  {
    if (s->input.length == 0)
    {
      return;
    }

    /* In PP mode, a newline can end the directive. We handle that in next_token. */
    char c = s->input.ptr[0];

    /* Whitespace */
    if (s_char_is_space(c))
    {
      if (c == '\n')
      {
        if (s->in_pp && !s->pp_line_continuation)
        {
          return;
        }
        s->at_bol = true;
      }

      s_tokenizer_advance(s, 1);
      continue;
    }

    /* Line comment */
    if (s->input.length >= 2 && s->input.ptr[0] == '/' && s->input.ptr[1] == '/')
    {
      s_tokenizer_advance(s, 2);
      while (s->input.length > 0 && s->input.ptr[0] != '\n')
      {
        s_tokenizer_advance(s, 1);
      }
      continue;
    }

    /* Block comment (doc handled elsewhere) */
    if (s->input.length >= 2 && s->input.ptr[0] == '/' && s->input.ptr[1] == '*')
    {
      /* If it's doc, stop: caller will emit it as a token. */
      if (s->input.length >= 3 && s->input.ptr[2] == '*')
      {
        return;
      }

      s_tokenizer_advance(s, 2);
      while (s->input.length > 0)
      {
        if (s->input.length >= 2 && s->input.ptr[0] == '*' && s->input.ptr[1] == '/')
        {
          s_tokenizer_advance(s, 2);
          break;
        }
        s_tokenizer_advance(s, 1);
      }
      continue;
    }

    return;
  }
}

/* Returns a doc comment token starting at "/ * *". Advances past it. */
static DoxterToken s_tokenizer_read_doc_comment(DoxterTokenizer* s)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  // Consume '/**' 
  s_tokenizer_advance(s, 3);
  len += 3;

  while (s->input.length > 0)
  {
    if (s->input.length >= 2 && s->input.ptr[0] == '*' && s->input.ptr[1] == '/')
    {
      s_tokenizer_advance(s, 2);
      len += 2;
      break;
    }

    /* Track newlines for at_bol */
    if (s->input.ptr[0] == '\n')
    {
      s->at_bol = true;
    }

    s_tokenizer_advance(s, 1);
    len += 1;
  }

  DoxterToken t;
  t.kind = DOXTER_DOX_COMMENT;
  t.text.ptr = start;
  t.text.length = len;
  t.start = start;
  return t;
}

/* Reads a string or char literal as a single token. */
static DoxterToken s_tokenizer_read_quoted(DoxterTokenizer* s, DoxterTokenKind kind, char quote)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  /* Consume opening quote */
  s_tokenizer_advance(s, 1);
  len += 1;

  bool escape = false;

  while (s->input.length > 0)
  {
    char c = s->input.ptr[0];

    s_tokenizer_advance(s, 1);
    len += 1;

    if (escape)
    {
      escape = false;
      continue;
    }

    if (c == '\\')
    {
      escape = true;
      continue;
    }

    if (c == quote)
    {
      break;
    }

    if (c == '\n')
    {
      s->at_bol = true;
    }
  }

  DoxterToken t;
  t.kind = kind;
  t.text.ptr = start;
  t.text.length = len;
  t.start = start;
  return t;
}

static DoxterToken s_tokenizer_read_ident(DoxterTokenizer* s)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  while (s->input.length > 0 && s_char_is_ident_char(s->input.ptr[0]))
  {
    s_tokenizer_advance(s, 1);
    len += 1;
  }

  DoxterToken t;
  t.kind = DOXTER_IDENT;
  t.text.ptr = start;
  t.text.length = len;
  t.start = start;
  return t;
}

static DoxterToken s_tokenizer_read_number(DoxterTokenizer* s)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  while (s->input.length > 0)
  {
    char c = s->input.ptr[0];
    if ((c >= '0' && c <= '9') || c == '.' || c == 'x' || c == 'X' || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'u' || c == 'U' || c == 'l' || c == 'L')
    {
      s_tokenizer_advance(s, 1);
      len += 1;
      continue;
    }
    break;
  }

  DoxterToken t;
  t.kind = DOXTER_NUMBER;
  t.text.ptr = start;
  t.text.length = len;
  t.start = start;
  return t;
}

static DoxterToken s_tokenizer_read_macro_directive(DoxterTokenizer* s)
{
  const char* start = s->input.ptr;

  s_tokenizer_advance(s, 1); /* '#' */

  while (s->input.length > 0)
  {
    char c = s->input.ptr[0];
    if (c == '\n')
    {
      break;
    }
    if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v')
    {
      s_tokenizer_advance(s, 1);
      continue;
    }
    break;
  }

  const char* name_start = s->input.ptr;
  size_t name_len = 0;

  while (s->input.length > 0 && s_char_is_ident_char(s->input.ptr[0]))
  {
    s_tokenizer_advance(s, 1);
    name_len++;
  }

  s->in_pp = true;
  s->pp_line_continuation = false;
  s->at_bol = false;

  DoxterToken t;
  t.kind = DOXTER_MACRO_DIRECTIVE;
  t.text.ptr = name_start;
  t.text.length = name_len;
  t.start = start;
  return t;
}

static DoxterToken s_tokenizer_read_punct(DoxterTokenizer* s)
{
  const char* start = s->input.ptr;
  size_t len = 1;

  if (s->input.length >= 2)
  {
    const char a = s->input.ptr[0];
    const char b = s->input.ptr[1];

    if ((a == ':' && b == ':') ||
        (a == '-' && b == '>') ||
        (a == '=' && b == '=') ||
        (a == '!' && b == '=') ||
        (a == '<' && b == '=') ||
        (a == '>' && b == '=') ||
        (a == '&' && b == '&') ||
        (a == '|' && b == '|') ||
        (a == '+' && b == '+') ||
        (a == '-' && b == '-') ||
        (a == '<' && b == '<') ||
        (a == '>' && b == '>')
       )
    {
      len = 2;
    }
  }

  s_tokenizer_advance(s, len);

  DoxterToken t;
  t.kind = DOXTER_PUNCT;
  t.text.ptr = start;
  t.text.length = len;
  t.start = start;
  return t;
}

/* returns next token (or END). */
static DoxterToken s_tokenizer_next_token(DoxterTokenizer* s, uint32_t* out_line, uint32_t* out_col)
{
  for (;;)
  {
    s_tokenizer_skip_trivia(s);

    if (out_line)
    {
      *out_line = s->line;
    }
    if (out_col)
    {
      *out_col = s->column;
    }

    if (s->input.length == 0)
    {
      DoxterToken t;
      t.kind = DOXTER_END;
      t.text = x_slice_empty();
      t.start = s->input.ptr;
      return t;
    }

    /* Doc comment tokenization: must be returned whole */
    if (x_slice_starts_with_cstr(s->input, "/**"))
    {
      return s_tokenizer_read_doc_comment(s);
    }

    /* Preprocessor end: s_tokenizer_skip_trivia stops at '\n' in pp mode when not continued. */
    if (s->in_pp && s->input.length > 0 && s->input.ptr[0] == '\n')
    {
      const char* end_ptr = s->input.ptr;

      s->in_pp = false;
      s->pp_line_continuation = false;
      s->at_bol = true;
      s_tokenizer_advance(s, 1);

      DoxterToken t;
      t.kind = DOXTER_PP_END;
      t.text.ptr = end_ptr;
      t.text.length = 0;
      t.start = end_ptr;
      return t;
    }

    /* Macro directive at BOL: '#define' and '# define' are the same token. */
    if (s->at_bol)
    {
      if (s->input.ptr[0] == '#')
      {
        return s_tokenizer_read_macro_directive(s);
      }
      s->at_bol = false;
    }

    char c = s->input.ptr[0];

    if (c == '"')
    {
      DoxterToken t = s_tokenizer_read_quoted(s, DOXTER_STRING, '"');
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    if (c == '\'')
    {
      DoxterToken t = s_tokenizer_read_quoted(s, DOXTER_CHAR, '\'');
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    if (s_char_is_ident_start(c))
    {
      DoxterToken t = s_tokenizer_read_ident(s);
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    if (c >= '0' && c <= '9')
    {
      DoxterToken t = s_tokenizer_read_number(s);
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    {
      DoxterToken t = s_tokenizer_read_punct(s);
      if (s->in_pp)
      {
        s->pp_line_continuation = (t.kind == DOXTER_PUNCT && t.text.length == 1 && t.text.ptr[0] == '\\');
      }
      return t;
    }
  }
}

static bool s_token_is_punct(DoxterToken t, char c)
{
  return t.kind == DOXTER_PUNCT && t.text.length == 1 && t.text.ptr[0] == c;
}

static DoxterToken s_token_dup(DoxterProject* proj, DoxterToken t)
{
  if (t.text.length > 0)
  {
    char* dup = x_arena_slicedup(proj->scratch, t.text.ptr, t.text.length, true);
    t.text.ptr = dup;
  }
  else
  {
    t.text.ptr = "";
    t.text.length = 0;
  }

  t.start = t.text.ptr;
  return t;
}

static void s_symbol_dup_slices(DoxterProject* proj, DoxterSymbol* sym)
{
  sym->comment.ptr = (sym->comment.length ?
      x_arena_slicedup(proj->scratch, sym->comment.ptr, sym->comment.length, true)
      : "");
  sym->declaration.ptr = (sym->declaration.length ?
      x_arena_slicedup(proj->scratch, sym->declaration.ptr, sym->declaration.length, true)
      : "");
  sym->name.ptr = (sym->name.length ?
      x_arena_slicedup(proj->scratch, sym->name.ptr, sym->name.length, true)
      : "");
}

static void s_symbol_collect_tokens(DoxterProject* proj, XSlice declaration, u32* out_first, u32* out_count)
{
  *out_first = x_array_count(proj->tokens);
  *out_count = 0;

  if (declaration.length == 0)
  {
    return;
  }

  DoxterTokenizer ts;
  ts.input = declaration;
  ts.line = 1;
  ts.column = 1;
  ts.at_bol = true;
  ts.in_pp = false;
  ts.pp_line_continuation = false;

  for (;;)
  {
    DoxterToken t = s_tokenizer_next_token(&ts, NULL, NULL);
    if (t.kind == DOXTER_END)
    {
      break;
    }

    /* PP_END is a structural token; it is not part of any declaration text. */
    if (t.kind == DOXTER_PP_END)
    {
      continue;
    }

    t = s_token_dup(proj, t);
    x_array_push(proj->tokens, &t);
    (*out_count)++;
  }
}

static bool s_skip_code_block(XSlice* input)
{
  if (input->length == 0 || *input->ptr != '{')
  {
    return false;
  }

  size_t i = 0;
  int depth = 0;

  bool in_string = false;
  bool in_char = false;
  bool in_line_comment = false;
  bool in_block_comment = false;
  bool escape = false;

  while (i < input->length)
  {
    char c = input->ptr[i];
    char n = (i + 1 < input->length) ? input->ptr[i + 1] : '\0';

    if (in_line_comment)
    {
      if (c == '\n')
      {
        in_line_comment = false;
      }
      i++;
      continue;
    }

    if (in_block_comment)
    {
      if (c == '*' && n == '/')
      {
        in_block_comment = false;
        i += 2;
        continue;
      }
      i++;
      continue;
    }

    if (in_string)
    {
      if (escape)
      {
        escape = false;
      }
      else if (c == '\\')
      {
        escape = true;
      }
      else if (c == '"')
      {
        in_string = false;
      }
      i++;
      continue;
    }

    if (in_char)
    {
      if (escape)
      {
        escape = false;
      }
      else if (c == '\\')
      {
        escape = true;
      }
      else if (c == '\'')
      {
        in_char = false;
      }
      i++;
      continue;
    }

    if (c == '/' && n == '/')
    {
      in_line_comment = true;
      i += 2;
      continue;
    }

    if (c == '/' && n == '*')
    {
      in_block_comment = true;
      i += 2;
      continue;
    }

    if (c == '"')
    {
      in_string = true;
      i++;
      continue;
    }

    if (c == '\'')
    {
      in_char = true;
      i++;
      continue;
    }

    if (c == '{')
    {
      depth++;
      i++;
      continue;
    }

    if (c == '}')
    {
      depth--;
      i++;
      if (depth == 0)
      {
        input->ptr += i;
        input->length -= i;
        return true;
      }
      continue;
    }

    i++;
  }

  return false;
}

static DoxterSymbol* s_find_symbol(XArray* symbols, XSlice name)
{
  DoxterSymbol* s = (DoxterSymbol*) x_array_getdata(symbols).ptr;
  u32 count = x_array_count(symbols);
  for (u32 i = 0; i < count; i++)
  {
    if (x_slice_eq(s->name, name))
    {
      return s;
    }
    s++;
  }
  return NULL;
}

static bool s_filter_symbol(DoxterSymbol* sym, DoxterConfig* cfg)
{
  if (cfg->option_skip_empty_defines && sym->is_empty_macro)
  {
    return false;
  }

  if (cfg->option_skip_static_functions && sym->is_static)
  {
    return false;
  }

  if (cfg->option_skip_undocumented_symbols && x_slice_is_empty(sym->comment))
  {
    return false;
  }

  return true;
}

static bool s_classify_statement(XSlice stmt, XSlice comment, uint32_t line, uint32_t col, DoxterSymbol* out)
{
  XSlice trimmed = x_slice_trim(stmt);
  if (trimmed.length == 0)
  {
    return false;
  }

  memset(out, 0, sizeof(*out));
  out->line = line;
  out->column = col;
  out->comment = comment;
  out->declaration = trimmed;
  out->name = x_slice_empty();
  out->is_typedef = false;
  out->is_static = false;
  out->is_empty_macro = false;

  bool saw_typedef = false;
  bool saw_struct = false;
  bool saw_enum = false;
  bool saw_union = false;

  XSlice struct_name = x_slice_empty();
  XSlice enum_name = x_slice_empty();
  XSlice union_name = x_slice_empty();

  XSlice last_ident = x_slice_empty();
  XSlice fn_name = x_slice_empty();

  /* Preferred typedef name for function pointers: (*Name) */
  XSlice fp_typedef_name = x_slice_empty();
  int fp_state = 0;
  int paren_depth = 0;

  DoxterTokenizer ts;
  ts.input = trimmed;
  ts.line = 1;
  ts.column = 1;
  ts.at_bol = true;
  ts.in_pp = false;
  ts.pp_line_continuation = false;

  DoxterToken prev = {0};

  for (;;)
  {
    DoxterToken t = s_tokenizer_next_token(&ts, NULL, NULL);
    if (t.kind == DOXTER_END)
    {
      break;
    }

    if (t.kind == DOXTER_IDENT)
    {
      if (x_slice_eq_cstr(t.text, "typedef"))
      {
        saw_typedef = true;
        out->is_typedef = true;
      }
      else if (x_slice_eq_cstr(t.text, "static"))
      {
        out->is_static = true;
      }
      else if (x_slice_eq_cstr(t.text, "struct"))
      {
        if (paren_depth == 0)
        {
          saw_struct = true;
        }
      }
      else if (x_slice_eq_cstr(t.text, "enum"))
      {
        if (paren_depth == 0)
        {
          saw_enum = true;
        }
      }
      else if (x_slice_eq_cstr(t.text, "union"))
      {
        if (paren_depth == 0)
        {
          saw_union = true;
        }
      }
      else
      {
        last_ident = t.text;
        if (saw_struct && struct_name.length == 0)
        {
          struct_name = t.text;
        }
        if (saw_enum && enum_name.length == 0)
        {
          enum_name = t.text;
        }
        if (saw_union && union_name.length == 0)
        {
          union_name = t.text;
        }

        if (fp_state == 2)
        {
          fp_typedef_name = t.text;
          fp_state = 3;
        }
      }
    }

    if (t.kind == DOXTER_PUNCT)
    {
      /* Track parentheses depth for top-level classification. */
      if (t.text.length == 1 && t.text.ptr[0] == '(')
      {
        /* Function name is the identifier immediately before '(' at top-level. */
        if (!saw_typedef && paren_depth == 0 && prev.kind == DOXTER_IDENT)
        {
          fn_name = prev.text;
        }

        /* Function-pointer typedef name pattern: typedef ... (*Name)(...) */
        if (saw_typedef && fp_typedef_name.length == 0 && fp_state == 0)
        {
          fp_state = 1;
        }

        paren_depth += 1;
      }
      else if (t.text.length == 1 && t.text.ptr[0] == ')')
      {
        if (paren_depth > 0)
        {
          paren_depth -= 1;
        }

        if (fp_state == 3)
        {
          fp_state = 0;
        }
      }
      else if (t.text.length == 1 && t.text.ptr[0] == '*')
      {
        if (fp_state == 1)
        {
          fp_state = 2;
        }
      }
    }

    prev = t;
  }

  if (saw_struct)
  {
    out->type = DOXTER_STRUCT;
    out->name = struct_name;
    return out->name.length > 0;
  }

  if (saw_enum)
  {
    out->type = DOXTER_ENUM;
    out->name = enum_name;
    return out->name.length > 0;
  }

  if (saw_union)
  {
    out->type = DOXTER_UNION;
    out->name = union_name;
    return out->name.length > 0;
  }

  if (saw_typedef)
  {
    out->type = DOXTER_TYPEDEF;
    out->name = fp_typedef_name.length > 0 ? fp_typedef_name : last_ident;
    return out->name.length > 0;
  }

  if (fn_name.length > 0)
  {
    out->type = DOXTER_FUNCTION;
    out->name = fn_name;
    return out->name.length > 0;
  }

  return false;
}

i32 doxter_source_parse(DoxterProject* proj, u32 source_index)
{
  DoxterSourceInfo* source_info = &proj->sources[source_index];
  source_info->first_symbol_index = x_array_count(proj->symbols);
  source_info->num_symbols = 0;

  size_t file_size = 0;
  const char *source_file_name = source_info->path;
  char *input = x_io_read_text(source_file_name, &file_size);
  if (! input)
    return -1;

  XSlice source = x_slice_init(input, file_size);

  DoxterTokenizer ts;
  ts.input = source;
  ts.line = 1;
  ts.column = 1;
  ts.at_bol = true;
  ts.in_pp = false;
  ts.pp_line_continuation = false;

  XSlice pending_doc = x_slice_empty();
  i32 count = 0;

  uint32_t tok_line = 1;
  uint32_t tok_col = 1;

  /* File symbol: if the file begins with a doxter comment. */
  {
    DoxterTokenizer tmp = ts;
    DoxterToken t = s_tokenizer_next_token(&tmp, &tok_line, &tok_col);
    if (t.kind == DOXTER_DOX_COMMENT && t.text.ptr == source.ptr)
    {
      XSlice file_comment =  s_cleanup_comment(t.text);

      DoxterSymbol sym;
      memset(&sym, 0, sizeof(sym));
      sym.type = DOXTER_FILE;
      sym.line = 1;
      sym.column = 1;
      sym.comment = file_comment;
      sym.declaration = x_slice_empty();
      sym.name = x_slice_empty();
      sym.first_token_index = x_array_count(proj->tokens);
      sym.num_tokens = 0;

      s_symbol_dup_slices(proj, &sym);
      x_array_push(proj->symbols, &sym);
      count++;

      ts = tmp;
    }
  }

  const char* stmt_start = NULL;
  uint32_t stmt_line = 0;
  uint32_t stmt_col = 0;
  bool seen_equal = false;

  int brace_depth = 0;
  int brace_top = 0;
  unsigned char brace_stack[512];
  int extern_depth = 0;

  DoxterToken prev_sig = (DoxterToken){0};
  DoxterToken prev_sig2 = (DoxterToken){0};

  while (ts.input.length > 0)
  {
    DoxterToken t = s_tokenizer_next_token(&ts, &tok_line, &tok_col);
    if (t.kind == DOXTER_END)
    {
      break;
    }

    if (t.kind == DOXTER_DOX_COMMENT)
    {
      pending_doc = t.text;
      continue;
    }

    if (t.kind == DOXTER_MACRO_DIRECTIVE)
    {
      /* Only #define is collected as a symbol for now. */
      XSlice directive = t.text;

      const char* start_ptr = t.start;
      const char* end_ptr = start_ptr;
      XSlice macro_name = x_slice_empty();
      bool empty_macro = true;

      for (;;)
      {
        DoxterToken mt = s_tokenizer_next_token(&ts, NULL, NULL);

        if (mt.kind == DOXTER_END)
        {
          end_ptr = source.ptr + source.length;
          break;
        }

        if (mt.kind == DOXTER_PP_END)
        {
          end_ptr = mt.start;
          break;
        }

        if (x_slice_eq_cstr(directive, "define"))
        {
          if (macro_name.length == 0 && mt.kind == DOXTER_IDENT)
          {
            macro_name = mt.text;
            empty_macro = true;
            continue;
          }

          empty_macro = false;
        }
      }

      if (x_slice_eq_cstr(directive, "define") && macro_name.length > 0)
      {
        DoxterSymbol sym;
        memset(&sym, 0, sizeof(sym));
        sym.type = DOXTER_MACRO;
        sym.line = tok_line;
        sym.column = tok_col;
        sym.comment = pending_doc;
        sym.declaration.ptr = start_ptr;
        sym.declaration.length = (size_t)(end_ptr - start_ptr);
        sym.declaration = x_slice_trim(sym.declaration);
        sym.name = macro_name;
        sym.is_empty_macro = empty_macro;

        s_symbol_dup_slices(proj, &sym);
        s_symbol_collect_tokens(proj, sym.declaration, &sym.first_token_index, &sym.num_tokens);
        x_array_push(proj->symbols, &sym);
        count++;
      }

      pending_doc = x_slice_empty();
      stmt_start = NULL;
      seen_equal = false;
      brace_depth = 0;
      brace_top = 0;
      extern_depth = 0;
      prev_sig = (DoxterToken){0};
      prev_sig2 = (DoxterToken){0};
      continue;
    }

    int effective_depth = brace_depth - extern_depth;
    if (effective_depth < 0)
    {
      effective_depth = 0;
    }

    if (stmt_start == NULL && effective_depth == 0)
    {
      if (!(t.kind == DOXTER_PUNCT && (s_token_is_punct(t, ';') || s_token_is_punct(t, '}'))))
      {
        stmt_start = t.text.ptr;
        stmt_line = tok_line;
        stmt_col = tok_col;
        seen_equal = false;
      }
    }

    if (t.kind == DOXTER_PUNCT && t.text.length == 1 && t.text.ptr[0] == '=')
    {
      seen_equal = true;
    }

    if (s_token_is_punct(t, '{'))
    {
      bool is_extern_c = (prev_sig2.kind == DOXTER_IDENT && x_slice_eq_cstr(prev_sig2.text, "extern") &&
          prev_sig.kind == DOXTER_STRING && x_slice_eq_cstr(prev_sig.text, "\"C\""));

      if (!is_extern_c && effective_depth == 0 && s_token_is_punct(prev_sig, ')') && !seen_equal)
      {
        /* Function body: classify using the signature slice up to '{', then skip the body. */
        XSlice sig;
        sig.ptr = stmt_start ? stmt_start : t.text.ptr;
        sig.length = (size_t) (t.text.ptr - sig.ptr);
        sig = x_slice_trim(sig);

        if (sig.length > 0)
        {
          DoxterSymbol sym;
          if (s_classify_statement(sig, pending_doc, stmt_line, stmt_col, &sym))
          {
            if (!s_find_symbol(proj->symbols, sym.name) && s_filter_symbol(&sym, &proj->config))
            {
              s_symbol_dup_slices(proj, &sym);
              s_symbol_collect_tokens(proj, sym.declaration, &sym.first_token_index, &sym.num_tokens);
              x_array_push(proj->symbols, &sym);
              count++;
            }
          }
        }

        pending_doc = x_slice_empty();
        stmt_start = NULL;
        seen_equal = false;

        /* Skip block starting at '{' (token already consumed, so build slice from token start). */
        {
          XSlice tmp;
          tmp.ptr = t.text.ptr;
          tmp.length = (size_t) ((source.ptr + source.length) - t.text.ptr);
          if (s_skip_code_block(&tmp))
          {
            const char* target = tmp.ptr;
            if (target > ts.input.ptr)
            {
              size_t delta = (size_t) (target - ts.input.ptr);
              s_tokenizer_advance(&ts, delta);
            }
            ts.in_pp = false;
            ts.pp_line_continuation = false;
          }
        }

        prev_sig = (DoxterToken){0};
        prev_sig2 = (DoxterToken){0};
        continue;
      }

      /* Regular brace: track nesting. */
      if (brace_top < (int) (sizeof(brace_stack) / sizeof(brace_stack[0])))
      {
        brace_stack[brace_top++] = (unsigned char) (is_extern_c ? 1 : 0);
      }
      brace_depth++;
      if (is_extern_c)
      {
        extern_depth++;
      }

      prev_sig2 = prev_sig;
      prev_sig = t;
      continue;
    }

    if (s_token_is_punct(t, '}'))
    {
      if (brace_depth > 0)
      {
        brace_depth--;
      }
      if (brace_top > 0)
      {
        unsigned char was_extern = brace_stack[--brace_top];
        if (was_extern && extern_depth > 0)
        {
          extern_depth--;
        }
      }

      prev_sig2 = prev_sig;
      prev_sig = t;
      continue;
    }

    effective_depth = brace_depth - extern_depth;
    if (effective_depth < 0)
    {
      effective_depth = 0;
    }

    if (s_token_is_punct(t, ';') && effective_depth == 0 && stmt_start != NULL)
    {
      XSlice stmt;
      stmt.ptr = stmt_start;
      stmt.length = (size_t) ((t.text.ptr + t.text.length) - stmt_start);
      stmt = x_slice_trim(stmt);

      if (stmt.length > 0)
      {
        DoxterSymbol sym;
        if (s_classify_statement(stmt, pending_doc, stmt_line, stmt_col, &sym))
        {
          if (!s_find_symbol(proj->symbols, sym.name) && s_filter_symbol(&sym, &proj->config))
          {
            s_symbol_dup_slices(proj, &sym);
            s_symbol_collect_tokens(proj, sym.declaration, &sym.first_token_index, &sym.num_tokens);
            x_array_push(proj->symbols, &sym);
            count++;
          }
        }
      }

      pending_doc = x_slice_empty();
      stmt_start = NULL;
      seen_equal = false;
      prev_sig2 = prev_sig;
      prev_sig = t;
      continue;
    }

    prev_sig2 = prev_sig;
    prev_sig = t;
  }

  source_info->num_symbols = count;
  return count;
}
