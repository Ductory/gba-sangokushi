#ifndef _DEBUG_H
#define _DEBUG_H

#include "logger.h"
#include "stack.h"
#include <stdlib.h>

#define debug_print(...) ((void*)0)

#define ENABLE_DEBUG static stack_t *_dbg_stack;
#define INIT_DEBUG do { _dbg_stack = init_debug(); } while (0);
#define UNINIT_DEBUG uninit_debug(_dbg_stack);

stack_t *init_debug(void);
void uninit_debug(stack_t *s);
bool debug_is_uncaught(stack_t *s);
int debug_push_frame(stack_t *s);
bool debug_pop_frame(stack_t *s, int code);
void __attribute__((noreturn)) debug_throw(stack_t *s, int code);
int debug_catch(stack_t *s);

#define __try do { int _sjret = debug_push_frame(_dbg_stack); if (!_sjret)
#define __throw(code) debug_throw(_dbg_stack, code)
#define __catch(code) else if (_sjret == (code) && debug_catch(_dbg_stack))
#define __finally
#define __endtry if (!debug_pop_frame(_dbg_stack, _sjret)) { \
	LOG_E("in <%s>", __func__); exit(1); } } while (0);


#define DBG_ERROR 0
#define DBG_WARNING 1
#define DBG_DEBUG 2

#endif // _DEBUG_H
