
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "tcl.h"
#include <stdx_strbuilder.h>

static bool tcl_is_name_start(char c)
{
  /* Tcl é permissiva; para parser inicial, use [A-Za-z_]. */
  return (c=='_') || (c>='A'&&c<='Z') || (c>='a'&&c<='z');
}

static bool tcl_is_name_char(char c)
{
  return tcl_is_name_start(c) || (c>='0'&&c<='9');
}

static void tcl_skip_ws_and_comments(const char* s, size_t n, size_t* p, bool at_cmd_start)
{
  size_t i = *p;
  for (;;)
  {
    // white space
    while (i < n && (s[i]==' ' || s[i]=='\t' || s[i]=='\r'))
      i++;
    // comments can appear whenever a command could appear
    if (at_cmd_start && i < n && s[i] == '#')
    {
      while (i < n && s[i] != '\n')
        i++;
    }
    else break;
  }
  *p = i;
}

/* Handle segment inside { ... }. No substitutions. */
static TclParseError tcl_read_brace_literal(const char* s, size_t n, size_t* io_pos,
    XSlice* out_view, size_t* out_close_pos)
{
  size_t i = *io_pos;
  size_t depth = 1;
  size_t start = i; /* após '{' inicial já consumido pelo chamador */

  while (i < n)
  {
    char c = s[i++];
    if (c == '{') depth++;
    else if (c == '}')
    {
      depth--;
      if (depth == 0)
      {
        out_view->ptr = s + start;
        out_view->length = (i - 1) - start; /* até antes do '}' final */
        *out_close_pos = i;              /* posição após '}' */
        *io_pos = i;
        return TCL_OK;
      }
    }
    /* backslash dentro de {} NÃO tem significado especial em Tcl clássico. */
  }
  return TCL_ERR_UNCLOSED_BRACE;
}

/* Lê conteúdo de " ... " com substituições habilitadas (\ escapes, $var, [cmd]). */
static TclParseError tcl_read_quoted_word_segments(const char* s, size_t n, size_t* io_pos,
    XArena* arena, TclWord** out_word);

/* Read a variable name */
static TclParseError tcl_read_var_name(const char* s, size_t n, size_t* io_pos, XSlice* out_name)
{
  size_t i = *io_pos;
  if (i >= n) return TCL_ERR_BAD_VAR_SYNTAX;

  if (s[i] == '{')
  {
    // ${...} até '}' (No nesting here) 
    i++;
    size_t start = i;
    while (i < n && s[i] != '}')
      i++;
    if (i >= n)
      return TCL_ERR_BAD_VAR_SYNTAX;
    out_name->ptr = s + start;
    out_name->length = i - start;
    *io_pos = i + 1;
    return TCL_OK;
  }
  else
  {
    if (!tcl_is_name_start(s[i])) return TCL_ERR_BAD_VAR_SYNTAX;
    size_t start = i++;
    while (i < n && tcl_is_name_char(s[i]))
      i++;
    out_name->ptr = s + start;
    out_name->length = i - start;
    *io_pos = i;
    return TCL_OK;
  }
}

/* Parseia [ ... ] como subscript (CmdSub): conteúdo é um Script recursivo até o ']' pareado. */
static TclParseError tcl_read_cmdsub(const char* s, size_t n, size_t* io_pos,
    XArena* arena, TclScript** out_script)
{
  size_t i = *io_pos; /* após '[' já consumido pelo chamador */
  int depth = 1;
  size_t start = i;
  while (i < n)
  {
    char c = s[i++];
    if (c == '[') depth++;
    else if (c == ']')
    {
      depth--;
      if (depth == 0)
      {
        size_t inner_len = (i - 1) - start;
        TclParseResult pr = tcl_parse_script(s + start, inner_len, arena);
        if (pr.error != TCL_OK) return pr.error;
        *out_script = pr.script;
        *io_pos = i;
        return TCL_OK;
      }
    }
    else if (c == '{')
    {
      /* pular bloco de chaves literal dentro do cmdsub */
      XSlice tmp; size_t close_pos = 0;
      size_t pos2 = i;
      TclParseError e = tcl_read_brace_literal(s, n, &pos2, &tmp, &close_pos);
      if (e != TCL_OK) return e;
      i = pos2;
    }
    else if (c == '"')
    {
      /* pular bloco de aspas corretamente (permitindo escapes/aninhamentos de [ ]) */
      size_t pos2 = i;
      TclWord* dummy = NULL;
      TclParseError e = tcl_read_quoted_word_segments(s, n, &pos2, arena, &dummy);
      if (e != TCL_OK) return e;
      i = pos2;
    }
    /* backslash continua no fluxo normal: parser de script não precisa transformar aqui */
  }
  return TCL_ERR_UNCLOSED_BRACKET;
}

