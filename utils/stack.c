#include <stdlib.h>
#include "stack.h"

#define STACK_INIT_SIZE 8

stack_t *new_stack(void)
{
	stack_t *s = malloc(sizeof(stack_t));
	s->size = STACK_INIT_SIZE;
	s->data = malloc(s->size * sizeof(*s->data));
	s->top = -1;
	return s;
}

bool del_stack(stack_t *s)
{
	if (s->top >= 0) // stack is not empty, cannot delete
		return false;
	free(s->data);
	free(s);
	return true;
}

bool is_stack_empty(stack_t *s) { return s->top == -1; }

void stack_push(stack_t *s, void *data)
{
	if (s->top + 1 >= s->size)
		s->data = realloc(s->data, (s->size <<= 1) * sizeof(*s->data));
	s->data[++s->top] = data;
}

void *stack_pop(stack_t *s)
{
	if (is_stack_empty(s))
		return NULL;
	return s->data[s->top--];
}
