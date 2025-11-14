#ifndef TCL_EXPR_H
#define TCL_EXPR_H

#include <stdx_string.h>
#include "tcl.h"

XSlice tcl_cmd_while(TclCommand* C, TclEnv* E);
XSlice tcl_cmd_set(TclCommand* C, TclEnv* E);
XSlice tcl_cmd_puts(TclCommand* C, TclEnv* E);
XSlice tcl_cmd_incr(TclCommand* C, TclEnv* E);
XSlice tcl_cmd_if(TclCommand* C, TclEnv* E);
XSlice tcl_cmd_expr(TclCommand* C, TclEnv* E);

#endif  // TCL_EXPR_H
