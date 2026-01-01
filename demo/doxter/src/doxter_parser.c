#include "doxter.h"
#include <stdx_array.h>
#include <stdx_string.h>
#include <stdx_io.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct DoxterTokenizer
{
  XSlice input;
  u32 line;
  u32 column;
  bool at_bol;                // beginning of line (after newline)
  bool in_pp;                 // currently tokenizing a preprocessor directive
  bool pp_line_continuation;  // last returned token was "\" while in_pp
} DoxterTokenizer;

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

static XSlice s_cleanup_comment(DoxterProject* proj, XSlice comment)
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

  // Worst-case output is <= input length (+1 for NUL).
  char* out = x_arena_alloc(proj->scratch, comment.length + 1);
  size_t out_len = 0;

  XSlice c = comment;
  XSlice line;

  while (x_slice_next_token(&c, '\n', &line))
  {
    const char* p = line.ptr;
    const char* end = line.ptr + line.length;

    // Skip leading whitespace.
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r'))
    {
      p++;
    }

    // Skip leading '*' characters (matches your current behavior).
    while (p < end && *p == '*')
    {
      p++;
    }

    // Skip one space after '*' if present.
    if (p < end && *p == ' ')
    {
      p++;
    }

    // Copy rest of the line.
    size_t n = (size_t)(end - p);
    if (n > 0)
    {
      memcpy(out + out_len, p, n);
      out_len += n;
    }

    // Preserve newline between lines
    if (c.length > 0)
    {
      out[out_len++] = '\n';
    }
  }

  // Trim trailing whitespace/newlines to mimic x_slice_trim-ish end behavior.
  while (out_len > 0)
  {
    char ch = out[out_len - 1];
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
    {
      out_len--;
      continue;
    }
    break;
  }

  out[out_len] = '\0';

  XSlice result;
  result.ptr = out;
  result.length = out_len;
  return result;
}