/* Constrói/append de um segmento em uma palavra. */
static bool tcl_word_push_segment(XArena* a, TclWord* w, TclNode* seg)
{
  size_t cap = w->segment_count + 1;
  TclNode** newv = (TclNode**)x_arena_alloc(a, cap * sizeof(*newv));
  if (!newv) return false;
  /* nota: para simplicidade do esqueleto, omitimos realoc inteligente;
     em implementação real, use vetor com capacidade. */
  for (size_t i = 0; i < w->segment_count; ++i) newv[i] = w->segments ? w->segments[i] : NULL;
  newv[w->segment_count] = seg;
  w->segments = newv;
  w->segment_count++;
  return true;
}

/* Parseia uma palavra (fora de aspas/chaves), respeitando $, [ ], \ escapes e término por espaço/;/\n. */
TclParseError tcl_parse_one_word(const char* s, size_t n, size_t* io_pos,
    XArena* arena, TclWord** out_word)
{
  size_t i = *io_pos;
  TclWord* w = (TclWord*)x_arena_alloc_zero(arena, sizeof(*w));
  if (!w)
    return TCL_ERR_OOM;

  while (i < n)
  {
    char c = s[i];

    /* término da palavra por separador (mas não dentro de aspas/chaves) */
    if (c==' ' || c=='\t' || c=='\r' || c=='\n' || c==';')
      break;

    if (c == '\\')
    {
      /* escape simples: captura o próximo char como literal (inclui \n-continuation se quiser) */
      if (i + 1 < n)
      {
        TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
        if (!seg)
          return TCL_ERR_OOM;
        seg->kind = TclNode_Literal;
        seg->as.literal.ptr = s + (i + 1);
        seg->as.literal.length = 1;
        if (!tcl_word_push_segment(arena, w, seg))
          return TCL_ERR_OOM;
        i += 2;
        continue;
      }
      else
      {
        /* barra invertida final: considere inválido ou literal vazio */
        i++;
        continue;
      }
    }
    else if (c == '{')
    {
      /* bloco literal sem substituição */
      i++; /* consome '{' */
      XSlice view; size_t close_pos = 0;
      TclParseError e = tcl_read_brace_literal(s, n, &i, &view, &close_pos);
      if (e != TCL_OK)
        return e;
      TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
      if (!seg) return TCL_ERR_OOM;
      seg->kind = TclNode_Literal;
      seg->as.literal = view; /* cru */
      if (!tcl_word_push_segment(arena, w, seg))
        return TCL_ERR_OOM;
      continue;
    }
    else if (c == '"')
    {
      /* palavra entre aspas, com substituições habilitadas */
      i++; /* consome '"' */
      size_t pos2 = i;
      TclWord* qw = NULL;
      TclParseError e = tcl_read_quoted_word_segments(s, n, &pos2, arena, &qw);
      if (e != TCL_OK)
        return e;
      i = pos2;
      /* mescla segmentos de qw em w */
      for (size_t k = 0; k < qw->segment_count; ++k)
        if (!tcl_word_push_segment(arena, w, qw->segments[k]))
          return TCL_ERR_OOM;
      continue;
    }
    else if (c == '$')
    {
      i++; /* consome '$' */
      XSlice name;
      TclParseError e = tcl_read_var_name(s, n, &i, &name);
      if (e != TCL_OK) return e;
      TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
      if (!seg)
        return TCL_ERR_OOM;
      seg->kind = TclNode_Var;
      seg->as.var_name = name;
      if (!tcl_word_push_segment(arena, w, seg))
        return TCL_ERR_OOM;
      continue;
    }
    else if (c == '[')
    {
      i++; /* consome '[' */
      TclScript* sub = NULL;
      TclParseError e = tcl_read_cmdsub(s, n, &i, arena, &sub);
      if (e != TCL_OK) return e;
      TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
      if (!seg) return TCL_ERR_OOM;
      seg->kind = TclNode_CmdSub;
      seg->as.cmd_sub = sub;
      if (!tcl_word_push_segment(arena, w, seg)) return TCL_ERR_OOM;
      continue;
    }
    else
    {
      /* literal “simples” até próximo meta-char */
      size_t start = i;
      while (i < n)
      {
        char d = s[i];
        if (d=='\\' || d=='{' || d=='}' || d=='[' || d==']' || d=='"' ||
            d==' ' || d=='\t' || d=='\r' || d=='\n' || d==';' || d=='$')
          break;
        i++;
      }
      if (i > start)
      {
        TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
        if (!seg) return TCL_ERR_OOM;
        seg->kind = TclNode_Literal;
        seg->as.literal.ptr = s + start;
        seg->as.literal.length = i - start;
        if (!tcl_word_push_segment(arena, w, seg)) return TCL_ERR_OOM;
      }
      continue;
    }
  }

  *io_pos = i;
  *out_word = w;
  return TCL_OK;
}

