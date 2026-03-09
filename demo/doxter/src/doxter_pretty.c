#include <stdx_common.h>
#include <stdx_hashtable.h>
#include "doxter.h"

#define INDENTATION "  "
#define FUNCTION_PARAM_INDENT " "


// --------------------------------------------------------
// Syntax Highlight render
// --------------------------------------------------------

static void s_append_html_escaped(XStrBuilder* out, XSlice s)
{
  for (size_t i = 0; i < s.length; ++i)
  {
    char c = s.ptr[i];
    if (c == '&') { x_strbuilder_append(out, "&amp;"); continue; }
    if (c == '<') { x_strbuilder_append(out, "&lt;");  continue; }
    if (c == '>') { x_strbuilder_append(out, "&gt;");  continue; }
    if (c == '"') { x_strbuilder_append(out, "&quot;"); continue; }
    x_strbuilder_append_char(out, c);
  }
}

static bool s_is_c_keyword(XSlice s)
{
  static const char* kw[] =
  {
    "auto","break","case","char","const","continue","default","do","double","else","enum",
    "extern","float","for","goto","if","inline","int","long","register","restrict","return",
    "short","signed","sizeof","static","struct","switch","typedef","union","unsigned","void",
    "volatile","while","_Alignas","_Alignof","_Atomic","_Bool","_Complex","_Generic",
    "_Imaginary","_Noreturn","_Static_assert","_Thread_local", "bool", "__FILE__", "__LINE__",
    "int32_t", "uint32_t", "int8_t", "uint8_t", "int16_t", "uint16_t"
  };

  for (size_t i = 0; i < sizeof(kw) / sizeof(kw[0]); ++i)
  {
    if (x_slice_eq_cstr(s, kw[i]))
      return true;
  }

  return false;
}

static void s_emit_tok_span_begin(XStrBuilder* out, const char* cls)
{
  x_strbuilder_append(out, "<span class=\"tok_");
  x_strbuilder_append(out, cls);
  x_strbuilder_append(out, "\">");
}

static void s_emit_tok_span_end(XStrBuilder* out)
{
  x_strbuilder_append(out, "</span>");
}

static void s_emit_token_highlighted(DoxterProject* project, XStrBuilder* out, const DoxterToken* t, const DoxterSymbol* current_symbol)
{
  const char* cls = "unk";
  bool is_href = false;

  switch (t->kind)
  {
    case DOXTER_IDENT: {
      if (current_symbol && x_slice_eq(current_symbol->name, t->text))
      {
        cls = "id";
        break;
      }

      XSmallstr key;
      x_smallstr_from_slice(t->text, &key);
      cls = s_is_c_keyword(t->text) || x_hashtable_dox_symbol_map_has(project->symbol_map, key) ? "kw" :  "id";
      break;
    }
    case DOXTER_MACRO_DIRECTIVE: cls = "pp"; break;
    case DOXTER_NUMBER:          cls = "num"; break;
    case DOXTER_STRING:          cls = "str"; break;
    case DOXTER_CHAR:            cls = "chr"; break;
    case DOXTER_PUNCT:           cls = "pun"; break;
    case DOXTER_DOX_COMMENT:     cls = "doc"; break;
    default:                     cls = "unk"; break;
  }

  s_emit_tok_span_begin(out, cls);

  /* Only link identifiers that are not C keywords and are not the current symbol name. */
  if (t->kind == DOXTER_IDENT && !s_is_c_keyword(t->text))
  {
    bool is_self = false;

    if (current_symbol && current_symbol->name.length > 0)
    {
      /* Compare by content: token slice vs current symbol name slice */
      if (current_symbol->name.length == t->text.length &&
          memcmp(current_symbol->name.ptr, t->text.ptr, t->text.length) == 0)
      {
        is_self = true;
      }
    }

    if (!is_self)
    {
      DoxterSymbol ref;
      if (doxter_symbol_map_get(project, t->text, &ref))
      {
        x_strbuilder_append_format(out, "<a href=\"%s\">", ref.anchor.buf);
        is_href = true;
      }
    }
  }

  /* Macro directive tokens are stored as "define"/"include"/etc. Add the '#'. */
  if (t->kind == DOXTER_MACRO_DIRECTIVE)
  {
    s_append_html_escaped(out, x_slice_from_cstr("#"));
  }

  s_append_html_escaped(out, t->text);

  if (is_href)
  {
    x_strbuilder_append_cstr(out, "</a>");
  }

  s_emit_tok_span_end(out);
}