// --------------------------------------------------------
// Tokenizer
// --------------------------------------------------------

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

    // In PP mode, a newline can end the directive. We handle that in next_token.
    char c = s->input.ptr[0];

    // Whitespace
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

    // Line comment
    if (s->input.length >= 2 && s->input.ptr[0] == '/' && s->input.ptr[1] == '/')
    {
      s_tokenizer_advance(s, 2);
      while (s->input.length > 0 && s->input.ptr[0] != '\n')
      {
        s_tokenizer_advance(s, 1);
      }
      continue;
    }

    // Block comment (doc handled elsewhere)
    if (s->input.length >= 2 && s->input.ptr[0] == '/' && s->input.ptr[1] == '*')
    {
      // If it's doc, stop: caller will emit it as a token.
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

    // Track newlines for at_bol
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

  // Consume opening quote
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
    if ((c >= '0' && c <= '9') || c == '.' || c == 'x' ||
        c == 'X' || (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F') || c == 'u' ||
        c == 'U' || c == 'l' || c == 'L')
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
  s_tokenizer_advance(s, 1); // '#'

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

static bool s_tok_is_punct_text(DoxterToken* t, char c)
{
  return t->kind == DOXTER_PUNCT && t->text.length == 1 && t->text.ptr[0] == c;
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

static u32 s_find_matching_paren(DoxterProject* proj, u32 open_i, u32 first, u32 end)
{
  u32 depth = 0;
  for (u32 i = open_i; i < end; ++i)
  {
    DoxterToken* t = x_array_get(proj->tokens, i).ptr;

    if (s_tok_is_punct_text(t, '('))
    {
      depth++;
    }
    else if (s_tok_is_punct_text(t, ')'))
    {
      depth--;
      if (depth == 0)
      {
        return i;
      }
    }
  }
  return 0;
}

static u32 s_find_matching_brace(DoxterProject* proj, u32 open_i, u32 first, u32 end)
{
  u32 depth = 0;
  for (u32 i = open_i; i < end; ++i)
  {
    DoxterToken* t = x_array_get(proj->tokens, i).ptr;

    if (s_tok_is_punct_text(t, '{'))
    {
      depth++;
    }
    else if (s_tok_is_punct_text(t, '}'))
    {
      depth--;
      if (depth == 0)
      {
        return i;
      }
    }
  }
  return 0;
}

// --------------------------------------------------------
// Symbol composing
// --------------------------------------------------------

static void s_symbol_fill_function_stmt(DoxterProject* proj, DoxterSymbol* sym)
{
  sym->stmt.fn.name_tok = 0;
  sym->stmt.fn.return_ts.first = 0;
  sym->stmt.fn.return_ts.count = 0;
  sym->stmt.fn.params_ts.first = 0;
  sym->stmt.fn.params_ts.count = 0;

  const u32 first = sym->first_token_index;
  const u32 end = first + sym->num_tokens;

  /* Find top-level '(' that begins the parameter list */
  u32 paren_depth = 0;
  u32 open_paren = 0;

  for (u32 i = first; i < end; ++i)
  {
    DoxterToken* t = x_array_get(proj->tokens, i).ptr;

    if (s_tok_is_punct_text(t, '('))
    {
      if (paren_depth == 0)
      {
        open_paren = i;
        break;
      }
      paren_depth++;
    }
    else if (s_tok_is_punct_text(t, ')'))
    {
      if (paren_depth > 0)
      {
        paren_depth--;
      }
    }
  }

  if (open_paren == 0 || open_paren <= first)
  {
    return;
  }

  /* Name token: the identifier right before '(' (common prototypes) */
  u32 name_i = open_paren - 1;
  DoxterToken* name_t = x_array_get(proj->tokens, name_i).ptr;
  if (name_t->kind != DOXTER_IDENT)
  {
    return;
  }

  sym->stmt.fn.name_tok = name_i;

  /* Params span: ( ... ) including parens */
  u32 close_paren = s_find_matching_paren(proj, open_paren, first, end);
  if (close_paren != 0 && close_paren >= open_paren)
  {
    sym->stmt.fn.params_ts.first = open_paren;
    sym->stmt.fn.params_ts.count = (close_paren - open_paren) + 1;
  }

  /* Return span: everything before name token */
  sym->stmt.fn.return_ts.first = first;
  sym->stmt.fn.return_ts.count = (name_i > first) ? (name_i - first) : 0;
}

static void s_symbol_fill_macro_stmt(DoxterProject* proj, DoxterSymbol* sym)
{
  sym->stmt.macro.name_tok = 0;
  sym->stmt.macro.args_ts.first = 0;
  sym->stmt.macro.args_ts.count = 0;
  sym->stmt.macro.value_ts.first = 0;
  sym->stmt.macro.value_ts.count = 0;

  const u32 first = sym->first_token_index;
  const u32 end = first + sym->num_tokens;

  /* Expected: MACRO_DIRECTIVE then macro name ident */
  u32 i = first;
  if (i >= end)
  {
    return;
  }

  DoxterToken* d = x_array_get(proj->tokens, i).ptr;
  if (d->kind != DOXTER_MACRO_DIRECTIVE)
  {
    return;
  }

  i++;
  while (i < end)
  {
    DoxterToken* t = x_array_get(proj->tokens, i).ptr;
    if (t->kind == DOXTER_IDENT)
    {
      sym->stmt.macro.name_tok = i;
      i++;
      break;
    }
    i++;
  }

  if (sym->stmt.macro.name_tok == 0)
  {
    return;
  }

  /* Optional function-like args: immediately following '(' */
  if (i < end)
  {
    DoxterToken* t = x_array_get(proj->tokens, i).ptr;
    if (s_tok_is_punct_text(t, '('))
    {
      u32 close_paren = s_find_matching_paren(proj, i, first, end);
      if (close_paren != 0)
      {
        sym->stmt.macro.args_ts.first = i;
        sym->stmt.macro.args_ts.count = (close_paren - i) + 1;
        i = close_paren + 1;
      }
    }
  }

  /* Value: remainder */
  if (i < end)
  {
    sym->stmt.macro.value_ts.first = i;
    sym->stmt.macro.value_ts.count = end - i;
  }
}

static void s_symbol_fill_record_stmt(DoxterProject* proj, DoxterSymbol* sym)
{
  sym->stmt.record.name_tok = 0;
  sym->stmt.record.body_ts.first = 0;
  sym->stmt.record.body_ts.count = 0;

  const u32 first = sym->first_token_index;
  const u32 end = first + sym->num_tokens;

  /* Find 'struct/union/enum' keyword then optional tag name */
  for (u32 i = first; i + 1 < end; ++i)
  {
    DoxterToken* t = x_array_get(proj->tokens, i).ptr;

    if (t->kind == DOXTER_IDENT)
    {
      /* crude: your classifier already set sym->type; we only need the tag name near it */
      if ((sym->type == DOXTER_STRUCT || sym->type == DOXTER_UNION || sym->type == DOXTER_ENUM) &&
          (x_slice_eq_cstr(t->text, "struct") || x_slice_eq_cstr(t->text, "union") || x_slice_eq_cstr(t->text, "enum")))
      {
        DoxterToken* n = x_array_get(proj->tokens, i + 1).ptr;
        if (n->kind == DOXTER_IDENT)
        {
          sym->stmt.record.name_tok = i + 1;
        }
      }
    }

    if (s_tok_is_punct_text(t, '{'))
    {
      u32 close_brace = s_find_matching_brace(proj, i, first, end);
      if (close_brace != 0)
      {
        sym->stmt.record.body_ts.first = i;
        sym->stmt.record.body_ts.count = (close_brace - i) + 1;
      }
      return;
    }
  }
}

static void s_symbol_fill_typedef_stmt(DoxterProject* proj, DoxterSymbol* sym)
{
  sym->stmt.tdef.name_tok = 0;
  sym->stmt.tdef.value_ts.first = 0;
  sym->stmt.tdef.value_ts.count = 0;

  const u32 first = sym->first_token_index;
  const u32 end = first + sym->num_tokens;

  /* For now: typedef name is the last IDENT before ';' */
  u32 last_ident = 0;

  for (u32 i = first; i < end; ++i)
  {
    DoxterToken* t = x_array_get(proj->tokens, i).ptr;
    if (t->kind == DOXTER_IDENT)
    {
      last_ident = i;
    }
  }

  if (last_ident != 0)
  {
    sym->stmt.tdef.name_tok = last_ident;

    /* RHS span excludes the name itself */
    sym->stmt.tdef.value_ts.first = first;
    sym->stmt.tdef.value_ts.count = (last_ident > first) ? (last_ident - first) : 0;
  }
}

/* returns next token (or END). */
static DoxterToken s_tokenizer_next_token(DoxterTokenizer* s, u32* out_line, u32* out_col)
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

    // Doc comment tokenization: must be returned whole
    if (x_slice_starts_with_cstr(s->input, "/**"))
    {
      return s_tokenizer_read_doc_comment(s);
    }

    // Preprocessor end: s_tokenizer_skip_trivia stops at '\n' in pp mode when not continued.
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

    // Macro directive at BOL: '#define' and '# define' are the same token.
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

static void s_symbol_dup_slices(DoxterProject* proj, DoxterSymbol* sym)
{
  sym->comment.ptr = (sym->comment.length ?
      x_arena_slicedup(proj->scratch,
        sym->comment.ptr, sym->comment.length, true)
      : "");
  sym->declaration.ptr = (sym->declaration.length ?
      x_arena_slicedup(proj->scratch,
        sym->declaration.ptr, sym->declaration.length, true)
      : "");
  sym->name.ptr = (sym->name.length ?
      x_arena_slicedup(proj->scratch,
        sym->name.ptr, sym->name.length, true)
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

    // PP_END is a structural token; it is not part of any declaration text.
    if (t.kind == DOXTER_PP_END)
    {
      continue;
    }

    t = s_token_dup(proj, t);
    x_array_push(proj->tokens, &t);
    (*out_count)++;
  }
}

#ifdef DOXTER_DEBUG_PRINT

static void s_debug_print_token_span( DoxterProject* proj, const char* label, DoxterTokenSpan span)
{
  printf("    %s: ", label);

  if (span.count == 0)
  {
    printf("(empty)\n");
    return;
  }

  for (u32 i = 0; i < span.count; ++i)
  {
    u32 ti = span.first + i;
    DoxterToken* t = x_array_get(proj->tokens, ti).ptr;

    printf("'%.*s'", (int)t->text.length, t->text.ptr);

    if (i + 1 < span.count)
    {
      printf(" ");
    }
  }

  printf("\n");
}

static void s_debug_print_token_at( DoxterProject* proj, const char* label, u32 tok_index)
{
  if (tok_index == 0)
  {
    printf("    %s: (none)\n", label);
    return;
  }

  DoxterToken* t = x_array_get(proj->tokens, tok_index).ptr;
  printf(
    "    %s: '%.*s'\n",
    label,
    (int)t->text.length,
    t->text.ptr);
}

void doxter_debug_print_symbol( DoxterProject* proj, const DoxterSymbol* sym)
{
  printf("--------------------------------------------------\n");
  printf("Symbol\n");
  printf("  type: %d\n", sym->type);
  printf("  line: %u\n", sym->line);

  if (sym->name.length > 0)
  {
    printf("  name: %.*s\n", (int)sym->name.length, sym->name.ptr);
  }
  else
  {
    printf("  name: (none)\n");
  }

  printf(
    "  tokens: [%u .. %u)\n",
    sym->first_token_index,
    sym->first_token_index + sym->num_tokens);

  /* Type-specific payload */

  switch (sym->type)
  {
    case DOXTER_FUNCTION:
    {
      const DoxterSymbolFunction* fn = &sym->stmt.fn;
      printf("  FUNCTION\n");

      s_debug_print_token_at(proj, "name_tok", fn->name_tok);
      s_debug_print_token_span(proj, "params_span", fn->params_ts);
      s_debug_print_token_span(proj, "return_span", fn->return_ts);
    } break;

    case DOXTER_MACRO:
    {
      const DoxterSymbolMacro* m = &sym->stmt.macro;
      printf("  MACRO\n");

      s_debug_print_token_at(proj, "name_tok", m->name_tok);
      s_debug_print_token_span(proj, "args_span", m->args_ts);
      s_debug_print_token_span(proj, "value_span", m->value_ts);
    } break;

    case DOXTER_STRUCT:
    case DOXTER_UNION:
    case DOXTER_ENUM:
    {
      const DoxterSymbolRecord* r = &sym->stmt.record;
      printf("  RECORD\n");

      s_debug_print_token_at(proj, "tag_tok", r->name_tok);
      s_debug_print_token_span(proj, "body_span", r->body_ts);
    } break;

    case DOXTER_TYPEDEF:
    {
      const DoxterSymbolTypedef* td = &sym->stmt.tdef;
      printf("  TYPEDEF\n");

      s_debug_print_token_at(proj, "name_tok", td->name_tok);
      s_debug_print_token_span(proj, "value_span", td->value_ts);
    } break;

    default:
    {
      printf("  (no structured payload)\n");
    } break;
  }

  printf("\n");
}

#endif // DOXTER_DEBUG_PRINT

static bool s_skip_code_block(XSlice* input)
{
  if (input->length == 0 || *input->ptr != '{')
  {
    return false;
  }

  size_t i = 0;
  i32 depth = 0;
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
  if (cfg->skip_empty_defines && sym->is_empty_macro)
    return false;

  if (cfg->skip_static_functions && sym->is_static)
    return false;

  if (cfg->skip_undocumented && x_slice_is_empty(sym->comment))
    return false;

  return true;
}

static bool s_classify_statement(XSlice stmt, XSlice comment, u32 line, u32 col, DoxterSymbol* out)
{
  XSlice trimmed = x_slice_trim(stmt);
  if (trimmed.length == 0)
    return false;

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

  // Preferred typedef name for function pointers: (*Name)
  XSlice fp_typedef_name = x_slice_empty();
  i32 fp_state = 0;
  i32 paren_depth = 0;

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
      // Track parentheses depth for top-level classification.
      if (t.text.length == 1 && t.text.ptr[0] == '(')
      {
        // Function name is the identifier immediately before '(' at top-level.
        if (!saw_typedef && paren_depth == 0 && prev.kind == DOXTER_IDENT)
        {
          fn_name = prev.text;
        }

        // Function-pointer typedef name pattern: typedef ... (*Name)(...)
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

static bool s_symbol_push(DoxterProject* proj, DoxterSymbol* sym)
{
  if (! s_filter_symbol(sym, &proj->config))
    return false;

  memset(&sym->stmt, 0, sizeof(sym->stmt));
  if (sym->type == DOXTER_FUNCTION)
  {
    s_symbol_fill_function_stmt(proj, sym);
  }
  else if (sym->type == DOXTER_MACRO)
  {
    s_symbol_fill_macro_stmt(proj, sym);
  }
  else if (sym->type == DOXTER_STRUCT || sym->type == DOXTER_UNION || sym->type == DOXTER_ENUM)
  {
    s_symbol_fill_record_stmt(proj, sym);
  }
  else if (sym->type == DOXTER_TYPEDEF)
  {
    s_symbol_fill_typedef_stmt(proj, sym);
  }

#ifdef DOXTER_DEBUG_PRINT
  doxter_debug_print_symbol(proj, sym);
#endif

  x_array_push(proj->symbols, sym);
  return true;
}

static void s_reset_stmt_state(
  const char** stmt_start,
  u32* stmt_line,
  u32* stmt_col,
  bool* seen_equal,
  DoxterToken* prev_sig,
  DoxterToken* prev_sig2)
{
  *stmt_start = NULL;
  *stmt_line = 0;
  *stmt_col = 0;
  *seen_equal = false;
  *prev_sig = (DoxterToken){0};
  *prev_sig2 = (DoxterToken){0};
}

static void s_emit_symbol_from_stmt(
  DoxterProject* proj,
  XSlice stmt,
  XSlice pending_doc,
  u32 stmt_line,
  u32 stmt_col,
  i32* count)
{
  stmt = x_slice_trim(stmt);

  if (stmt.length == 0)
  {
    return;
  }

  DoxterSymbol sym;
  if (!s_classify_statement(stmt, pending_doc, stmt_line, stmt_col, &sym))
  {
    return;
  }

  if (s_find_symbol(proj->symbols, sym.name))
  {
    return;
  }

  s_symbol_dup_slices(proj, &sym);
  s_symbol_collect_tokens(proj, sym.declaration, &sym.first_token_index, &sym.num_tokens);
  if (s_symbol_push(proj, &sym))
    (*count)++;
}

i32 doxter_source_parse(DoxterProject* proj, u32 source_index)
{
  DoxterSourceInfo* source_info = &proj->sources[source_index];
  source_info->first_symbol_index = x_array_count(proj->symbols);
  source_info->num_symbols = 0;

  size_t file_size = 0;
  const char* source_file_name = source_info->path;
  char* input = x_io_read_text(source_file_name, &file_size);
  if (!input)
  {
    return -1;
  }

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

  u32 tok_line = 1;
  u32 tok_col = 1;

  /* File symbol: if file begins with a dox comment. */
  {
    DoxterTokenizer tmp = ts;
    DoxterToken ft = s_tokenizer_next_token(&tmp, &tok_line, &tok_col);
    if (ft.kind == DOXTER_DOX_COMMENT && ft.text.ptr == source.ptr)
    {
      XSlice file_comment = s_cleanup_comment(proj, ft.text);

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
      if (s_symbol_push(proj, &sym))
        count++;
      ts = tmp;
    }
  }

  const char* stmt_start = NULL;
  u32 stmt_line = 0;
  u32 stmt_col = 0;
  bool seen_equal = false;

  i32 brace_depth = 0;
  i32 brace_top = 0;
  u8 brace_stack[512];
  i32 extern_depth = 0;

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
      /* Keep parser deterministic: comment becomes pending doc and nothing else. */
      pending_doc = s_cleanup_comment(proj, t.text);
      continue;
    }

    if (t.kind == DOXTER_MACRO_DIRECTIVE)
    {
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
        if (s_symbol_push(proj, &sym))
          count++;
      }

      pending_doc = x_slice_empty();
      s_reset_stmt_state(&stmt_start, &stmt_line, &stmt_col, &seen_equal, &prev_sig, &prev_sig2);
      brace_depth = 0;
      brace_top = 0;
      extern_depth = 0;
      continue;
    }

    /* Effective depth ignoring extern "C" blocks */
    i32 effective_depth = brace_depth - extern_depth;
    if (effective_depth < 0)
    {
      effective_depth = 0;
    }

    /* Start of a top-level statement */
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
      bool is_extern_c =
        (prev_sig2.kind == DOXTER_IDENT && x_slice_eq_cstr(prev_sig2.text, "extern") &&
         prev_sig.kind == DOXTER_STRING && x_slice_eq_cstr(prev_sig.text, "\"C\""));

      /* Deterministic function-body boundary:  ) {  at top-level, not extern "C", not assignment */
      if (!is_extern_c && effective_depth == 0 && s_token_is_punct(prev_sig, ')') && !seen_equal)
      {
        XSlice sig;
        sig.ptr = stmt_start ? stmt_start : t.text.ptr;
        sig.length = (size_t)(t.text.ptr - sig.ptr);

        s_emit_symbol_from_stmt(proj, sig, pending_doc, stmt_line, stmt_col, &count);

        pending_doc = x_slice_empty();
        stmt_start = NULL;
        seen_equal = false;

        /* Skip the function body block */
        {
          XSlice tmp;
          tmp.ptr = t.text.ptr;
          tmp.length = (size_t)((source.ptr + source.length) - t.text.ptr);

          if (s_skip_code_block(&tmp))
          {
            const char* target = tmp.ptr;
            if (target > ts.input.ptr)
            {
              size_t delta = (size_t)(target - ts.input.ptr);
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

      /* Regular brace nesting */
      if (brace_top < (int)(sizeof(brace_stack) / sizeof(brace_stack[0])))
      {
        brace_stack[brace_top++] = (u8)(is_extern_c ? 1 : 0);
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
        u8 was_extern = brace_stack[--brace_top];
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
      stmt.length = (size_t)((t.text.ptr + t.text.length) - stmt_start);

      s_emit_symbol_from_stmt(proj, stmt, pending_doc, stmt_line, stmt_col, &count);

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

  free(input);

  source_info->num_symbols = count;
  return count;
}
