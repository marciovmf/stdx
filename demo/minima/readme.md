# Minima

Minima is a tiny command-oriented scripting language used in this repository as a **demo interpreter/runtime**. It is implemented in C in `demo/minima/src` and ships with a parser, runtime, and a small standard command set (variables, conditionals, loops, lists, string ops, `include`, and `eval`).


## How the language works

Minima programs are a sequence of **commands**. The whole language syntax is can
be sumarized in a few bullet points:

- A programm is a sequence of commands.
- Commands are separated by newline or `;`.
- The first token in a command is the command name and follows C variable naming
conention.
- The rest are arguments.
- `#` starts a comment that extends until end of line.
- Blocks of code are delimited by `{ ... }`.
- Subcommands are delimited by `( ... )` and must contain exactly **one** command.

This is all the language syntax.
In Minima, everything is a command. There are no reserved keywords or special constructs—no if, else, or while.
Control flow, loops, data structures, and expressions are all implemented as ordinary commands. Even features that look like language constructs are just commands provided by the standard library.

### Values and argument forms

Arguments can be:

- **Raw tokens**: `hello`, `123`, `my_name`
- **Strings**: `"hello world"` (stored without quotes; escapes are interpreted by `print`/`format`)
- **Variables**: `$name`
- **Subcommands**: `(expr 1 + 2)`
- **Blocks**: `{ ... }`

Runtime value kinds:

- `null`
- string
- raw
- number
- user (host-defined type)

### Evaluation model (important)

Minima is command-centric. A command receives the AST nodes of its arguments and may evaluate each one explicitly.

- `set` evaluates the value expression but keeps the variable name raw.
- `if` evaluates condition nodes and executes block nodes.
- `list` first resolves the subcommand name, then dispatches to list operations.


This design is why custom commands are straightforward to implement in C.

## Programming in Minima

### Variables and output

```minima
set name "Minima"
set n (expr 40 + 2)
print (format "name=${name}, n=${n}\n")
```

### Expressions (`expr`)

`expr` evaluates numeric/boolean expressions and returns a number.

Supported operators:

- unary: `-`, `not`
- multiplicative: `*`, `/`, `%`
- additive: `+`, `-`
- relational: `>`, `>=`, `<`, `<=`
- equality: `==`, `!=`
- logical: `and`, `or`
- grouping: `[ ... ]`

Example:

```minima
set ok (expr [10 + 2 * 3] >= 16 and not 0)
print (format "ok=${ok}\n")
```

### Conditionals

```minima
if (expr $n > 10) {
  print "large\n"
} elseif (expr $n == 10) {
  print "ten\n"
} else {
  print "small\n"
}
```

### Loops

```minima
set i 0
while (expr $i < 5) {
  print (format "i=${i}\n")
  set i (expr $i + 1)
}
```

`break` and `continue` are valid inside loops.

### Lists

```minima
set xs (list create a b c)
print (format "len=$(list len $xs)\n")
print (format "first=$(list get $xs 0)\n")

list set $xs 1 z
list push $xs d
set last (list pop $xs)

set ys (list create x y)
list append $xs $ys

foreach item $xs {
  print (format "item=${item}\n")
}
```

List subcommands:

- `list create [items...]`
- `list len LIST`
- `list get LIST INDEX`
- `list set LIST INDEX VALUE`
- `list push LIST VALUE`
- `list pop LIST`
- `list clear LIST`
- `list destroy LIST`
- `list append LIST OTHER`
- `list index_of LIST VALUE`

### Strings (`str`)

```minima
print (format "cmp=$(str compare a b)\n")
print (format "eq=$(str equals hello hello)\n")
print (format "len=$(str length \"hello\")\n")
```

String subcommands:

- `str compare LEFT RIGHT`
- `str compare_i LEFT RIGHT`
- `str equals LEFT RIGHT`
- `str equals_i LEFT RIGHT`
- `str length VALUE`

### `format`, `eval`, and `include`

`format` interpolates a string and returns the formatted string.

- `${name}` -> variable interpolation
- `$( ... )` -> parse and execute nested Minima source, then insert the resulting value
- `$$` -> literal `$`

