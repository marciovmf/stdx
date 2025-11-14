#ifndef X_TCL_H
#define X_TCL_H

#include <stdx_common.h>
#include <stdx_strbuilder.h>
#include <stdx_arena.h>
#include <stdx_string.h>

typedef struct TclNode TclNode;
typedef struct TclWord TclWord;
typedef struct TclCommand TclCommand;
typedef struct TclScript TclScript;

struct TclWord
{
  TclNode** segments;
  size_t    segment_count;
};

struct TclCommand
{
  TclWord** words;
  size_t    word_count;
};

struct TclScript
{
  TclCommand** commands;
  size_t       command_count;
};

typedef enum TclParseError
{
  TCL_OK = 0,
  TCL_ERR_OOM,
  TCL_ERR_UNCLOSED_BRACE,
  TCL_ERR_UNCLOSED_BRACKET,
  TCL_ERR_UNCLOSED_QUOTE,
  TCL_ERR_BAD_VAR_SYNTAX,
  TCL_ERR_INVALID_CHAR,
} TclParseError;

typedef struct TclParseResult
{
  TclScript*    script;
  TclParseError error;
  size_t        err_offset; // byte where error was detected
} TclParseResult;

typedef enum TclNodeKind
{
  TclNode_Literal,
  TclNode_Var,      /* $name ou ${name} */
  TclNode_CmdSub    /* [ script ] */
} TclNodeKind;

struct TclNode
{
  TclNodeKind kind;
  union
  {
    // Literal: bytes finais da palavra, já com escapes resolvidos
    //   (exceto em blocos { ... }, onde ficam crus). 
    XSlice literal;

    // Var: nome da variável sem o '$' (para ${...}, sem as chaves).
    XSlice var_name;

    // CmdSub: script aninhado entre [ ... ].
    TclScript* cmd_sub;
  } as;
};

TclParseResult tcl_parse_script(const char* src, size_t len, XArena* arena);

TclParseError tcl_parse_one_word(const char* src, size_t len, size_t* io_pos, XArena* arena, TclWord** out_word);

typedef struct TclVar
{
  XSlice name;
  XSlice value;
  struct TclVar* next;
  u32 hash;
} TclVar;

typedef struct TclCmdDef TclCmdDef;

typedef struct TclEnv
{
  TclVar* head;
  XArena* arena;          // Persistent arena
  TclCmdDef* commands;    // list of registered commands
} TclEnv;

typedef XSlice (*TclCmdFn)(TclCommand* cmd, TclEnv* env);
struct TclCmdDef
{
  const char*  name;   // command name
  TclCmdFn     fn;     // command function
  struct TclCmdDef* next;
};

TclVar*   tcl_env_find(TclEnv* E, XSlice name);
XSlice    tcl_env_get(TclEnv* E, XSlice name);
XSlice    tcl_env_get_cstr(TclEnv* E, const char* cname);
XSlice    tcl_env_set(TclEnv* E, XSlice name, XSlice value);

void      tcl_command_register(TclEnv* E, const char* name, TclCmdFn fn);
TclCmdDef*  tcl_command_find(TclEnv* E, XSlice name);
XSlice    tcl_eval_script_from_text(XSlice text, TclEnv* E, XArena* A);
XSlice    tcl_eval_script(TclScript* S, TclEnv* E, XArena* A);
XSlice    tcl_eval_command(TclCommand* C, TclEnv* E, XArena* A);
XSlice    tcl_eval_word(TclWord* W, TclEnv* E, XArena* A);
XSlice    tcl_eval_words_join(TclCommand* C, size_t from_idx, TclEnv* E, XArena* A);


#endif // X_TCL_H