/* Conteúdo entre aspas " ... ":
   - permite $var, [cmdsub], \-escapes
   - fecha ao encontrar '"' não escapado
   */
static TclParseError tcl_read_quoted_word_segments(const char* s, size_t n, size_t* io_pos,
    XArena* arena, TclWord** out_word)
{
  size_t i = *io_pos;
  TclWord* w = (TclWord*)x_arena_alloc_zero(arena, sizeof(*w));
  if (!w) return TCL_ERR_OOM;

  while (i < n)
  {
    char c = s[i];
    if (c == '"')
    {
      i++; /* fecha aspas */
      *io_pos = i;
      *out_word = w;
      return TCL_OK;
    }
    else if (c == '\\')
    {
      if (i + 1 < n)
      {
        TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
        if (!seg) return TCL_ERR_OOM;
        seg->kind = TclNode_Literal;
        seg->as.literal.ptr = s + (i + 1);
        seg->as.literal.length = 1;
        if (!tcl_word_push_segment(arena, w, seg)) return TCL_ERR_OOM;
        i += 2;
        continue;
      }
      else { i++; continue; }
    }
    else if (c == '$')
    {
      i++;
      XSlice name;
      TclParseError e = tcl_read_var_name(s, n, &i, &name);
      if (e != TCL_OK) return e;
      TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
      if (!seg) return TCL_ERR_OOM;
      seg->kind = TclNode_Var;
      seg->as.var_name = name;
      if (!tcl_word_push_segment(arena, w, seg)) return TCL_ERR_OOM;
      continue;
    }
    else if (c == '[')
    {
      i++;
      TclScript* sub = NULL;
      TclParseError e = tcl_read_cmdsub(s, n, &i, arena, &sub);
      if (e != TCL_OK) return e;
      TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
      if (!seg) return TCL_ERR_OOM;
      seg->kind = TclNode_CmdSub;
      seg->as.cmd_sub = sub;
      if (!tcl_word_push_segment(arena, w, seg)) return TCL_ERR_OOM;
      continue;
    }
    else
    {
      /* literal contínuo até meta */
      size_t start = i;
      while (i < n)
      {
        char d = s[i];
        if (d=='\\' || d=='$' || d=='[' || d==']' || d=='"')
          break;
        i++;
      }
      if (i > start)
      {
        TclNode* seg = (TclNode*)x_arena_alloc_zero(arena, sizeof(*seg));
        if (!seg) return TCL_ERR_OOM;
        seg->kind = TclNode_Literal;
        seg->as.literal.ptr = s + start;
        seg->as.literal.length = i - start;
        if (!tcl_word_push_segment(arena, w, seg)) return TCL_ERR_OOM;
      }
    }
  }
  return TCL_ERR_UNCLOSED_QUOTE;
}