```minima
set who "world"
print (format "hello ${who}\n")
print (format "2+3=$(expr 2 + 3)\n")
```

`eval` parses and executes source from a **string value**.

```minima
eval "print \"from eval\\n\""
```

`include` reads a file path from a **string value**, then parses/executes its contents.

```minima
include "demo/minima/test.txt"
```

## Writing custom commands (C API)

You can embed Minima and register host commands via `mi_register_command`.

### Command signature

```c
typedef MiExecResult (*MiCommandFn)(MiContext *ctx, i32 argc, MiNode **argv);
```

- `argv` contains argument AST nodes (not auto-evaluated).
- Use `mi_eval_node(ctx, argv[i])` for normal value evaluation.
- Return `mi_exec_ok(...)`, `mi_exec_null()`, or `mi_exec_error()`.
- Report errors with `mi_context_set_error(ctx, "message", line, column)`.

### Minimal example

```c
#include "minima.h"

static MiExecResult cmd_twice(MiContext *ctx, i32 argc, MiNode **argv)
{
  MiExecResult value;

  if (argc != 1)
  {
    mi_context_set_error(ctx, "twice expects 1 argument", 0, 0);
    return mi_exec_error();
  }

  value = mi_eval_node(ctx, argv[0]);
  if (value.signal != MI_SIGNAL_NONE)
  {
    return value;
  }

  /* This command simply returns the same value.
     You can transform it however you want. */
  return mi_exec_ok(value.value);
}

void register_my_commands(MiContext *ctx)
{
  mi_register_command(ctx, "twice", cmd_twice);
}
```

Script usage:

```minima
print (format "value=$(twice hello)\n")
```

### Embedding Minima in a host application

Below is the practical flow used by `demo/minima/src/main.c`, adapted for embedding.

#### 1) Initialize runtime state

- Create an arena (`x_arena_create`) used by parser/runtime allocations.
- Initialize `MiContext` with `mi_context_init`.
- Register builtin commands with `mi_register_builtins`.
- Register your custom commands with `mi_register_command`.

```c
XArena *arena = x_arena_create(8 * 1024);
MiContext ctx;

if (!arena) {
  /* host error handling */
}

if (!mi_context_init(&ctx, arena, stdout, stderr)) {
  /* host error handling */
}

if (!mi_register_builtins(&ctx)) {
  /* host error handling */
}

mi_register_command(&ctx, "twice", cmd_twice);
```

#### 2) Parse source

```c
MiParseResult p = mi_parse(arena, source);
if (!p.ok) {
  fprintf(stderr, "%d:%d: %s\n", p.error_line, p.error_column,
          p.error_message ? p.error_message : "parse error");
  /* abort this execution */
}
```

#### 3) Execute parsed program

```c
MiExecResult r = mi_exec_block(&ctx, p.root, false);
if (r.signal == MI_SIGNAL_ERROR) {
  fprintf(stderr, "%d:%d: %s\n", ctx.error_line, ctx.error_column,
          ctx.error_message ? ctx.error_message : "runtime error");
}
```

#### 4) Re-run safely in the same process

For multiple scripts in one process:

- keep the same `MiContext` and command registrations,
- call `mi_context_reset(&ctx)` before each new script run,
- parse new source and execute again.

This clears transient error/output/source-stack state while preserving registered commands and global structures.

#### 5) Optional: source tracking for include-like behavior

Minima exposes source stack helpers:

- `mi_context_push_source(ctx, file_name)`
- `mi_context_pop_source(ctx)`
- `mi_context_current_source(ctx)`

Builtin `include` uses these so runtime errors can point to the currently executing source.

#### 6) Optional: evaluating generated code

When your host generates Minima code dynamically, use:

- `mi_call_cmd_eval(&ctx, source_slice)` to parse+execute a source slice directly, or
- language-level `eval "..."` from inside scripts.

#### 7) Shutdown

Destroy the arena when the embedded interpreter is no longer needed:

```c
x_arena_destroy(arena);
```

Because Minima allocations are arena-backed, this is the main lifetime boundary.

`demo/minima/src/main.c` remains the canonical minimal reference for CLI-style embedding.
