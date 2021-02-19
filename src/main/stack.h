#if !defined(STACK_H)
#define STACK_H

typedef struct Stack
{
    void **base, **top;
    int capacity, increment;
} Stack;

void stack_init(Stack *stack, int initCapacity, int increment);
void stack_clear(Stack *stack);
int stack_size(Stack *stack);
void stack_push(Stack *stack, void *ptr);
void *stack_pop(Stack *stack);

#define stack_for_each(s) for (void **p = (s)->base; p < (s)->top; p++)

#endif // STACK_H