/* Parser de script: separa comandos por ; ou \n, respeitando [] {} "" e escapes. */
TclParseResult tcl_parse_script(const char* src, size_t len, XArena* arena)
{
  TclParseResult result = {0};
  if (!src)
  {
    result.error = TCL_OK;
    return result;
  }

  TclScript* script = (TclScript*)x_arena_alloc_zero(arena, sizeof(*script));
  if (!script)
  {
    result.error = TCL_ERR_OOM;
    return result;
  }

  size_t i = 0;
  while (i < len)
  {
    // command start
    tcl_skip_ws_and_comments(src, len, &i, true);
    if (i >= len)
      break;

    // skip leading ';' and \n
    if (src[i] == '\n' || src[i] == ';')
    {
      i++;
      continue;
    }

    // Look for the end of the command ( ';' or \n )
    TclCommand* cmd = (TclCommand*)x_arena_alloc_zero(arena, sizeof(*cmd));
    if (!cmd)
    {
      result.error = TCL_ERR_OOM;
      result.err_offset = i; ;
    }

    for (;;)
    {
      tcl_skip_ws_and_comments(src, len, &i, false);
      if (i >= len) break;
      if (src[i] == '\n' || src[i] == ';')
      {
        i++; // end of command
        break;
      }

      TclWord* w = NULL;
      TclParseError e = tcl_parse_one_word(src, len, &i, arena, &w);
      if (e != TCL_OK)
      {
        result.error = e;
        result.err_offset = i;
        return result;
      }

      // append word to command
      size_t cap = cmd->word_count + 1;
      TclWord** newv = (TclWord**)x_arena_alloc(arena, cap * sizeof(*newv));
      if (!newv)
      {
        result.error = TCL_ERR_OOM;
        result.err_offset = i;
        ;
      }

      for (size_t k = 0; k < cmd->word_count; ++k)
        newv[k] = cmd->words ? cmd->words[k] : NULL;

      newv[cmd->word_count] = w;
      cmd->words = newv;
      cmd->word_count++;
    }

    // append command to script
    if (cmd->word_count > 0)
    {
      size_t cap = script->command_count + 1;
      TclCommand** newc = (TclCommand**)x_arena_alloc(arena, cap * sizeof(*newc));
      if (!newc)
      {
        result.error = TCL_ERR_OOM;
        result.err_offset = i;
        return result;
      }

      for (size_t k = 0; k < script->command_count; ++k)
        newc[k] = script->commands ? script->commands[k] : NULL;

      newc[script->command_count] = cmd;
      script->commands = newc;
      script->command_count++;
    }
  }

  result.script = script;
  result.error = TCL_OK;
  result.err_offset = 0;
  return result;
}

//
// Env functions
//

TclVar* tcl_env_find(TclEnv* E, XSlice name)
{
  for (TclVar* v = E->head; v; v = v->next)
  {
    if (v->name.length == name.length)
    {
      if (x_slice_hash(name) == v->hash && strncmp(v->name.ptr, name.ptr, name.length) == 0)
        return v;
    }
  }
  return NULL;
}

XSlice tcl_env_get(TclEnv* E, XSlice name)
{
  TclVar* v = tcl_env_find(E, name);
  if (!v)
    return (XSlice){NULL, 0};
  return v->value;
}

XSlice tcl_env_get_cstr(TclEnv* E, const char* cname)
{
  XSlice n = x_slice_from_cstr(cname);
  return tcl_env_get(E, n);
}

XSlice tcl_env_set(TclEnv* E, XSlice name, XSlice value)
{
  TclVar* v = tcl_env_find(E, name);
  if (!v)
  {
    v = (TclVar*)x_arena_alloc_zero(E->arena, sizeof(*v));
    if (!v) return (XSlice){NULL, 0};
    v->name = x_slice_from_cstr(x_arena_slicedup(E->arena, name.ptr, name.length, true));
    v->next = E->head;
    v->hash = x_slice_hash(name);
    E->head = v;
  }
  v->value = x_slice_from_cstr(x_arena_slicedup(E->arena, value.ptr, value.length, true));
  return v->value;
}

//
// Command registration and lookup
//

void tcl_command_register(TclEnv* E, const char* name, TclCmdFn fn)
{
  TclCmdDef* def = (TclCmdDef*)x_arena_alloc_zero(E->arena, sizeof(*def));
  if (!def) return; 

  def->name = x_arena_strdup(E->arena, name);
  def->fn   = fn;
  def->next = E->commands;
  E->commands = def;
}

TclCmdDef* tcl_command_find(TclEnv* E, XSlice name)
{
  for (TclCmdDef* def = E->commands; def != NULL; def = def->next)
  {
    if (x_slice_eq_cstr(name, def->name))
    {
      return def;
    }
  }
  return NULL;
}

