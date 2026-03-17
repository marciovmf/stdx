#ifndef MI_RUNTIME_H
#define MI_RUNTIME_H

#include <stdx_arena.h>
#include <stdx_array.h>
#include <stdx_hashtable.h>
#include <stdx_strbuilder.h>
#include <stdx_string.h>


#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MiNode MiNode;
typedef struct MiUser MiUser;
typedef struct MiScope MiScope;
typedef struct MiContext MiContext;
typedef struct MiFunc MiFunc;

#define MI_DEFINE_TYPE(NAME) \
  const MiUserType NAME##_type = { #NAME }

#define MI_IMPORT_TYPE(NAME)                       \
  extern const MiUserType NAME##_type;             \
                                                   \
  static inline MiValue mi_value_##NAME(void *ptr) \
  {                                                \
    return mi_value_user(&NAME##_type, ptr);       \
  }



typedef enum MiValueKind
{
  MI_VAL_NULL   = 1,
  MI_VAL_STRING = 2,
  MI_VAL_RAW    = 3,
  MI_VAL_NUMBER = 4,
  MI_VAL_USER   = 5,
} MiValueKind;

typedef struct MiUserType
{
  const char* name;
} MiUserType;

typedef struct MiUser
{
  const MiUserType *type;
  void *data;
} MiUser;

typedef struct MiValue
{
  MiValueKind kind;
  union
  {
    XSlice string;
    XSlice raw;
    double number;
    MiUser user;
  } as;
} MiValue;


typedef enum MiExecSignal
{
  MI_SIGNAL_NONE      = 0,
  MI_SIGNAL_RETURN    = 1,
  MI_SIGNAL_BREAK     = 2,
  MI_SIGNAL_CONTINUE  = 3,
  MI_SIGNAL_ERROR     = 4,
} MiExecSignal;

typedef struct MiExecResult
{
  MiExecSignal signal;
  MiValue value;
} MiExecResult;

typedef MiExecResult (*MiCommandFn)(MiContext *ctx, i32 argc, MiNode **argv);

typedef struct MiFunc
{
  XSlice name;
  size_t param_count;
  XSlice *params;
  MiNode *body;
} MiFunc;

X_HASHTABLE_TYPE_CSTR_KEY_NAMED(MiValue, mi_value_table);
X_HASHTABLE_TYPE_CSTR_KEY_NAMED(MiCommandFn, mi_command_table);
X_HASHTABLE_TYPE_CSTR_KEY_NAMED(MiFunc, mi_func_table);

struct MiScope
{
  XArena *arena;
  MiScope *parent;
  XHashtable_mi_value_table *vars;
};

struct MiContext
{
  XArena *arena;

  MiScope *global_scope;
  MiScope *current_scope;

  XHashtable_mi_command_table *commands;
  XHashtable_mi_func_table *funcs;

  FILE *out;    // defaults to stdout
  FILE *err;    // defaults to stderr

  XStrBuilder *output; /* optional buffer */

  const char *error_message;
  int error_line;
  int error_column;
};

MiValue mi_value_null(void);
MiValue mi_value_string(XSlice s);
MiValue mi_value_raw(XSlice s);
MiValue mi_value_number(double d);
MiValue mi_value_user(const MiUserType *type, void* payload);

MiExecResult mi_exec_ok(MiValue value);
MiExecResult mi_exec_null(void);
MiExecResult mi_exec_error(void);
MiExecResult mi_exec_return(MiValue value);
MiExecResult mi_exec_break(void);
MiExecResult mi_exec_continue(void);

bool mi_value_is_null(MiValue value);
bool mi_value_is_string(MiValue value);
bool mi_value_is_raw(MiValue value);
bool mi_value_is_user(MiValue value);

MiScope *mi_scope_create(XArena *arena, MiScope *parent);
bool mi_scope_get(MiScope *scope, XSlice name, MiValue *out_value);
bool mi_scope_define(MiScope *scope, XSlice name, MiValue value);
bool mi_scope_set(MiScope *scope, XSlice name, MiValue value);

bool mi_context_init(MiContext *ctx, XArena *arena, FILE* out_stream, FILE* err_stream);
void mi_context_reset(MiContext *ctx);
void mi_context_set_error(MiContext *ctx, const char *message, int line, int column);

bool mi_register_command(MiContext *ctx, const char *name, MiCommandFn fn);
bool mi_register_command_slice(MiContext *ctx, XSlice name, MiCommandFn fn);
bool mi_lookup_command(MiContext *ctx, XSlice name, MiCommandFn *out_fn);

bool mi_register_func_slice(MiContext *ctx, XSlice name, MiFunc func);
bool mi_lookup_func(MiContext *ctx, XSlice name, MiFunc *out_func);


MiScope *mi_push_scope(MiContext *ctx);
void mi_pop_scope(MiContext *ctx);

MiExecResult mi_eval_node(MiContext *ctx, MiNode *node);
MiExecResult mi_exec_command(MiContext *ctx, MiNode *command);
MiExecResult mi_exec_block(MiContext *ctx, MiNode *block, bool create_scope);
MiExecResult mi_call_func(MiContext *ctx, const MiFunc *func, int argc, MiNode **argv);


#ifdef __cplusplus
}
#endif

#endif /* MI_RUNTIME_H */
