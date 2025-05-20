/**
 * for tcc (aka. Tiny C Compiler), a better solution is `tccseh` in root directory.
 * but tcc uses MSVCRT and has incorrect support for vm types.
 * therefore, we won't use tcc.
 * we implement a exception handling model using sjlj.
 */
#include "logger.h"
#include "debug.h"
#include <setjmp.h>

stack_t *init_debug(void)
{
	stack_t *s = new_stack();
	stack_push(s, (void*)0);
	return s;
}

void uninit_debug(stack_t *s)
{
	stack_pop(s);
	if (!is_stack_empty(s)) {
		LOG_E("debug stack collapsed");
		exit(1);
	}
	del_stack(s);
}

bool debug_is_uncaught(stack_t *s)
{
	return s->data[0];
}

int debug_push_frame(stack_t *s)
{
	jmp_buf *buf = malloc(sizeof(jmp_buf));
	stack_push(s, buf);
	return setjmp(*buf);
}

bool debug_pop_frame(stack_t *s, int code)
{
	free(stack_pop(s));
	if (!debug_is_uncaught(s))
		return true;
	if (s->top == 0) {
		LOG_E("uncaught exception");
		return false;
	} else {
		longjmp(s->data[s->top], code);
	}
}

void __attribute__((noreturn)) debug_throw(stack_t *s, int code)
{
	s->data[0] = (void*)1;
	longjmp(s->data[s->top], code);
}

int debug_catch(stack_t *s)
{
	s->data[0] = (void*)0;
	return 1;
}
