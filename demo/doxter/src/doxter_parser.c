#include "doxter.h"
#include "stdx_array.h"
#include "stdx_string.h"
#include <stdio.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef enum DoxterTokenKind
{
  DOXTER_END = 0,
  DOXTER_MACRO_DIRECTIVE,
  DOXTER_IDENT,
  DOXTER_NUMBER,
  DOXTER_STRING,
  DOXTER_CHAR,
  DOXTER_PUNCT,
  DOXTER_DOX_COMMENT,
  DOXTER_PP_END
} DoxterTokenKind;

typedef struct DoxterToken
{
  DoxterTokenKind kind;
  XSlice text;
  const char* start;
} DoxterToken;

typedef struct DoxterTokenState
{
  XSlice input;
  bool at_bol;      /* beginning of line (after newline) */
  bool in_pp;       /* currently tokenizing a preprocessor directive */
  bool pp_line_continuation; /* last returned token was "\" while in_pp */
} DoxterTokenState;

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

static bool s_is_space(char c)
{
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

static bool s_is_ident_start(char c)
{
  if (c >= 'a' && c <= 'z') return true;
  if (c >= 'A' && c <= 'Z') return true;
  if (c == '_') return true;
  return false;
}

static bool s_is_ident_char(char c)
{
  if (s_is_ident_start(c)) return true;
  if (c >= '0' && c <= '9') return true;
  return false;
}

static void s_advance(DoxterTokenState* s, size_t n)
{
  if (n > s->input.length)
  {
    n = s->input.length;
  }
  s->input.ptr += n;
  s->input.length -= n;
}

static bool s_starts_with(XSlice in, const char* lit)
{
  size_t n = strlen(lit);
  if (in.length < n)
  {
    return false;
  }
  return memcmp(in.ptr, lit, n) == 0;
}

/* Skips whitespace and non-doc comments. Updates at_bol/in_pp state. */
static void s_skip_trivia(DoxterTokenState* s)
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
    if (s_is_space(c))
    {
      if (c == '\n')
      {
        if (s->in_pp && !s->pp_line_continuation)
        {
          return;
        }
        s->at_bol = true;
      }

      s_advance(s, 1);
      continue;
    }

    /* Line comment */
    if (s->input.length >= 2 && s->input.ptr[0] == '/' && s->input.ptr[1] == '/')
    {
      s_advance(s, 2);
      while (s->input.length > 0 && s->input.ptr[0] != '\n')
      {
        s_advance(s, 1);
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

      s_advance(s, 2);
      while (s->input.length > 0)
      {
        if (s->input.length >= 2 && s->input.ptr[0] == '*' && s->input.ptr[1] == '/')
        {
          s_advance(s, 2);
          break;
        }
        s_advance(s, 1);
      }
      continue;
    }

    return;
  }
}

