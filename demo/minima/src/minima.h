/**
 * Minima
 */

#ifndef MINIMA_H
#define MINIMA_H

#include <stdx_common.h>
#include <stdx_arena.h>
#include <stdx_array.h>
#include <stdx_hashtable.h>
#include <stdx_strbuilder.h>
#include <stdx_string.h>
#include <stdx_filesystem.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Kinds of syntax nodes produced by the Minima parser.
 */
typedef enum MiNodeKind
{
  
  MI_NODE_COMMAND,    // A command node containing child argument nodes.
  MI_NODE_STRING,     // A quoted string literal.
  MI_NODE_RAW,        // An unquoted raw token.
  MI_NODE_VARIABLE,   // A variable read such as `$name`.
  MI_NODE_SUBCOMMAND, // A subcommand expression such as `(expr 1 + 2)`. 
  MI_NODE_BLOCK,      // A block node delimited by `{` and `}`.
} MiNodeKind;

typedef struct MiNode MiNode;

/**
 * A parsed syntax node.
 *
 * Nodes form a linked tree. Child nodes are linked through
 * `first_child`/`last_child` and siblings through `next_sibling`.
 */
struct MiNode
{
  
  MiNodeKind kind;      // Node kind.
  XSlice text;          // Source text associated with the node.
  int line;             // 1-based source line where the node begins.
  int column;           // 1-based source column where the node begins.
  MiNode *first_child;  // First child node, or NULL when there are no children.
  MiNode *last_child;   // Last child node, or NULL when there are no children.
  MiNode *next_sibling; // Next sibling node in the parent child list.
};

/**
 * Result returned by the Minima parser.
 */
typedef struct MiParseResult
{
  int ok; // Non-zero on success, zero on parse failure. */
  MiNode *root; // Root node of the parsed program when parsing succeeds. */
  const char *error_message; // Human-readable parse error message, or NULL on success. */
  int error_line; // 1-based line where the parse error happened. */
  int error_column; // 1-based column where the parse error happened. */
} MiParseResult;

/**
 * Parse Minima source code into an AST.
 *
 * All nodes are allocated from the supplied arena.
 *
 * @param arena Arena used to allocate parser data and AST nodes.
 * @param source Source code to parse.
 * @return Parse result containing either the AST root or error information.
 */
MiParseResult mi_parse(XArena *arena, XSlice source);

/**
 * Print a parsed AST for debugging purposes.
 *
 * @param node Root node to print.
 * @param indent Initial indentation level.
 * @return Nothing.
 */
void mi_debug_print_ast(MiNode *node, int indent);

typedef struct MiUser MiUser;
typedef struct MiScope MiScope;
typedef struct MiContext MiContext;
typedef struct MiFunc MiFunc;

/**
 * Define a public Minima user type object named `NAME##_type`.
 * @param NAME The type name
 */
#define MI_DEFINE_TYPE(NAME) \
  const MiUserType NAME##_type = { #NAME }

/**
 * Import a public Minima user type and generate a typed value helper.
 *
 * The macro declares `extern const MiUserType NAME##_type;` and creates an
 * inline constructor named `mi_value_NAME(void *ptr)`.
 */
