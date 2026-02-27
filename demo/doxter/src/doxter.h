#ifndef DOXTER_H
#define DOXTER_H

#include <stdx_common.h>
#include <stdx_string.h>
#include <stdx_hashtable.h>
#include <stdx_array.h>
#include <stdx_arena.h>

#ifndef DOXTER_COMMENT_MAX_ARGS
#define DOXTER_COMMENT_MAX_ARGS 32
#endif

#ifndef DOXTER_MAX_MODULES
#define DOXTER_MAX_MODULES 64
#endif

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

#define doxter_info(msg, ...)     x_log_raw(stdout, XLOG_LEVEL_INFO, XLOG_COLOR_WHITE, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define doxter_warning(msg, ...)  x_log_raw(stdout, XLOG_LEVEL_INFO, XLOG_COLOR_YELLOW, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)
#define doxter_error(msg, ...)    x_log_raw(stderr, XLOG_LEVEL_INFO, XLOG_COLOR_RED, XLOG_COLOR_BLACK, 0, msg, __VA_ARGS__, 0)

typedef struct DoxterConfig DoxterConfig;
typedef struct DoxterSymbol DoxterSymbol;
typedef struct DoxterComment DoxterComment;

typedef enum DoxterType
{
  DOXTER_FUNCTION   = 1,
  DOXTER_MACRO      = 2,
  DOXTER_STRUCT     = 3,
  DOXTER_ENUM       = 4,
  DOXTER_UNION      = 5,
  DOXTER_TYPEDEF    = 6,
  DOXTER_FILE       = 7
} DoxterType;

typedef struct DoxterTokenSpan
{
  u32 first; /* index into DoxterProject.tokens */
  u32 count; /* number of tokens */
} DoxterTokenSpan;

typedef struct DoxterSymbolFunction
{
  u32 name_tok;
  DoxterTokenSpan return_ts;  /* return type + pointer stars tied to return */
  DoxterTokenSpan params_ts;
} DoxterSymbolFunction;

typedef struct DoxterSymbolMacro
{
  u32 name_tok;              /* macro name token index */
  DoxterTokenSpan args_ts;   /* function-like macro args "(...)" including parens; empty if object-like */
  DoxterTokenSpan value_ts;  /* replacement list tokens (may be empty) */
} DoxterSymbolMacro;

typedef struct DoxterSymbolRecord
{
  u32 name_tok;              /* struct/union/enum tag token index (0 if anonymous) */
  DoxterTokenSpan body_ts;   /* tokens for "{ ... }" including braces; empty if forward decl */
} DoxterSymbolRecord;

typedef struct DoxterSymbolTypedef
{
  u32 name_tok;              /* typedef name token index (0 if unknown) */
  DoxterTokenSpan value_ts;  /* typedef RHS tokens (excluding name); optional */
} DoxterSymbolTypedef;

struct DoxterSymbol
{
  uint32_t line;
  uint32_t column;
  XSlice declaration;
  XSlice comment;
  XSlice name;
  bool is_typedef;
  bool is_static;
  bool is_empty_macro;
  u32 num_tokens;
  u32 first_token_index;
  XSmallstr anchor;
  DoxterType type;

  union
  {
    DoxterSymbolFunction fn;
    DoxterSymbolMacro macro;
    DoxterSymbolRecord record;     /* struct/union/enum */
    DoxterSymbolTypedef tdef;
  } stmt;
};

struct DoxterComment
{
  uint32_t line;
  uint32_t column;
  XSlice   declaration;   // Full /** ... */ comment text including tags
  XSlice   target;        // C code of whatever is being documented (without body)
  XSlice   name;          // Identifier extracted from target (only the symbol name)
  XSlice   doc_brief;
  XSlice   doc_return;
  XSlice   doc_arg[DOXTER_COMMENT_MAX_ARGS];
  XSlice   doc_arg_name[DOXTER_COMMENT_MAX_ARGS];
  bool     is_typedef;
  uint32_t doc_arg_count;
  DoxterType type;
};

struct DoxterConfig
{
  const char* color_page_background;
  const char* color_sidebar_background;
  const char* color_main_text;
  const char* color_secondary_text;
  const char* color_highlight;
  const char* color_light_borders;
  const char* color_code_blocks;
  const char* color_code_block_border;
  const char* font_ui;
  const char* font_heading;
  const char* font_code;
  const char* font_symbol;
  u8 border_radius;

  const char* color_tok_pp;
  const char* color_tok_kw;
  const char* color_tok_id;
  const char* color_tok_num;
  const char* color_tok_str;
  const char* color_tok_crt;
  const char* color_tok_pun;
  const char* color_tok_doc;

  const char* project_name;
  const char* project_url;
  const char* project_brief;
  const char* project_index_md;

  const char* h_files;
  const char* c_files;

  u8 skip_static_functions;
  u8 skip_undocumented;
  u8 skip_empty_defines;
  u8 markdown_gobal_comments;
  u8 markdown_index_page;
};

typedef struct
{
  char *project_index_html;
  char *project_file_item_html;
  char *file_index_html;
  char *symbol_html;
  char *index_item_html;
  char *params_html;
  char *param_item_html;
  char *return_html;
  char *file_item_html;
  char *style_css;
} DoxterTemplates;

typedef struct
{
  char *path;               // Original path as passed via cmdline
  char *base_name;          // soruce file name without path
  char *output_name;        // source output file name with html extension
  u32   num_symbols;        // number of symbols for this source in DoxterProject.symbols
  u32   first_symbol_index; // index of first symbol in DoxterProject.symbols
} DoxterSourceInfo;

typedef struct
{
  DoxterTemplates   templates;
  DoxterConfig      config;
  XHashtable        *symbol_map;
  XArena            *scratch;     // A scratch buffer.
  XArray            *tokens;      // An array of tokens.
  XArray            *symbols;     // An array of all symbols.
  uint32_t          source_count;
  DoxterSourceInfo  sources[DOXTER_MAX_MODULES];
} DoxterProject;

/**
 * @brief Collects all symbols from a source file. Comments are also collected
 * when present.
 * @param source Source file to scan for symbols
 * @param aray An XArray for storing the symbols
 * @return the number or symbols found
 */
i32 doxter_source_parse(DoxterProject* proj, u32 source_index);

#endif //DOXTER_H
