#include "stdx_string.h"
#include "tcl.h"
#include "tcl_runtime.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//
// Internals
//

typedef struct ExprCursor
{
  const char* s;
  size_t n;
  size_t i;
} ExprCursor;

static double tcl_expr_parse(ExprCursor* c);
static double tcl_expr_cmp(ExprCursor* c);

static void expskip(ExprCursor* c)
{
  while (c->i < c->n && isspace((unsigned char)c->s[c->i]))
    c->i++;
}

static int expmatch(ExprCursor* c, const char* tok)
{
  expskip(c);
  size_t j = c->i;
  for (size_t k = 0; tok[k]; ++k)
  {
    if (j >= c->n || c->s[j] != tok[k])
      return 0;
    j++;
  }
  c->i = j;
  return 1;
}

static double tcl_expr_number(ExprCursor* c)
{
  expskip(c);
  size_t i0 = c->i;
  int seen = 0;
  if (c->i < c->n && (c->s[c->i] == '+' || c->s[c->i] == '-'))
    c->i++;
  while (c->i < c->n && isdigit((unsigned char)c->s[c->i]))
  {
    c->i++;
    seen = 1;
  }
  if (c->i < c->n && c->s[c->i] == '.')
  {
    c->i++;
    while (c->i < c->n && isdigit((unsigned char)c->s[c->i]))
    {
      c->i++;
      seen = 1;
    }
  }
  if (!seen)
    return NAN;

  double v = strtod(c->s + i0, NULL);
  return v;
}

static double tcl_expr_primary(ExprCursor* c)
{
  expskip(c);
  if (expmatch(c, "("))
  {
    double v = tcl_expr_parse(c);
    expmatch(c, ")"); /* best-effort */
    return v;
  }
  double v = tcl_expr_number(c);
  if (!isnan(v)) return v;
  /* Sem strings/símbolos aqui. */
  return 0.0;
}

static double tcl_expr_unary(ExprCursor* c)
{
  expskip(c);
  if (expmatch(c, "+")) return tcl_expr_unary(c);
  if (expmatch(c, "-")) return -tcl_expr_unary(c);
  return tcl_expr_primary(c);
}

static double tcl_expr_mul(ExprCursor* c)
{
  double v = tcl_expr_unary(c);
  for (;;)
  {
    if (expmatch(c, "*")) v *= tcl_expr_unary(c);
    else if (expmatch(c, "/")) v /= tcl_expr_unary(c);
    else break;
  }
  return v;
}

static double tcl_expr_add(ExprCursor* c)
{
  double v = tcl_expr_mul(c);
  for (;;)
  {
    if (expmatch(c, "+")) v += tcl_expr_mul(c);
    else if (expmatch(c, "-")) v -= tcl_expr_mul(c);
    else break;
  }
  return v;
}

static double tcl_expr_cmp(ExprCursor* c)
{
  double a = tcl_expr_add(c);
  for (;;)
  {
    if (expmatch(c, "==")) { double b = tcl_expr_add(c); a = (a == b) ? 1.0 : 0.0; }
    else if (expmatch(c, "!=")) { double b = tcl_expr_add(c); a = (a != b) ? 1.0 : 0.0; }
    else if (expmatch(c, "<=")) { double b = tcl_expr_add(c); a = (a <= b) ? 1.0 : 0.0; }
    else if (expmatch(c, ">=")) { double b = tcl_expr_add(c); a = (a >= b) ? 1.0 : 0.0; }
    else if (expmatch(c, "<")) { double b = tcl_expr_add(c); a = (a < b) ? 1.0 : 0.0; }
    else if (expmatch(c, ">")) { double b = tcl_expr_add(c); a = (a > b) ? 1.0 : 0.0; }
    else break;
  }
  return a;
}

static double tcl_expr_parse(ExprCursor* c)
{
  return tcl_expr_cmp(c);
}

