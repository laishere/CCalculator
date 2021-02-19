#include "test.h"
#include "stack.h"
#include <stdio.h>
#include <assert.h>

void stack_test()
{
    Stack stack;
    TEST_START("ջ�ĳ�ʼ������");
    stack_init(&stack, 1, 2);
    assert(stack.base != NULL && stack.base == stack.top);
    assert(stack_size(&stack) == 0);
    assert(stack.capacity == 1);
    TEST_END();
    
    TEST_START("ջ����ջ����");
    stack_push(&stack, NULL);
    assert(stack_size(&stack) == 1);
    assert(stack.capacity == 1);
    stack_push(&stack, NULL);
    assert(stack_size(&stack) == 2);
    assert(stack.capacity == 3);
    TEST_END();

    TEST_START("ջ�ĳ�ջ����");    
    int a = 123;
    stack_push(&stack, &a);
    assert(stack_size(&stack) == 3);
    assert(*(int *)stack_pop(&stack) == a);
    assert(stack_size(&stack) == 2);
    TEST_END();

    TEST_START("ջ���������");
    stack_clear(&stack);
    assert(stack_size(&stack) == 0);
    assert(stack.capacity == 0);
    TEST_END();
}