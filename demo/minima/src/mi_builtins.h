#ifndef MI_BUILTINS_H
#define MI_BUILTINS_H

#include <stdx_common.h>
#include "mi_runtime.h"


//
// List
//

MiExecResult mi_call_cmd_list_push_value(MiValue list_value, MiValue value);
MiExecResult  mi_call_cmd_eval(MiContext *ctx, XSlice source);

MiValue mi_value_list(i32 capacity);

bool mi_list_push_value(MiValue list_value, MiValue value);

bool mi_register_builtins(MiContext *ctx);

#endif  // MI_BUILTINS_H
