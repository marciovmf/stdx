#ifndef MI_BUILTINS_H
#define MI_BUILTINS_H

#include <stdx_common.h>
#include "mi_runtime.h"


MiExecResult mi_cmd_break(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_continue(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_expr(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_foreach(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_format(MiContext *ctx, int argc, MiNode **argv);

MiExecResult mi_cmd_if(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_print(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_set(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_while(MiContext *ctx, i32 argc, MiNode **argv);

//
// List
//

MiValue mi_value_list(i32 capacity);

bool mi_list_push_value(MiValue list_value, MiValue value);

MiExecResult mi_list_cmd_create(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_len(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_get(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_set(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_push(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_pop(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_clear(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_destroy(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_append(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_list_cmd_index_of(MiContext *ctx, i32 argc, MiNode **argv);

MiExecResult mi_cmd_list(MiContext *ctx, i32 argc, MiNode **argv);

bool mi_register_builtins(MiContext *ctx);

#endif  // MI_BUILTINS_H