static XSlice substitute_vars_in_expr(XArena* A, TclEnv* E, XSlice expr_sv)
{
  XSlice empty = { NULL, 0 };

  const char* s = expr_sv.ptr;
  size_t n = expr_sv.length;

  if (!s || n == 0)
    return empty;

  typedef struct Piece
  {
    const char* ptr;
    size_t      len;
  } Piece;

  Piece local[8];
  Piece* pieces = local;
  size_t cap = 8;
  size_t count = 0;
  size_t total = 0;

  /* small helper to append a literal piece */
#define ADD_PIECE(p_, l_)                                                            \
  do                                                                                 \
  {                                                                                  \
    const char* _p = (p_);                                                           \
    size_t _l = (l_);                                                                \
    if (_p && _l > 0)                                                                \
    {                                                                                \
      if (count == cap)                                                              \
      {                                                                              \
        size_t new_cap = cap * 2;                                                    \
        Piece* bigger = (Piece*)x_arena_alloc(A, new_cap * sizeof(Piece));          \
        if (!bigger)                                                                 \
        return empty;                                                              \
        memcpy(bigger, pieces, count * sizeof(Piece));                               \
        if (pieces != local)                                                         \
        {                                                                            \
          /* previous array stays in arena, abandoned */                             \
        }                                                                            \
        pieces = bigger;                                                             \
        cap = new_cap;                                                               \
      }                                                                              \
      pieces[count].ptr = _p;                                                        \
      pieces[count].len = _l;                                                        \
      count++;                                                                       \
      total += _l;                                                                   \
    }                                                                                \
  } while (0)

  size_t i = 0;

  while (i < n)
  {
    char c = s[i];

    if (c != '$')
    {
      /* copy run of non-$ literal characters in one go */
      size_t start = i;
      while (i < n && s[i] != '$')
      {
        i++;
      }
      ADD_PIECE(s + start, i - start);
      continue;
    }

    /* we saw a '$' */
    i++;

    if (i < n && s[i] == '{')
    {
      /* ${name} form */
      i++;
      size_t start = i;
      while (i < n && s[i] != '}')
      {
        i++;
      }

      if (i < n && s[i] == '}')
      {
        XSlice name;
        name.ptr = s + start;
        name.length = i - start;

        XSlice val = tcl_env_get(E, name);
        if (val.ptr && val.length > 0)
          ADD_PIECE(val.ptr, val.length);

        i++; /* skip '}' */
        continue;
      }

      /* no closing '}' – treat "${" literally and continue from start */
      ADD_PIECE("$", 1);
      ADD_PIECE("{", 1);
      /* don't advance further; loop will continue from 'start' */
      continue;
    }
    else
    {
      /* $name form, name = [A-Za-z_][A-Za-z0-9_]* */
      size_t start = i;

      if (i < n && (isalpha((unsigned char)s[i]) || s[i] == '_'))
      {
        i++;
        while (i < n &&
            (isalnum((unsigned char)s[i]) || s[i] == '_'))
        {
          i++;
        }

        XSlice name;
        name.ptr = s + start;
        name.length = i - start;

        XSlice val = tcl_env_get(E, name);
        if (val.ptr && val.length > 0)
          ADD_PIECE(val.ptr, val.length);

        continue;
      }

      /* $ not followed by valid name → literal '$' */
      ADD_PIECE("$", 1);
      /* do not consume s[i]; next loop iteration handles it */
      continue;
    }
  }

#undef ADD_PIECE

  if (total == 0)
    return empty;

  char* dst = (char*)x_arena_alloc(A, total + 1);
  if (!dst)
    return empty;

  char* out = dst;
  for (size_t k = 0; k < count; ++k)
  {
    memcpy(out, pieces[k].ptr, pieces[k].len);
    out += pieces[k].len;
  }
  *out = '\0';

  XSlice result;
  result.ptr = dst;
  result.length = total;
  return result;
}

static XSlice eval_expr_to_sv(XArena* A, TclEnv* E, XSlice expr_word)
{
  XSlice expanded = substitute_vars_in_expr(A, E, expr_word);
  ExprCursor cur = { expanded.ptr, expanded.length, 0 };
  double val = tcl_expr_parse(&cur);
  char buf[64];
  int m = snprintf(buf, sizeof(buf), "%.15g", val);
  return x_slice_from_cstr(x_arena_slicedup(A, buf, (size_t)m, false));
}


//
// Runtime initialization util
//