XSlice tcl_eval_word(TclWord* W, TclEnv* E, XArena* A)
{
  XSlice empty = { NULL, 0 };

  if (!W || W->segment_count == 0)
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

  for (size_t i = 0; i < W->segment_count; ++i)
  {
    TclNode* seg = W->segments[i];
    XSlice part = empty;

    if (seg->kind == TclNode_Literal)
    {
      part = seg->as.literal;
    }
    else if (seg->kind == TclNode_Var)
    {
      /* main.c currently uses env_get() */
      part = tcl_env_get(E, seg->as.var_name);
    }
    else if (seg->kind == TclNode_CmdSub)
    {
      part = tcl_eval_script(seg->as.cmd_sub, E, A);
    }

    if (!part.ptr || part.length == 0)
      continue;

    if (count == cap)
    {
      size_t new_cap = cap * 2;
      Piece* bigger = (Piece*)x_arena_alloc(A, new_cap * sizeof(Piece));
      if (!bigger)
        return empty;

      memcpy(bigger, pieces, count * sizeof(Piece));
      if (pieces != local)
      {
        /* previous pieces array lives in the arena as well; just abandon it */
      }

      pieces = bigger;
      cap = new_cap;
    }

    pieces[count].ptr = part.ptr;
    pieces[count].len = part.length;
    count++;
    total += part.length;
  }

  if (total == 0)
    return empty;

  char* dst = (char*)x_arena_alloc(A, total + 1);
  if (!dst)
    return empty;

  char* out = dst;
  for (size_t i = 0; i < count; ++i)
  {
    memcpy(out, pieces[i].ptr, pieces[i].len);
    out += pieces[i].len;
  }
  *out = '\0';

  XSlice result;
  result.ptr = dst;
  result.length = total;
  return result;
}

XSlice tcl_eval_words_join(TclCommand* C, size_t from_idx, TclEnv* E, XArena* A)
{
  XSlice empty = { NULL, 0 };

  if (from_idx >= C->word_count)
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
  int first = 1;

  for (size_t i = from_idx; i < C->word_count; ++i)
  {
    XSlice w = tcl_eval_word(C->words[i], E, A);
    if (!w.ptr || w.length == 0)
      continue;

    if (!first)
    {
      /* add one space between words */
      if (count == cap)
      {
        size_t new_cap = cap * 2;
        Piece* bigger = (Piece*)x_arena_alloc(A, new_cap * sizeof(Piece));
        if (!bigger)
          return empty;

        memcpy(bigger, pieces, count * sizeof(Piece));
        if (pieces != local)
        {
          /* previous array stays in arena, just abandoned */
        }

        pieces = bigger;
        cap = new_cap;
      }

      pieces[count].ptr = " ";
      pieces[count].len = 1;
      count++;
      total += 1;
    }

    first = 0;

    if (count == cap)
    {
      size_t new_cap = cap * 2;
      Piece* bigger = (Piece*)x_arena_alloc(A, new_cap * sizeof(Piece));
      if (!bigger)
        return empty;

      memcpy(bigger, pieces, count * sizeof(Piece));
      if (pieces != local)
      {
        /* previous stays in arena */
      }

      pieces = bigger;
      cap = new_cap;
    }

    pieces[count].ptr = w.ptr;
    pieces[count].len = w.length;
    count++;
    total += w.length;
  }

  if (first || total == 0)
    return empty;

  char* dst = (char*)x_arena_alloc(A, total + 1);
  if (!dst)
    return empty;

  char* out = dst;
  for (size_t i = 0; i < count; ++i)
  {
    memcpy(out, pieces[i].ptr, pieces[i].len);
    out += pieces[i].len;
  }
  *out = '\0';

  XSlice result;
  result.ptr = dst;
  result.length = total;
  return result;
}

XSlice tcl_eval_script_from_text(XSlice text, TclEnv* E, XArena* A)
{
  TclParseResult pr = tcl_parse_script(text.ptr, text.length, A);
  if (pr.error != TCL_OK)
  {
    const char* msg = "parse error";
    return x_slice_from_cstr(x_arena_strdup(A, msg));
  }
  return tcl_eval_script(pr.script, E, A);
}

XSlice tcl_eval_command(TclCommand* C, TclEnv* E, XArena* A)
{
  const XSlice empty = { NULL, 0 };

  if (C->word_count == 0)
    return empty;

  // get command name
  XSlice cmd = tcl_eval_word(C->words[0], E, A);

  // Look for the command
  TclCmdDef* def = tcl_command_find(E, cmd);
  if (!def)
  {
    fprintf(stderr, "unknown command: %.*s\n",
        (int)cmd.length, cmd.ptr ? cmd.ptr : "");
    return empty;
  }
  // Inoke
  return def->fn(C, E);
}

XSlice tcl_eval_script(TclScript* S, TclEnv* E, XArena* A)
{
  XSlice last = {NULL, 0};
  for (size_t i = 0; i < S->command_count; ++i)
  {
    last = tcl_eval_command(S->commands[i], E, A);
  }
  return last;
}