/* Returns a doc comment token starting at "/ * *". Advances past it. */
static DoxterToken s_read_doc_comment(DoxterTokenState* s)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  /* Consume '/**' */
  s_advance(s, 3);
  len += 3;

  while (s->input.length > 0)
  {
    if (s->input.length >= 2 && s->input.ptr[0] == '*' && s->input.ptr[1] == '/')
    {
      s_advance(s, 2);
      len += 2;
      break;
    }

    /* Track newlines for at_bol */
    if (s->input.ptr[0] == '\n')
    {
      s->at_bol = true;
    }

    s_advance(s, 1);
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
static DoxterToken s_read_quoted(DoxterTokenState* s, DoxterTokenKind kind, char quote)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  /* Consume opening quote */
  s_advance(s, 1);
  len += 1;

  bool escape = false;

  while (s->input.length > 0)
  {
    char c = s->input.ptr[0];

    s_advance(s, 1);
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

static DoxterToken s_read_ident(DoxterTokenState* s)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  while (s->input.length > 0 && s_is_ident_char(s->input.ptr[0]))
  {
    s_advance(s, 1);
    len += 1;
  }

  DoxterToken t;
  t.kind = DOXTER_IDENT;
  t.text.ptr = start;
  t.text.length = len;
  t.start = start;
  return t;
}

static DoxterToken s_read_number(DoxterTokenState* s)
{
  const char* start = s->input.ptr;
  size_t len = 0;

  while (s->input.length > 0)
  {
    char c = s->input.ptr[0];
    if ((c >= '0' && c <= '9') || c == '.' || c == 'x' || c == 'X' || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'u' || c == 'U' || c == 'l' || c == 'L')
    {
      s_advance(s, 1);
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

static DoxterToken s_read_macro_directive(DoxterTokenState* s)
{
  const char* start = s->input.ptr;

  s_advance(s, 1); /* '#' */

  while (s->input.length > 0)
  {
    char c = s->input.ptr[0];
    if (c == '\n')
    {
      break;
    }
    if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v')
    {
      s_advance(s, 1);
      continue;
    }
    break;
  }

  const char* name_start = s->input.ptr;
  size_t name_len = 0;

  while (s->input.length > 0 && s_is_ident_char(s->input.ptr[0]))
  {
    s_advance(s, 1);
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

static bool s_is_space_no_nl(char c)
{
  return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v';
}

static XSlice s_read_macro_body(XSlice* input)
{
  XSlice out = x_slice_empty();

  if (input->length == 0)
  {
    return out;
  }

  /* We expect to be positioned at '#' (possibly after leading whitespace). */
  const char* start = input->ptr;
  size_t i = 0;

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
        /* End of physical line. Decide if directive continues. */
        in_line_comment = false;

        /* Look back for backslash before newline (ignoring trailing spaces and '\r'). */
        size_t k = i;
        while (k > 0 && (input->ptr[k - 1] == '\r' || s_is_space_no_nl(input->ptr[k - 1])))
        {
          k--;
        }

        if (k > 0 && input->ptr[k - 1] == '\\')
        {
          i++;
          continue; /* Continued directive */
        }

        i++; /* Include newline in macro body slice */
        break; /* End of directive */
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

    /* Not inside string/char/comment */
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

    if (c == '\n')
    {
      /* End of physical line. Decide if directive continues. */
      size_t k = i;
      while (k > 0 && (input->ptr[k - 1] == '\r' || s_is_space_no_nl(input->ptr[k - 1])))
      {
        k--;
      }

      if (k > 0 && input->ptr[k - 1] == '\\')
      {
        i++;
        continue; /* Continued directive */
      }

      i++; /* Include newline */
      break; /* End of directive */
    }

    i++;
  }

  out.ptr = start;
  out.length = i;

  input->ptr += i;
  input->length -= i;

  return out;
}

/* Punct: supports a small set of 2-char ops; everything else is 1 char. */
static DoxterToken s_read_punct(DoxterTokenState* s)
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

  s_advance(s, len);

  DoxterToken t;
  t.kind = DOXTER_PUNCT;
  t.text.ptr = start;
  t.text.length = len;
  t.start = start;
  return t;
}

/* Main: returns next token (or END). */
static DoxterToken doxter_next_token(DoxterTokenState* s)
{
  for (;;)
  {
    s_skip_trivia(s);

    if (s->input.length == 0)
    {
      DoxterToken t;
      t.kind = DOXTER_END;
      t.text = x_slice_empty();
      t.start = s->input.ptr;
      return t;
    }

    /* Doc comment tokenization: must be returned whole */
    if (s_starts_with(s->input, "/**"))
    {
      return s_read_doc_comment(s);
    }

    /* Preprocessor end: s_skip_trivia stops at '\n' in pp mode when not continued. */
    if (s->in_pp && s->input.length > 0 && s->input.ptr[0] == '\n')
    {
      const char* end_ptr = s->input.ptr;

      s->in_pp = false;
      s->pp_line_continuation = false;
      s->at_bol = true;
      s_advance(s, 1);

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
        return s_read_macro_directive(s);
      }
      s->at_bol = false;
    }

    char c = s->input.ptr[0];

    if (c == '"')
    {
      DoxterToken t = s_read_quoted(s, DOXTER_STRING, '"');
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    if (c == '\'')
    {
      DoxterToken t = s_read_quoted(s, DOXTER_CHAR, '\'');
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    if (s_is_ident_start(c))
    {
      DoxterToken t = s_read_ident(s);
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    if (c >= '0' && c <= '9')
    {
      DoxterToken t = s_read_number(s);
      if (s->in_pp) s->pp_line_continuation = false;
      return t;
    }

    {
      DoxterToken t = s_read_punct(s);
      if (s->in_pp)
      {
        s->pp_line_continuation = (t.kind == DOXTER_PUNCT && t.text.length == 1 && t.text.ptr[0] == '\\');
      }
      return t;
    }
  }
}

static bool s_slice_eq_cstr(XSlice a, const char* b)
{
  size_t blen = strlen(b);
  if (a.length != blen)
  {
    return false;
  }
  return memcmp(a.ptr, b, blen) == 0;
}

static bool s_token_is_punct(DoxterToken t, char c)
{
  return t.kind == DOXTER_PUNCT && t.text.length == 1 && t.text.ptr[0] == c;
}

static bool s_token_is_ident_cstr(DoxterToken t, const char* s)
{
  return t.kind == DOXTER_IDENT && s_slice_eq_cstr(t.text, s);
}

static void s_update_line_col(const char* from, const char* to, uint32_t* line, uint32_t* col)
{
  const char* p = from;
  while (p < to)
  {
    if (*p == '\n')
    {
      (*line)++;
      *col = 1;
    }
    else
    {
      (*col)++;
    }
    p++;
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

static XSlice s_trim_pp_leading_ws(XSlice s)
{
  while (s.length > 0)
  {
    char c = *s.ptr;
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\f' && c != '\v')
    {
      break;
    }
    s.ptr++;
    s.length--;
  }
  return s;
}

static bool s_classify_macro(XSlice macro, XSlice comment, uint32_t line, uint32_t col, DoxterSymbol* out)
{
  XSlice m = s_trim_pp_leading_ws(macro);
  if (m.length == 0 || *m.ptr != '#')
  {
    return false;
  }

  memset(out, 0, sizeof(*out));
  out->line = line;
  out->column = col;
  out->comment = comment;
  out->declaration = macro;
  out->name = x_slice_empty();
  out->is_typedef = false;
  out->is_static = false;
  out->is_empty_macro = false;

  /* Tokenize directive with whitespace tokenizer; punctuation remains attached, handle name(a,b) via '(' scan. */
  XSlice it = m;
  XSlice tok = x_slice_empty();

  x_slice_next_token_white_space(&it, &tok);
  if (!s_slice_eq_cstr(tok, "#") && !s_slice_eq_cstr(tok, "#define"))
  {
    /* Accept '# define' too */
    if (!s_slice_eq_cstr(tok, "#"))
    {
      return false;
    }
  }

  XSlice t1 = x_slice_empty();
  x_slice_next_token_white_space(&it, &t1);
  if (s_slice_eq_cstr(tok, "#define"))
  {
    /* ok */
  }
  else
  {
    if (!s_slice_eq_cstr(t1, "define"))
    {
      return false;
    }
    x_slice_next_token_white_space(&it, &t1);
  }

  if (t1.length == 0)
  {
    return false;
  }

  out->type = DOXTER_MACRO;

  /* Macro name may be NAME(args) with no whitespace. */
  const char* p = (const char*) memchr(t1.ptr, '(', t1.length);
  if (p != NULL)
  {
    out->name.ptr = t1.ptr;
    out->name.length = (size_t) (p - t1.ptr);
  }
  else
  {
    out->name = t1;
  }

  /* Determine empty macro: no further tokens (besides possible trailing whitespace). */
  {
    XSlice rest = x_slice_trim(it);
    if (rest.length == 0)
    {
      out->is_empty_macro = true;
    }
  }

  return out->name.length > 0;
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

  DoxterTokenState ts;
  ts.input = trimmed;
  ts.at_bol = true;
  ts.in_pp = false;
  ts.pp_line_continuation = false;

  DoxterToken prev = {0};

  for (;;)
  {
    DoxterToken t = doxter_next_token(&ts);
    if (t.kind == DOXTER_END)
    {
      break;
    }

    if (t.kind == DOXTER_IDENT)
    {
      if (s_slice_eq_cstr(t.text, "typedef"))
      {
        saw_typedef = true;
        out->is_typedef = true;
      }
      else if (s_slice_eq_cstr(t.text, "static"))
      {
        out->is_static = true;
      }
      else if (s_slice_eq_cstr(t.text, "struct"))
      {
        if (paren_depth == 0)
        {
          saw_struct = true;
        }
      }
      else if (s_slice_eq_cstr(t.text, "enum"))
      {
        if (paren_depth == 0)
        {
          saw_enum = true;
        }
      }
      else if (s_slice_eq_cstr(t.text, "union"))
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

/**
 * @brief Collects all symbols from a source file. Comments are also collected
 * when present.
 * @param source Source file to scan for symbols
 * @param aray An XArray for storing the symbols
 * @return the number or symbols found
 */
u32 doxter_source_symbols_collect(XSlice source, DoxterConfig* cfg, XArray* array)
{
  DoxterTokenState ts;
  ts.input = source;
  ts.at_bol = true;
  ts.in_pp = false;
  ts.pp_line_continuation = false;

  XSlice pending_doc = x_slice_empty();
  u32 count = 0;

  uint32_t line = 1;
  uint32_t col = 1;
  const char* cur_ptr = source.ptr;

  /* File symbol: if the file begins with a doxter comment. */
  {
    DoxterTokenState tmp = ts;
    DoxterToken t = doxter_next_token(&tmp);
    if (t.kind == DOXTER_DOX_COMMENT && t.text.ptr == source.ptr)
    {
      XSlice file_comment = s_cleanup_comment(t.text);

      DoxterSymbol sym;
      memset(&sym, 0, sizeof(sym));
      sym.type = DOXTER_FILE;
      sym.line = 1;
      sym.column = 1;
      sym.comment = file_comment;
      sym.declaration = x_slice_empty();
      sym.name = x_slice_empty();

      x_array_push(array, &sym);
      count++;

      ts = tmp;
      s_update_line_col(cur_ptr, ts.input.ptr, &line, &col);
      cur_ptr = ts.input.ptr;
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
    DoxterToken t = doxter_next_token(&ts);
    if (t.kind == DOXTER_END)
    {
      break;
    }

    /* Track line/col up to token start. */
    s_update_line_col(cur_ptr, t.start, &line, &col);
    uint32_t tok_line = line;
    uint32_t tok_col = col;
    s_update_line_col(t.text.ptr, t.text.ptr + t.text.length, &line, &col);
    cur_ptr = t.text.ptr + t.text.length;

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
        DoxterToken mt = doxter_next_token(&ts);

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

        if (s_slice_eq_cstr(directive, "define"))
        {
          if (macro_name.length == 0 && mt.kind == DOXTER_IDENT)
          {
            macro_name = mt.text;
            continue;
          }

          if (macro_name.length > 0)
          {
            empty_macro = false;
          }
        }
      }

      if (s_slice_eq_cstr(directive, "define") && macro_name.length > 0)
      {
        DoxterSymbol sym;
        memset(&sym, 0, sizeof(sym));
        sym.type = DOXTER_MACRO;
        sym.line = line;
        sym.column = col;
        sym.comment = pending_doc;
        sym.declaration.ptr = start_ptr;
        sym.declaration.length = (size_t)(end_ptr - start_ptr);
        sym.declaration = x_slice_trim(sym.declaration);
        sym.name = macro_name;
        sym.is_empty_macro = empty_macro;

        x_array_push(array, &sym);
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
      bool is_extern_c = (prev_sig2.kind == DOXTER_IDENT && s_slice_eq_cstr(prev_sig2.text, "extern") &&
          prev_sig.kind == DOXTER_STRING && s_slice_eq_cstr(prev_sig.text, "\"C\""));

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
            if (!s_find_symbol(array, sym.name) && s_filter_symbol(&sym, cfg))
            {
              x_array_push(array, &sym);
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
            s_update_line_col(cur_ptr, tmp.ptr, &line, &col);
            cur_ptr = tmp.ptr;
            ts.input = tmp;
            ts.at_bol = true;
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
          if (!s_find_symbol(array, sym.name) && s_filter_symbol(&sym, cfg))
          {
            x_array_push(array, &sym);
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

  return count;
}

static const char* s_tok_kind_name(DoxterTokenKind k)
{
  switch (k)
  {
    case DOXTER_END:         return "END";
    case DOXTER_MACRO_DIRECTIVE:         return "MACRO";
    case DOXTER_IDENT:       return "IDENT";
    case DOXTER_NUMBER:      return "NUMBER";
    case DOXTER_STRING:      return "STRING";
    case DOXTER_CHAR:        return "CHAR";
    case DOXTER_PUNCT:       return "PUNCT";
    case DOXTER_DOX_COMMENT: return "DOX_COMMENT";
    case DOXTER_PP_END:      return "PP_END";
    default:                 return "UNKNOWN";
  }
}

#define s_next_token doxter_next
void doxter_token_dump(DoxterTokenState* s, const char* label)
{
  printf("=== TOKEN DUMP: %s ===\n", label);

  for (;;)
  {
    DoxterToken t = doxter_next_token(s);

    if (t.kind == DOXTER_END)
    {
      printf("END\n\n");
      break;
    }

    /* Optional: if your state tracks line/col, print them here */
    /* printf("%5u:%-4u ", (unsigned)s->line, (unsigned)s->col); */

    printf("%-12s len=%zu  ", s_tok_kind_name(t.kind), (size_t)t.text.length);

    size_t n = t.text.length;
    if (n > 80) n = 80;

    fwrite(t.text.ptr, 1, n, stdout);
    if (n < t.text.length) printf("...");
    printf("\n");

    /* Sanity check: DOX_COMMENT must be whole */
    if (t.kind == DOXTER_DOX_COMMENT)
    {
      if (t.text.length < 5 ||
          t.text.ptr[0] != '/' || t.text.ptr[1] != '*' || t.text.ptr[2] != '*' ||
          t.text.ptr[t.text.length - 2] != '*' || t.text.ptr[t.text.length - 1] != '/')
      {
        printf("!! ERROR: malformed DOX_COMMENT token\n");
      }
    }
  }
}

u32 doxter_test_tokenizer(XSlice source)
{
  DoxterTokenState ts;
  ts.input = source;
  ts.at_bol = true;
  ts.in_pp = false;
  doxter_token_dump(&ts, "test");
  return 0;
}