static bool s_tok_is_word(DoxterTokenKind k)
{
  return k == DOXTER_IDENT || k == DOXTER_NUMBER || k == DOXTER_STRING || k == DOXTER_CHAR;
}

static bool s_tok_is_punct_char(const DoxterToken* t, char c)
{
  return t->kind == DOXTER_PUNCT && t->text.length == 1 && t->text.ptr[0] == c;
}

static bool s_tok_is_punct_slice(const DoxterToken* t, const char* s)
{
  size_t n = strlen(s);
  return t->kind == DOXTER_PUNCT && t->text.length == n && memcmp(t->text.ptr, s, n) == 0;
}

static bool s_join_need_space_simple(const DoxterToken* prev, const DoxterToken* cur, bool glue_star_to_ident)
{
  if (!prev || !cur)
  {
    return false;
  }

  /* Always separate after preprocessor directive (we print "#define" as one token). */
  if (prev->kind == DOXTER_MACRO_DIRECTIVE)
  {
    return true;
  }

  /* No space after openers. */
  if (prev->kind == DOXTER_PUNCT)
  {
    if (s_tok_is_punct_char(prev, '(') || s_tok_is_punct_char(prev, '[') || s_tok_is_punct_char(prev, '{'))
    {
      return false;
    }

    if (s_tok_is_punct_char(prev, '.') || s_tok_is_punct_slice(prev, "->") || s_tok_is_punct_slice(prev, "::"))
    {
      return false;
    }

    /* Never put a space after '*' or '&' when we are gluing them to identifiers (params style). */
    if (glue_star_to_ident && (s_tok_is_punct_char(prev, '*') || s_tok_is_punct_char(prev, '&')))
    {
      return false;
    }
  }

  /* No space before closers and separators. */
  if (cur->kind == DOXTER_PUNCT)
  {
    if (s_tok_is_punct_char(cur, ')') || s_tok_is_punct_char(cur, ']') || s_tok_is_punct_char(cur, '}'))
    {
      return false;
    }

    if (s_tok_is_punct_char(cur, ',') || s_tok_is_punct_char(cur, ';'))
    {
      return false;
    }
  }

  /* Pointer/ref: space before '*' / '&' when following a word, but optionally glue after. */
  if (cur->kind == DOXTER_PUNCT && (s_tok_is_punct_char(cur, '*') || s_tok_is_punct_char(cur, '&')))
  {
    if (prev->kind == DOXTER_PUNCT)
    {
      if (s_tok_is_punct_char(prev, '(') || s_tok_is_punct_char(prev, '[') || s_tok_is_punct_char(prev, '{'))
      {
        return false;
      }
    }

    if (s_tok_is_word(prev->kind))
    {
      return true;
    }

    if (prev->kind == DOXTER_PUNCT && (s_tok_is_punct_char(prev, ')') || s_tok_is_punct_char(prev, ']')))
    {
      return true;
    }

    return false;
  }

  /* Space after comma. */
  if (prev->kind == DOXTER_PUNCT && s_tok_is_punct_char(prev, ','))
  {
    return true;
  }

  /* Space between two word-ish tokens. */
  if (s_tok_is_word(prev->kind) && s_tok_is_word(cur->kind))
  {
    return true;
  }

  /* No space between identifier and '(' (call / declaration name). */
  if (prev->kind == DOXTER_IDENT && cur->kind == DOXTER_PUNCT && s_tok_is_punct_char(cur, '('))
  {
    return false;
  }

  return false;
}

