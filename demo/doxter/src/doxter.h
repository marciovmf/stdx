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

#define HASH_PARAM_NAME         0xfd31e9b4  // x_cstr_hash(PARAM_NAME)
#define HASH_PARAM_DESC         0x7ea3a222  // x_cstr_hash(PARAM_DESC)
#define HASH_PARAMS_ITEMS       0x8ce3a478  // x_cstr_hash(PARAMS_ITEMS)
#define HASH_RETURN_DESC        0xaebc0b51  // x_cstr_hash(RETURN_DESC)
#define HASH_MODULE_NAME        0xd3406445  // x_cstr_hash(MODULE_NAME)
#define HASH_MODULE_HREF        0xd718e6c3  // x_cstr_hash(MODULE_HREF)
#define HASH_FILE_NAME          0x84d18b4f  // x_cstr_hash(FILE_NAME)
#define HASH_TITLE              0x222641e9  // x_cstr_hash(TITLE)
#define HASH_FILE_BRIEF         0xcf3191b4  // x_cstr_hash(FILE_BRIEF)
#define HASH_INDEX_ITEMS        0x0ab79160  // x_cstr_hash(INDEX_ITEMS)
#define HASH_INDEX_FUNCTIONS    0x7504a135  // x_cstr_hash(INDEX_FUNCTIONS)
#define HASH_INDEX_MACROS       0xaa47b3e1  // x_cstr_hash(INDEX_MACROS)
#define HASH_INDEX_STRUCTS      0x3bec9f5a  // x_cstr_hash(INDEX_STRUCTS)
#define HASH_INDEX_ENUMS        0x73f57d5e  // x_cstr_hash(INDEX_ENUMS)
#define HASH_INDEX_TYPEDEFS     0xc9b39a08  // x_cstr_hash(INDEX_TYPEDEFS)
#define HASH_SYMBOLS            0xf8cd4226  // x_cstr_hash(SYMBOLS)
#define HASH_MODULE_LIST        0x7d2df432  // x_cstr_hash(MODULE_LIST)
#define HASH_ANCHOR             0x97f56434  // x_cstr_hash(ANCHOR)
#define HASH_NAME               0x52ba8a26  // x_cstr_hash(NAME)
#define HASH_KIND               0x82cf2383  // x_cstr_hash(KIND)
#define HASH_LINE               0x152e7967  // x_cstr_hash(LINE)
#define HASH_DECL               0x9b94baad  // x_cstr_hash(DECL)
#define HASH_BRIEF              0xf1b614ff  // x_cstr_hash(BRIEF)
#define HASH_PARAMS_BLOCK       0x1a901b5d  // x_cstr_hash(PARAMS_BLOCK)
#define HASH_RETURN_BLOCK       0x0bad90f1  // x_cstr_hash(RETURN_BLOCK)
#define HASH_COLOR_PAGE_BACKGROUND   0x42135cf7  // x_cstr_hash(COLOR_PAGE_BACKGROUND)
#define HASH_COLOR_SIDEBAR_BACKGROUND 0x1a06b7bc  // x_cstr_hash(COLOR_SIDEBAR_BACKGROUND)
#define HASH_COLOR_MAIN_TEXT    0xc92d09ee  // x_cstr_hash(COLOR_MAIN_TEXT)
#define HASH_COLOR_SECONDARY_TEXT    0x68ae2aa1  // x_cstr_hash(COLOR_SECONDARY_TEXT)
#define HASH_COLOR_HIGHLIGHT    0x736a5067  // x_cstr_hash(COLOR_HIGHLIGHT)
#define HASH_COLOR_LIGHT_BORDERS     0x4d8a6767  // x_cstr_hash(COLOR_LIGHT_BORDERS)
#define HASH_COLOR_CODE_BLOCKS  0x5da17f0f  // x_cstr_hash(COLOR_CODE_BLOCKS)
#define HASH_COLOR_CODE_BLOCK_BORDER 0xc64dbf7d  // x_cstr_hash(COLOR_CODE_BLOCK_BORDER)
#define HASH_COLOR_TOK_PP       0xd4c04b56  // x_cstr_hash(COLOR_TOK_PP)
#define HASH_COLOR_TOK_KW       0xd7aa89c0  // x_cstr_hash(COLOR_TOK_KW)
#define HASH_COLOR_TOK_ID       0x36af9c7b  // x_cstr_hash(COLOR_TOK_ID)
#define HASH_COLOR_TOK_NUM      0x12137f2e  // x_cstr_hash(COLOR_TOK_NUM)
#define HASH_COLOR_TOK_STR      0xbd03b109  // x_cstr_hash(COLOR_TOK_STR)
#define HASH_COLOR_TOK_CRT      0xb30fea39  // x_cstr_hash(COLOR_TOK_CRT)
#define HASH_COLOR_TOK_PUN      0x44c36aa1  // x_cstr_hash(COLOR_TOK_PUN)
#define HASH_COLOR_TOK_DOC      0xa590bf38  // x_cstr_hash(COLOR_TOK_DOC)
#define HASH_FONT_UI            0xcd527b83  // x_cstr_hash(FONT_UI)
#define HASH_FONT_HEADING       0xd69b0fed  // x_cstr_hash(FONT_HEADING)
#define HASH_FONT_CODE          0xfbe6395c  // x_cstr_hash(FONT_CODE)
#define HASH_FONT_SYMBOL        0xa1842d09  // x_cstr_hash(FONT_SYMBOL)
#define HASH_BORDER_RADIUS      0x3f924f4e  // x_cstr_hash(BORDER_RADIUS)


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


#if 0 
// Used to generate the defines in this header
static void generate_placeholder_defines(int argc, char** argv)
{
  const char* tags[] = {
    "PARAM_NAME", "PARAM_DESC", "PARAMS_ITEMS", "RETURN_DESC", "MODULE_NAME", "MODULE_HREF",
    "FILE_NAME", "TITLE", "FILE_BRIEF", "INDEX_ITEMS", "INDEX_FUNCTIONS", "INDEX_MACROS",
    "INDEX_STRUCTS", "INDEX_ENUMS", "INDEX_TYPEDEFS", "SYMBOLS", "MODULE_LIST", "ANCHOR",
    "NAME", "KIND", "LINE", "DECL", "BRIEF", "PARAMS_BLOCK", "RETURN_BLOCK", "COLOR_PAGE_BACKGROUND",
    "COLOR_SIDEBAR_BACKGROUND", "COLOR_MAIN_TEXT", "COLOR_SECONDARY_TEXT", "COLOR_HIGHLIGHT",
    "COLOR_LIGHT_BORDERS", "COLOR_CODE_BLOCKS", "COLOR_CODE_BLOCK_BORDER", "COLOR_TOK_PP",
    "COLOR_TOK_KW", "COLOR_TOK_ID", "COLOR_TOK_NUM", "COLOR_TOK_STR", "COLOR_TOK_CRT",
    "COLOR_TOK_PUN", "COLOR_TOK_DOC", "FONT_UI", "FONT_HEADING", "FONT_CODE", "FONT_SYMBOL",
    "BORDER_RADIUS"
  };

  const u32 max_len = 23;
  for (u32 i = 0; i < sizeof(tags); ++i)
  {
    const char* tag = tags[i];
    u32 hash = x_cstr_hash(tag);

    printf("#define HASH_%-*s 0x%08x  // x_cstr_hash(%s)\n\n",
        max_len, tag, hash, tag);
  }
}
#endif

#endif //DOXTER_H

