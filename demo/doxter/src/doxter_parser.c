#include "doxter.h"
#include "stdx_array.h"
#include "stdx_string.h"

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef enum DoxterTokenKind
{
  DOXTER_END = 0,
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
} DoxterToken;

typedef struct DoxterTokenState
{
  XSlice input;
  bool at_bol;      /* beginning of line (after newline) */
  bool in_pp;       /* currently tokenizing a preprocessor directive */
} DoxterTokenState;

static void s_loc_from_ptr(XSlice source, const char* p, uint32_t* out_line, uint32_t* out_col)
{
  uint32_t line = 1;
  uint32_t col = 1;

  const char* it = source.ptr;
  const char* end = source.ptr + source.length;
  if (p < it)
  {
    p = it;
  }
  if (p > end)
  {
    p = end;
  }

  while (it < p)
  {
    char c = *it++;
    if (c == '\n')
    {
      line++;
      col = 1;
    }
    else
    {
      col++;
    }
  }

  if (out_line) *out_line = line;
  if (out_col) *out_col = col;
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
        s->at_bol = true;

        /* In PP mode, newline ends directive unless line continuation happened.
           Continuation is: previous non-newline char was '\'. We check that in next_token
           right before consuming '\n', because we need the previous char. */
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
  return t;
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

static bool s_filter_symbol(const DoxterSymbol* sym, DoxterConfig* cfg)
{
  if (!cfg)
  {
    return true;
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

static bool s_punct_is(const DoxterToken* t, const char* punct)
{
  size_t n = strlen(punct);
  if (t->kind != DOXTER_PUNCT)
  {
    return false;
  }
  if (t->text.length != n)
  {
    return false;
  }
  return memcmp(t->text.ptr, punct, n) == 0;
}

static bool s_ident_is(const DoxterToken* t, const char* ident)
{
  size_t n = strlen(ident);
  if (t->kind != DOXTER_IDENT)
  {
    return false;
  }
  if (t->text.length != n)
  {
    return false;
  }
  return memcmp(t->text.ptr, ident, n) == 0;
}

static DoxterToken s_next_token(DoxterTokenState* s)
{
  for (;;)
  {
    s_skip_trivia(s);

    if (s->input.length == 0)
    {
      DoxterToken t;
      t.kind = DOXTER_END;
      t.text = x_slice_empty();
      return t;
    }

    /* Doc comment tokenization: must be returned whole */
    if (s_starts_with(s->input, "/**"))
    {
      return s_read_doc_comment(s);
    }

    /* PP start: at beginning-of-line after trivia */
    if (s->at_bol)
    {
      /* If line starts with '#', enter PP mode. */
      if (s->input.ptr[0] == '#')
      {
        s->in_pp = true;
        s->at_bol = false;
        return s_read_punct(s);
      }

      s->at_bol = false;
    }

    /* In PP mode, a newline ends directive unless previous char is backslash.
       Note: because trivia skipping already consumed whitespace/newlines, we implement
       PP end by detecting that we *arrived* at a new BOL while still in_pp.
       Alternative: keep newlines as trivia but with special checks. Simpler:
       emit PP_END whenever we are in_pp and at_bol becomes true during trivia skipping.
       We do that here by checking at_bol flag post-skip. */
    if (s->in_pp && s->at_bol)
    {
      /* This means we consumed a newline. We need to know if it was escaped.
         Lightweight approach: treat any PP newline as end (works for most directives),
         OR require caller to keep raw newlines and do exact backslash check.
         If you need exact continuation support, we can do it by not eating '\n' in skip_trivia
         when in_pp. */
      s->in_pp = false;

      DoxterToken t;
      t.kind = DOXTER_PP_END;
      t.text = x_slice_empty();
      return t;
    }

    char c = s->input.ptr[0];

    if (c == '"')
    {
      return s_read_quoted(s, DOXTER_STRING, '"');
    }

    if (c == '\'')
    {
      return s_read_quoted(s, DOXTER_CHAR, '\'');
    }

    if (s_is_ident_start(c))
    {
      return s_read_ident(s);
    }

    if (c >= '0' && c <= '9')
    {
      return s_read_number(s);
    }

    return s_read_punct(s);
  }
}

static bool s_skip_code_block(DoxterTokenState* st)
{
  int depth = 0;

  for (;;)
  {
    DoxterToken t = s_next_token(st);

    if (t.kind == DOXTER_END)
    {
      return false;
    }

    if (t.kind == DOXTER_PUNCT && t.text.length == 1 && t.text.ptr[0] == '{')
    {
      depth++;
      continue;
    }

    if (t.kind == DOXTER_PUNCT && t.text.length == 1 && t.text.ptr[0] == '}')
    {
      depth--;
      if (depth <= 0)
      {
        return true;
      }
    }
  }
}

static bool s_extract_macro_from_directive(XSlice directive, DoxterSymbol* out)
{
  /* directive starts at '#'. We only care about #define NAME ... */
  XSlice it = directive;
  XSlice tok = x_slice_empty();

  /* '#' */
  x_slice_next_token_white_space(&it, &tok);

  /* Accept "#define" or "#" "define" */
  x_slice_next_token_white_space(&it, &tok);
  if (tok.length == 0)
  {
    return false;
  }

  if (! ((tok.length == 6 && memcmp(tok.ptr, "define", 6) == 0) &&
        (tok.length == 7 && memcmp(tok.ptr, "#define", 7) == 0)))
  {
    /* Could be '#', then directive name */
    if (tok.length == 1 && tok.ptr[0] == '#')
    {
      XSlice d = x_slice_empty();
      x_slice_next_token_white_space(&it, &d);
      if (!(d.length == 6 && memcmp(d.ptr, "define", 6) == 0))
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }

  /* macro name token */
  XSlice name_tok = x_slice_empty();
  x_slice_next_token_white_space(&it, &name_tok);
  if (name_tok.length == 0)
  {
    return false;
  }

  /* Name may be NAME(args) */
  const char* p = (const char*) memchr(name_tok.ptr, '(', name_tok.length);
  if (p)
  {
    out->name.ptr = name_tok.ptr;
    out->name.length = (size_t) (p - name_tok.ptr);
  }
  else
  {
    out->name = name_tok;
  }

  /* Empty macro: nothing else but whitespace/newlines after name */
  XSlice rest = x_slice_trim(it);
  out->is_empty_macro = (rest.length == 0);
  return out->name.length > 0;
}

u32 doxter_source_symbols_collect(XSlice source, DoxterConfig* cfg, XArray* array)
{
  DoxterTokenState st;
  st.input = source;
  st.at_bol = true;
  st.in_pp = false;

  XSlice pending_doc = x_slice_empty();
  u32 count = 0;

  /* File-level symbol: if the file starts with a dox comment, attach it. */
  {
    DoxterTokenState tmp = st;
    s_skip_trivia(&tmp);

    DoxterToken first = s_next_token(&tmp);
    XSlice file_comment = x_slice_empty();

    if (first.kind == DOXTER_DOX_COMMENT)
    {
      file_comment = first.text;
      st = tmp;
    }

    DoxterSymbol sym;
    memset(&sym, 0, sizeof(sym));
    sym.type        = DOXTER_FILE;
    sym.line        = 1;
    sym.column      = 1;
    sym.comment     = file_comment;
    sym.declaration = x_slice_empty();
    sym.name        = x_slice_empty();
    sym.is_typedef  = false;
    sym.is_static   = false;
    sym.is_empty_macro = false;

    x_array_push(array, &sym);
    count++;
  }

  int brace_depth = 0;

  while (st.input.length > 0)
  {
    /* If we're at beginning-of-line and see '#', read the whole directive. */
    {
      DoxterTokenState tmp = st;
      s_skip_trivia(&tmp);
      if (tmp.at_bol && tmp.input.length > 0 && tmp.input.ptr[0] == '#')
      {
        XSlice dir = s_read_macro_body(&tmp.input);

        uint32_t dir_line = 0;
        uint32_t dir_col = 0;
        s_loc_from_ptr(source, dir.ptr, &dir_line, &dir_col);

        /* Update main state to after directive. */
        st = tmp;
        st.in_pp = false;

        DoxterSymbol sym;
        memset(&sym, 0, sizeof(sym));
        sym.type = DOXTER_MACRO;
        sym.comment = pending_doc;
        sym.declaration = dir;
        sym.is_typedef = false;
        sym.is_static = false;
        sym.is_empty_macro = false;
        sym.line = dir_line;
        sym.column = dir_col;
        sym.name = x_slice_empty();

        if (s_extract_macro_from_directive(dir, &sym))
        {
          if (s_filter_symbol(&sym, cfg))
          {
            x_array_push(array, &sym);
            count++;
          }
        }

        pending_doc = x_slice_empty();
        continue;
      }
    }

    DoxterToken t = s_next_token(&st);

    if (t.kind == DOXTER_END)
    {
      break;
    }

    if (t.kind == DOXTER_DOX_COMMENT)
    {
      pending_doc = t.text;
      continue;
    }

    /* Track brace depth to avoid parsing inside function bodies. */
    if (t.kind == DOXTER_PUNCT && t.text.length == 1)
    {
      if (t.text.ptr[0] == '{')
      {
        brace_depth++;
      }
      else if (t.text.ptr[0] == '}')
      {
        if (brace_depth > 0)
        {
          brace_depth--;
        }
      }
    }

    if (brace_depth != 0)
    {
      pending_doc = x_slice_empty();
      continue;
    }

    /* At top-level: read a statement starting at current token. */
    {
      const char* stmt_start = t.text.ptr;
      const char* stmt_end = t.text.ptr + t.text.length;

      bool saw_typedef = false;
      bool saw_static = false;
      bool saw_struct = false;
      bool saw_enum = false;
      bool saw_union = false;

      bool want_struct_name = false;
      bool want_enum_name = false;
      bool want_union_name = false;

      XSlice name = x_slice_empty();
      XSlice typedef_candidate = x_slice_empty();

      DoxterToken prev = t;

      for (;;)
      {
        if (t.kind == DOXTER_IDENT)
        {
          if (want_struct_name)
          {
            name = t.text;
            want_struct_name = false;
          }
          else if (want_enum_name)
          {
            name = t.text;
            want_enum_name = false;
          }
          else if (want_union_name)
          {
            name = t.text;
            want_union_name = false;
          }

          if (s_ident_is(&t, "typedef"))
          {
            saw_typedef = true;
          }
          else if (s_ident_is(&t, "static"))
          {
            saw_static = true;
          }
          else if (s_ident_is(&t, "struct"))
          {
            saw_struct = true;
            want_struct_name = true;
          }
          else if (s_ident_is(&t, "enum"))
          {
            saw_enum = true;
            want_enum_name = true;
          }
          else if (s_ident_is(&t, "union"))
          {
            saw_union = true;
            want_union_name = true;
          }

          if (saw_typedef)
          {
            typedef_candidate = t.text;
          }
        }

        /* function name: ident immediately before '(' */
        if (!saw_typedef && s_punct_is(&t, "("))
        {
          if (prev.kind == DOXTER_IDENT)
          {
            name = prev.text;
          }
        }

        /* Function-pointer typedef: detect "(" "*" IDENT ")" */
        if (saw_typedef && s_punct_is(&t, ")") && prev.kind == DOXTER_PUNCT)
        {
          /* crude: handled by typedef_candidate updates; keep simple */
        }

        if (t.kind == DOXTER_PUNCT && t.text.length == 1)
        {
          if (t.text.ptr[0] == ';')
          {
            stmt_end = t.text.ptr + 1;
            break;
          }

          if (t.text.ptr[0] == '{')
          {
            /* Function definition: statement ends right before '{'. */
            stmt_end = t.text.ptr;
            brace_depth = 1;
            s_skip_code_block(&st);
            brace_depth = 0;
            break;
          }
        }

        prev = t;
        t = s_next_token(&st);
        if (t.kind == DOXTER_END)
        {
          stmt_end = st.input.ptr;
          break;
        }
      }

      if (stmt_end > stmt_start)
      {
        uint32_t stmt_line = 0;
        uint32_t stmt_col = 0;
        s_loc_from_ptr(source, stmt_start, &stmt_line, &stmt_col);

        DoxterSymbol sym;
        memset(&sym, 0, sizeof(sym));
        sym.comment = pending_doc;
        sym.declaration.ptr = stmt_start;
        sym.declaration.length = (size_t) (stmt_end - stmt_start);
        sym.is_typedef = saw_typedef;
        sym.is_static = saw_static;
        sym.is_empty_macro = false;
        sym.name = x_slice_empty();
        sym.line = stmt_line;
        sym.column = stmt_col;

        if (saw_struct)
        {
          sym.type = DOXTER_STRUCT;
          sym.name = name;
        }
        else if (saw_enum)
        {
          sym.type = DOXTER_ENUM;
          sym.name = name;
        }
        else if (saw_union)
        {
          sym.type = DOXTER_UNION;
          sym.name = name;
        }
        else if (saw_typedef)
        {
          sym.type = DOXTER_TYPEDEF;
          sym.name = typedef_candidate;
        }
        else if (!x_slice_is_empty(name))
        {
          sym.type = DOXTER_FUNCTION;
          sym.name = name;
        }
        else
        {
          pending_doc = x_slice_empty();
          continue;
        }

        if (!x_slice_is_empty(sym.name) && !s_find_symbol(array, sym.name) && s_filter_symbol(&sym, cfg))
        {
          x_array_push(array, &sym);
          count++;
        }
      }

      pending_doc = x_slice_empty();
    }
  }

  return count;
}

static inline const char* s_symbol_type_name(DoxterType t)
{
  switch (t)
  {
    case DOXTER_FUNCTION:
      return "<function>";
    case DOXTER_MACRO:
      return "<macro>";
    case DOXTER_STRUCT:
      return "<struct>";
    case DOXTER_ENUM:
      return "<enum>";
    case DOXTER_UNION:
      return "<union>";
    case DOXTER_TYPEDEF:
      return "<typedef>";
    case DOXTER_FILE:
      return "<FILE_COMMENT>";
    default:
      return "<?>";
  }
}

void doxter_debug_print_symbol(DoxterSymbol* symbol)
{
  if (!x_slice_is_empty(symbol->comment))
    printf("%.*s\n", (i32) symbol->comment.length, symbol->comment.ptr);

  printf("%s %d,%d - %s %s %s - %.*s\n\t%.*s\n",
      s_symbol_type_name(symbol->type),
      symbol->line, symbol->column,
      symbol->is_typedef ? "IS_TYPEDEF" : "",
      symbol->is_empty_macro ? "IS_EMPTY_MACRO" : "",
      symbol->is_static ? "IS_STATIC" : "",
      (i32) symbol->name.length, symbol->name.ptr,
      (i32) symbol->declaration.length, symbol->declaration.ptr);
}