static void s_emit_span_tokens(
    DoxterProject* project,
    DoxterTokenSpan ts,
    bool glue_star_to_ident,
    const DoxterSymbol* current_symbol,
    XStrBuilder* out)
{
  const DoxterToken* prev = NULL;

  for (u32 i = 0; i < ts.count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, ts.first + i);

    if (current_symbol &&
        current_symbol->type == DOXTER_ENUM &&
        t->kind == DOXTER_PUNCT &&
        t->text.length == 1 &&
        t->text.ptr[0] == '}')
    {
      x_strbuilder_append_char(out, '\n');
      prev = NULL;
    }

    if (prev && s_join_need_space_simple(prev, t, glue_star_to_ident))
    {
      x_strbuilder_append_char(out, ' ');
    }

    s_emit_token_highlighted(project, out, t, current_symbol);

    if (t->kind == DOXTER_PUNCT &&
        t->text.length == 1 &&
        t->text.ptr[0] == '\\')
    {
      x_strbuilder_append_char(out, '\n');
      prev = NULL;
      continue;
    }

    prev = t;
  }
}

static bool s_params_has_multiple_args(DoxterProject* project, DoxterTokenSpan params_ts)
{
  int paren_depth = 0;

  for (u32 i = 0; i < params_ts.count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, params_ts.first + i);

    if (t->kind == DOXTER_PUNCT && t->text.length == 1)
    {
      char c = t->text.ptr[0];

      if (c == '(')
      {
        paren_depth++;
      }
      else if (c == ')')
      {
        paren_depth--;
      }
      else if (c == ',' && paren_depth == 1)
      {
        return true;
      }
    }
  }

  return false;
}

static void s_emit_function_params(DoxterProject* project,
    DoxterTokenSpan params_ts, bool multiline, XStrBuilder* out)
{
  const DoxterToken* prev = NULL;
  int paren_depth = 0;

  for (u32 i = 0; i < params_ts.count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, params_ts.first + i);

    if (prev && s_join_need_space_simple(prev, t, true))
    {
      x_strbuilder_append_char(out, ' ');
    }

    s_emit_token_highlighted(project, out, t, NULL);

    if (t->kind == DOXTER_PUNCT && t->text.length == 1)
    {
      char c = t->text.ptr[0];

      if (c == '(')
      {
        paren_depth++;
        if (multiline && paren_depth == 1)
        {
          x_strbuilder_append_cstr(out, "\n");
          x_strbuilder_append_cstr(out, FUNCTION_PARAM_INDENT);

          prev = NULL;
          continue;
        }
      }
      else if (c == ')')
      {
        paren_depth--;
      }
      else if (c == ',' && paren_depth == 1)
      {
        x_strbuilder_append_cstr(out, "\n");
        x_strbuilder_append_cstr(out, FUNCTION_PARAM_INDENT);
        prev = NULL;
        continue;
      }
    }

    prev = t;
  }
}

static void s_emit_decl_function(DoxterProject* project, const DoxterSymbol* sym, XStrBuilder* out)
{
  s_emit_span_tokens(project, sym->stmt.fn.return_ts, false, sym, out);

  /* Always separate return part and name. This enforces: "char * x_func". */
  x_strbuilder_append_char(out, ' ');

  const DoxterToken* name = (const DoxterToken*)x_array_get(project->tokens, sym->stmt.fn.name_tok);
  s_emit_token_highlighted(project, out, name, sym);

  bool multiline = s_params_has_multiple_args(project, sym->stmt.fn.params_ts);
  s_emit_function_params(project, sym->stmt.fn.params_ts, multiline, out);

  x_strbuilder_append_char(out, ';');
}

static void s_emit_decl_macro(DoxterProject* project, const DoxterSymbol* sym, XStrBuilder* out)
{
  /* Emit deterministically using parser spans. */
  if (sym->stmt.macro.name_tok != 0)
  {
    DoxterTokenSpan pre;
    pre.first = sym->first_token_index;
    pre.count = (sym->stmt.macro.name_tok - sym->first_token_index) + 1;
    s_emit_span_tokens(project, pre, true, sym, out);

    if (sym->stmt.macro.args_ts.count > 0)
    {
      s_emit_span_tokens(project, sym->stmt.macro.args_ts, true, sym, out);
    }

    if (sym->stmt.macro.value_ts.count > 0)
    {
      x_strbuilder_append_char(out, ' ');
      s_emit_span_tokens(project, sym->stmt.macro.value_ts, true, sym, out);
    }

    return;
  }

  /* Fallback: emit raw tokens. */
  {
    DoxterTokenSpan ts;
    ts.first = sym->first_token_index;
    ts.count = sym->num_tokens;
    s_emit_span_tokens(project, ts, true, sym, out);
  }
}

