#include "stack.h"
#include <stdlib.h>

void stack_init(Stack *stack, int initCapacity, int increment)
{
    if (!stack) return;
    stack->top = stack->base = malloc(initCapacity * sizeof(void *));
    stack->increment = increment;
    stack->capacity = initCapacity;
}

void stack_clear(Stack *stack)
{
    if (!stack) return;
    free(stack->base);
    stack->top = stack->base = NULL;
    stack->capacity = 0;
}

int stack_size(Stack *stack)
{
    if (!stack) return 0;
    return stack->top - stack->base;
}

void stack_push(Stack *stack, void *ptr)
{
    if (!stack || !stack->top) return;
    int size = stack->top - stack->base;
    if (size == stack->capacity)
    {
        // ÐèÒªÀ©ÈÝ
        stack->capacity += stack->increment;
        stack->base = realloc(stack->base, stack->capacity * sizeof(void *));
        stack->top = stack->base + size; 
    }
    *(stack->top++) = ptr;
}

void *stack_pop(Stack *stack)
{
    if (stack_size(stack) == 0) return NULL;
    return *(--stack->top);
}