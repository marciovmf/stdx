#ifndef DOXTER_H
#define DOXTER_H

#include <stdx_common.h>
#include <stdx_string.h>
#include <stdx_array.h>

#ifndef DOXTER_COMMENT_MAX_ARGS
#define DOXTER_COMMENT_MAX_ARGS 32
#endif  // DOXTER_COMMENT_MAX_ARGS 

typedef struct DoxterConfig DoxterConfig;
typedef struct DoxterSymbol DoxterSymbol;
typedef struct DoxterComment DoxterComment;

typedef enum DoxterType
{
  DOXTER_FUNCTION,
  DOXTER_MACRO,
  DOXTER_STRUCT,
  DOXTER_ENUM,
  DOXTER_UNION,
  DOXTER_TYPEDEF,
  DOXTER_FILE
} DoxterType;

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
  const char* mono_fonts;
  const char* serif_fonts;
  u8 border_radius;

  const char* project_name;
  const char* project_url;
  const char* project_index_md;
  const char* h_files;
  const char* c_files;

  u8 option_skip_static_functions;
  u8 option_skip_undocumented_symbols; 
  u8 option_skip_empty_defines;
  u8 option_markdown_gobal_comments;
  u8 option_markdown_index_page;
};

struct DoxterSymbol
{
  uint32_t    line;           // declaration starting line
  uint32_t    column;         // declaration staing column
  XSlice      declaration;    // Actual C code of the declaration. Functions do not include { } body. Structs, macros, enums, typedefs etc should incude full code.
  XSlice      comment;        // /** ... */ right before the declaration, if any
  XSlice      name;           // Symbol name of declaration
  bool        is_typedef;     // true if it is a typedef
  bool        is_static;      // true if it is static
  bool        is_empty_macro; // true is it is an empty define macro like #define SOMENAME, without any value
  DoxterType  type;       
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

/**
 * @brief Collects all symbols from a source file. Comments are also collected
 * when present.
 * @param source Source file to scan for symbols
 * @param aray An XArray for storing the symbols
 * @return the number or symbols found
 */
u32 doxter_source_symbols_collect(XSlice source, DoxterConfig* cfg, XArray* array);

#endif //DOXTER_H