static void s_emit_decl_record(DoxterProject* project, const DoxterSymbol* sym, XStrBuilder* out)
{
  const u32 first = sym->first_token_index;
  const u32 count = sym->num_tokens;

  const DoxterToken* prev = NULL;
  int brace_depth = 0;

  for (u32 i = 0; i < count; ++i)
  {
    const DoxterToken* t = (const DoxterToken*)x_array_get(project->tokens, first + i);

    /* Ensure last enum item doesn't glue to '}' when there's no trailing comma. */
    if (sym->type == DOXTER_ENUM && brace_depth == 1 &&
        t->kind == DOXTER_PUNCT && t->text.length == 1 && t->text.ptr[0] == '}' &&
        prev != NULL)
    {
      x_strbuilder_append_cstr(out, "\n");
      prev = NULL;
    }

    if (prev && s_join_need_space_simple(prev, t, true))
    {
      x_strbuilder_append_char(out, ' ');
    }

    s_emit_token_highlighted(project, out, t, sym);

    if (t->kind == DOXTER_PUNCT && t->text.length == 1)
    {
      char c = t->text.ptr[0];

      if (c == '{')
      {
        brace_depth++;
        if (brace_depth == 1)
        {
          bool next_is_closing_brace = false;

          if ((i + 1) < count)
          {
            const DoxterToken* n = (const DoxterToken*)x_array_get(project->tokens, first + i + 1);
            if (n->kind == DOXTER_PUNCT && n->text.length == 1 && n->text.ptr[0] == '}')
            {
              next_is_closing_brace = true;
            }
          }

          x_strbuilder_append_cstr(out, "\n");
          if (!next_is_closing_brace)
          {
            x_strbuilder_append_cstr(out, INDENTATION);
          }
          prev = NULL;
          continue;
        }
      }
      else if (c == '}')
      {
        brace_depth--;
      }
      else if (c == ';' && brace_depth == 1)
      {
        bool next_is_closing_brace = false;

        if ((i + 1) < count)
        {
          const DoxterToken* n = (const DoxterToken*)x_array_get(project->tokens, first + i + 1);
          if (n->kind == DOXTER_PUNCT && n->text.length == 1 && n->text.ptr[0] == '}')
          {
            next_is_closing_brace = true;
          }
        }

        x_strbuilder_append_cstr(out, "\n");
        if (!next_is_closing_brace)
        {
          x_strbuilder_append_cstr(out, INDENTATION);
        }
        prev = NULL;
        continue;
      }
      else if (c == ',' && sym->type == DOXTER_ENUM && brace_depth == 1)
      {
        bool next_is_closing_brace = false;

        if ((i + 1) < count)
        {
          const DoxterToken* n = (const DoxterToken*)x_array_get(project->tokens, first + i + 1);
          if (n->kind == DOXTER_PUNCT && n->text.length == 1 && n->text.ptr[0] == '}')
          {
            next_is_closing_brace = true;
          }
        }

        x_strbuilder_append_cstr(out, "\n");
        if (!next_is_closing_brace)
        {
          x_strbuilder_append_cstr(out, INDENTATION);
        }
        prev = NULL;
        continue;
      }
    }

    prev = t;
  }
}

bool doxter_pretty_format_symbol(DoxterProject *project, const DoxterSymbol *sym, XStrBuilder *out)
{
  if (!project || !sym || !out)
  {
    return false;
  }

  switch (sym->type)
  {
    case DOXTER_FUNCTION:
      s_emit_decl_function(project, sym, out);
      break;

    case DOXTER_MACRO:
      s_emit_decl_macro(project, sym, out);
      break;

    case DOXTER_STRUCT:
    case DOXTER_UNION:
    case DOXTER_ENUM:
      s_emit_decl_record(project, sym, out);
      break;

    case DOXTER_TYPEDEF:
    default:
      {
        DoxterTokenSpan ts;
        ts.first = sym->first_token_index;
        ts.count = sym->num_tokens;
        s_emit_span_tokens(project, ts, true, sym, out);
      } break;
  }

  return true;
}