#define MI_IMPORT_TYPE(NAME)                       \
  extern const MiUserType NAME##_type;             \
                                                   \
  static inline MiValue mi_value_##NAME(void *ptr) \
  {                                                \
    return mi_value_user(&NAME##_type, ptr);       \
  }

/**
 * Kinds of runtime values supported by Minima.
 */
typedef enum MiValueKind
{
  MI_VAL_NULL   = 1, // Null value.
  MI_VAL_STRING = 2, // Quoted string value.
  MI_VAL_RAW    = 3, // Raw token value.
  MI_VAL_NUMBER = 4, // Numeric value stored as double.
  MI_VAL_USER   = 5, // Host-defined typed user value.
} MiValueKind;

/**
 * Runtime type descriptor for host-defined user values.
 */
typedef struct MiUserType
{
  const char* name;
} MiUserType;

/**
 * Wrapper for host-defined values carried by Minima.
 */
typedef struct MiUser
{
  const MiUserType *type; // Runtime type descriptor for the stored payload.
  void *data;             // Host-owned payload pointer.
} MiUser;

/**
 * A dynamically typed Minima runtime value.
 */
typedef struct MiValue
{
  
  MiValueKind kind; // Active kind stored in the union.
  union
  {
    XSlice string;  // String payload when `kind == MI_VAL_STRING`.
    XSlice raw;     // Raw token payload when `kind == MI_VAL_RAW`.
    double number;  // Number payload when `kind == MI_VAL_NUMBER`.
    MiUser user;    // User payload when `kind == MI_VAL_USER`.
  } as;
} MiValue;

/**
 * Execution control signals returned by commands and evaluators.
 */
typedef enum MiExecSignal
{
  MI_SIGNAL_NONE      = 0, // Normal completion.
  MI_SIGNAL_RETURN    = 1, // Function-style return.
  MI_SIGNAL_BREAK     = 2, // Break out of the current loop.
  MI_SIGNAL_CONTINUE  = 3, // Continue the current loop.
  MI_SIGNAL_ERROR     = 4, // Execution failed with an error.
} MiExecSignal;

/**
 * Result of evaluating Minima code.
 */
typedef struct MiExecResult
{
  MiExecSignal signal;  // Control signal produced by the evaluation.
  MiValue value;        // Value carried by the evaluation result.
} MiExecResult;

/**
 * Signature of a host command callable from Minima.
 *
 * Commands receive raw AST argument nodes and decide how and when to evaluate
 * them.
 *
 * @param ctx Active execution context.
 * @param argc Number of argument nodes.
 * @param argv Array of argument nodes.
 * @return Command execution result.
 */
typedef MiExecResult (*MiCommandFn)(MiContext *ctx, i32 argc, MiNode **argv);

/**
 * A user-defined Minima function.
 */
typedef struct MiFunc
{
  XSlice name;        // Function name.
  size_t param_count; // Number of parameters.
  XSlice *params;     // Parameter name array.
  MiNode *body;       // Function body block.
} MiFunc;

X_HASHTABLE_TYPE_CSTR_KEY_NAMED(MiValue, mi_value_table);       // Typed Hashtable for storing variables by name.
X_HASHTABLE_TYPE_CSTR_KEY_NAMED(MiCommandFn, mi_command_table); // Typed Hashtable  or storing commands by name.
X_HASHTABLE_TYPE_CSTR_KEY_NAMED(MiFunc, mi_func_table);         // Typed Hashtable  or storing functions by name.

/**
 * A Minima variable scope.
 */
struct MiScope
{
  XArena *arena;                    // Arena used for scope-owned allocations.
  MiScope *parent;                  // Parent scope, or NULL for the root scope.
  XHashtable_mi_value_table *vars;  // Variable table for the scope.
};

/**
 *
 */
typedef struct MiSourceFrame
{
  XFSPath source_name;
} MiSourceFrame;


/**
 * A Minima execution context.
 */
struct MiContext
{
  XArena *arena;                          // Arena used for runtime allocations.
  MiScope *global_scope;                  // Global root scope.
  MiScope *current_scope;                 // Currently active scope.
  XArray *source_stack;                   // Stack of MiSourceFrame
  XHashtable_mi_command_table *commands;  // Registered command table.
  XHashtable_mi_func_table *funcs;        // Registered function table.
  FILE *out;                              // Standard output stream used by builtins. Defaults to stdout.
  FILE *err;                              // Standard error stream used by builtins. Defaults to stderr.
  XStrBuilder *output;                    // Optional output buffer used by hosts that capture output.
  const char *error_message;              // Last error message set on the context.
  int error_line;                         // Line associated with the last error.
  int error_column;                       // Column associated with the last error.
  XFSPath error_source_file;
};

/**
 * Construct a null runtime value.
 *
 * @return A `MiValue` with kind `MI_VAL_NULL`.
 */
MiValue mi_value_null(void);

/**
 * Construct a string runtime value.
 *
 * @param s String slice to store.
 * @return A `MiValue` with kind `MI_VAL_STRING`.
 */
MiValue mi_value_string(XSlice s);

/**
 * Construct a raw runtime value.
 *
 * @param s Raw slice to store.
 * @return A `MiValue` with kind `MI_VAL_RAW`.
 */
MiValue mi_value_raw(XSlice s);

/**
 * Construct a numeric runtime value.
 *
 * @param d Numeric value to store.
 * @return A `MiValue` with kind `MI_VAL_NUMBER`.
 */
MiValue mi_value_number(double d);

/**
 * Construct a typed user runtime value.
 *
 * @param type Runtime type descriptor for the payload.
 * @param payload Host payload pointer.
 * @return A `MiValue` with kind `MI_VAL_USER`.
 */
MiValue mi_value_user(const MiUserType *type, void* payload);

/**
 * Construct a successful execution result carrying a value.
 *
 * @param value Result value.
 * @return A normal execution result.
 */
MiExecResult mi_exec_ok(MiValue value);

/**
 * Construct a successful execution result carrying null.
 *
 * @return A normal execution result with a null value.
 */
MiExecResult mi_exec_null(void);

/**
 * Construct an error execution result.
 *
 * @return An execution result with signal `MI_SIGNAL_ERROR`.
 */
MiExecResult mi_exec_error(void);

/**
 * Construct a return execution result.
 *
 * @param value Return value.
 * @return An execution result with signal `MI_SIGNAL_RETURN`.
 */
MiExecResult mi_exec_return(MiValue value);

/**
 * Construct a break execution result.
 *
 * @return An execution result with signal `MI_SIGNAL_BREAK`.
 */
MiExecResult mi_exec_break(void);

/**
 * Construct a continue execution result.
 *
 * @return An execution result with signal `MI_SIGNAL_CONTINUE`.
 */
MiExecResult mi_exec_continue(void);

/**
 * Test whether a value is null.
 *
 * @param value Value to test.
 * @return True when `value.kind == MI_VAL_NULL`, otherwise false.
 */
bool mi_value_is_null(MiValue value);

/**
 * Test whether a value is a string.
 *
 * @param value Value to test.
 * @return True when `value.kind == MI_VAL_STRING`, otherwise false.
 */
bool mi_value_is_string(MiValue value);

/**
 * Test whether a value is raw.
 *
 * @param value Value to test.
 * @return True when `value.kind == MI_VAL_RAW`, otherwise false.
 */
bool mi_value_is_raw(MiValue value);

/**
 * Test whether a value is a user value.
 *
 * @param value Value to test.
 * @return True when `value.kind == MI_VAL_USER`, otherwise false.
 */
bool mi_value_is_user(MiValue value);

/**
 * Create a new scope.
 *
 * @param arena Arena used to allocate scope data.
 * @param parent Optional parent scope.
 * @return Newly created scope, or NULL on failure.
 */
MiScope *mi_scope_create(XArena *arena, MiScope *parent);

/**
 * Look up a variable in a scope chain.
 *
 * @param scope Scope where the lookup begins.
 * @param name Variable name.
 * @param out_value Output location for the value.
 * @return True on success, false when the variable is not found.
 */
bool mi_scope_get(MiScope *scope, XSlice name, MiValue *out_value);

/**
 * Define a variable in the current scope only.
 *
 * @param scope Target scope.
 * @param name Variable name.
 * @param value Value to store.
 * @return True on success, false on failure.
 */
bool mi_scope_define(MiScope *scope, XSlice name, MiValue value);

/**
 * Set a variable in the nearest scope where it exists.
 *
 * If the variable does not exist in the scope chain, it is created in the
 * current scope.
 *
 * @param scope Scope where the search begins.
 * @param name Variable name.
 * @param value Value to store.
 * @return True on success, false on failure.
 */
bool mi_scope_set(MiScope *scope, XSlice name, MiValue value);

/**
 * Initialize a Minima execution context.
 *
 * @param ctx Context to initialize.
 * @param arena Arena used for runtime allocations.
 * @param out_stream Output stream, or NULL to use stdout.
 * @param err_stream Error stream, or NULL to use stderr.
 * @return True on success, false on failure.
 */
bool mi_context_init(MiContext *ctx, XArena *arena, FILE* out_stream, FILE* err_stream);

/**
 * Reset a context to its initial runtime state.
 *
 * @param ctx Context to reset.
 * @return Nothing.
 */
void mi_context_reset(MiContext *ctx);

/**
 * Store error information in the context.
 *
 * @param ctx Target context.
 * @param message Error message.
 * @param line Source line associated with the error.
 * @param column Source column associated with the error.
 * @return Nothing.
 */
void mi_context_set_error(MiContext *ctx, const char *message, int line, int column);

/**
 * Register a host command using a null-terminated C string name.
 *
 * @param ctx Target context.
 * @param name Command name.
 * @param fn Command callback.
 * @return True on success, false on failure.
 */
bool mi_register_command(MiContext *ctx, const char *name, MiCommandFn fn);

/**
 * Register a host command using a slice name.
 *
 * @param ctx Target context.
 * @param name Command name.
 * @param fn Command callback.
 * @return True on success, false on failure.
 */
bool mi_register_command_slice(MiContext *ctx, XSlice name, MiCommandFn fn);

/**
 * Look up a registered command by name.
 *
 * @param ctx Source context.
 * @param name Command name.
 * @param out_fn Output location for the callback.
 * @return True when the command is found, otherwise false.
 */
bool mi_lookup_command(MiContext *ctx, XSlice name, MiCommandFn *out_fn);

/**
 * Register a user-defined Minima function.
 *
 * @param ctx Target context.
 * @param name Function name.
 * @param func Function definition.
 * @return True on success, false on failure.
 */
bool mi_register_func_slice(MiContext *ctx, XSlice name, MiFunc func);

/**
 * Look up a registered Minima function.
 *
 * @param ctx Source context.
 * @param name Function name.
 * @param out_func Output location for the function definition.
 * @return True when the function is found, otherwise false.
 */
bool mi_lookup_func(MiContext *ctx, XSlice name, MiFunc *out_func);

/**
 * Push a new child scope and make it current.
 *
 * @param ctx Target context.
 * @return The new current scope, or NULL on failure.
 */
MiScope *mi_push_scope(MiContext *ctx);

/**
 * Pop the current scope and restore its parent.
 *
 * @param ctx Target context.
 * @return Nothing.
 */
void mi_pop_scope(MiContext *ctx);

/**
 * Evaluate a single AST node.
 *
 * @param ctx Active execution context.
 * @param node Node to evaluate.
 * @return Evaluation result.
 */
MiExecResult mi_eval_node(MiContext *ctx, MiNode *node);

/**
 * Execute a command node.
 *
 * @param ctx Active execution context.
 * @param command Command node to execute.
 * @return Execution result.
 */
MiExecResult mi_exec_command(MiContext *ctx, MiNode *command);

/**
 * Execute all commands in a block node.
 *
 * @param ctx Active execution context.
 * @param block Block node to execute.
 * @param create_scope When true, execute the block in a fresh child scope.
 * @return Execution result.
 */
MiExecResult mi_exec_block(MiContext *ctx, MiNode *block, bool create_scope);

/**
 * Call a registered Minima function.
 *
 * @param ctx Active execution context.
 * @param func Function to call.
 * @param argc Number of argument nodes.
 * @param argv Argument node array.
 * @return Execution result.
 */
MiExecResult mi_call_func(MiContext *ctx, const MiFunc *func, int argc, MiNode **argv);


X_ARRAY_TYPE(MiValue);
MI_IMPORT_TYPE(List);

typedef struct MiList
{
  uint32_t magic;
  bool destroyed;
  XArray_MiValue *items;
} MiList;


/**
 * Push a value into a builtin list object.
 *
 * @param list_value List runtime value.
 * @param value Value to append.
 * @return Execution result describing success or failure.
 */
MiExecResult mi_call_cmd_list_push_value(MiValue list_value, MiValue value);

/**
 * Parse and execute Minima source code from a string slice.
 *
 * @param ctx Active execution context.
 * @param source Source code to parse and execute.
 * @return Execution result.
 */
MiExecResult  mi_call_cmd_eval(MiContext *ctx, XSlice source);

/**
 * Construct a builtin list runtime value.
 *
 * @param capacity Initial list capacity hint.
 * @return A list user value, or null on allocation failure.
 */
MiValue mi_value_list(i32 capacity);

/**
 * Push a value into a builtin list object.
 *
 * @param list_value List runtime value.
 * @param value Value to append.
 * @return True on success, false on failure.
 */
bool mi_list_push_value(MiValue list_value, MiValue value);

/**
 * Register the standard builtin commands in a context.
 *
 * @param ctx Target context.
 * @return True on success, false on failure.
 */
bool mi_register_builtins(MiContext *ctx);


bool mi_context_push_source(MiContext *ctx, XSlice file_name);

void mi_context_pop_source(MiContext *ctx);

XSlice mi_context_current_source(MiContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MINIMA_H */
