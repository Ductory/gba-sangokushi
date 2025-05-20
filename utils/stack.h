#ifndef _STACK_H
#define _STACK_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	void **data;
	size_t size;
	int top;
} stack_t;

stack_t *new_stack(void);
bool del_stack(stack_t *s);
bool is_stack_empty(stack_t *s);
void stack_push(stack_t *s, void *data);
void *stack_pop(stack_t *s);

#endif // _STACK_H