bool tcl_env_init(TclEnv* env, TclParseResult* script)
{
  return false;
}


bool tcl_env_terminate(TclEnv* env)
{
  return false;
}

//
// Commands
//


XSlice tcl_cmd_set(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  if (C->word_count < 2)
    return x_slice_from_cstr(x_arena_strdup(A, ""));
  //return sv_from_cstr(A, "");
  XSlice name = tcl_eval_word(C->words[1], E, A);
  if (C->word_count == 2)
  {
    return tcl_env_get(E, name);
  }
  XSlice value = tcl_eval_word(C->words[2], E, A);
  return tcl_env_set(E, name, value);
}

XSlice tcl_cmd_expr(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  XSlice e = tcl_eval_words_join(C, 1, E, A);
  return eval_expr_to_sv(A, E, e);
}

XSlice tcl_cmd_puts(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  XSlice line = tcl_eval_words_join(C, 1, E, A);
  fwrite(line.ptr ? line.ptr : "", 1, line.length, stdout);
  fputc('\n', stdout);
  return (XSlice){NULL, 0};
}

XSlice tcl_cmd_incr(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  if (C->word_count < 2)
    //return sv_from_cstr(A, "0");
    return x_slice_from_cstr(x_arena_strdup(A, "0"));
  XSlice name = tcl_eval_word(C->words[1], E, A);
  XSlice curv = tcl_env_get(E, name);
  double base = 0.0;
  if (curv.ptr)
    base = strtod(curv.ptr, NULL);
  double inc = 1.0;
  if (C->word_count >= 3)
  {
    XSlice iv = tcl_eval_word(C->words[2], E, A);
    inc = strtod(iv.ptr ? iv.ptr : "0", NULL);
  }
  double nv = base + inc;
  char buf[64];
  int m = snprintf(buf, sizeof(buf), "%.15g", nv);
  XSlice newv = x_slice_from_cstr(x_arena_slicedup(A, buf, (size_t)m, true));
  tcl_env_set(E, name, newv);
  return newv;
}

/* if {cond} {body} elseif {cond} {body} else {body} */
XSlice tcl_cmd_if(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  size_t i = 1;
  while (i + 1 < C->word_count)
  {
    XSlice kw = tcl_eval_word(C->words[i], E, A);
    if (x_slice_eq_cstr(kw, "elseif"))
    {
      i++;
      continue;
    }

    XSlice cond_sv = tcl_eval_word(C->words[i], E, A);
    XSlice body_sv = (i + 1 < C->word_count)
      ? tcl_eval_word(C->words[i + 1], E, A)
      : x_slice_from_cstr("");

    XSlice cond_num = eval_expr_to_sv(A, E, cond_sv);
    double cval = strtod(cond_num.ptr ? cond_num.ptr : "0", NULL);
    if (cval != 0.0)
    {
      return tcl_eval_script_from_text(body_sv, E, A);
    }

    i += 2;

    if (i < C->word_count)
    {
      XSlice nextkw = tcl_eval_word(C->words[i], E, A);
      if (x_slice_eq_cstr(nextkw, "else"))
      {
        if (i + 1 < C->word_count)
        {
          XSlice else_body = tcl_eval_word(C->words[i + 1], E, A);
          return tcl_eval_script_from_text(else_body, E, A);
        }
      }
    }
  }
  return (XSlice){NULL, 0};
}

/* while {cond} {body} — fixed version */
XSlice tcl_cmd_while(TclCommand* C, TclEnv* E)
{
  XArena* A = E->arena;
  if (C->word_count < 3)
    return (XSlice){NULL, 0};

  XSlice body_sv = tcl_eval_word(C->words[2], E, A);

  for (;;)
  {
    /* Re-evaluate condition each iteration */
    XSlice cond_word = tcl_eval_word(C->words[1], E, A);
    XSlice cond_num = eval_expr_to_sv(A, E, cond_word);
    double cval = strtod(cond_num.ptr ? cond_num.ptr : "0", NULL);

    if (cval == 0.0)
      break;

    (void)tcl_eval_script_from_text(body_sv, E, A);
  }

  return (XSlice){NULL, 0};
}
