#include "test.h"
#include "stack.h"
#include <stdio.h>
#include <assert.h>

void stack_test()
{
    Stack stack;
    TEST_START("栈的初始化测试");
    stack_init(&stack, 1, 2);
    assert(stack.base != NULL && stack.base == stack.top);
    assert(stack_size(&stack) == 0);
    assert(stack.capacity == 1);
    TEST_END();
    
    TEST_START("栈的入栈测试");
    stack_push(&stack, NULL);
    assert(stack_size(&stack) == 1);
    assert(stack.capacity == 1);
    stack_push(&stack, NULL);
    assert(stack_size(&stack) == 2);
    assert(stack.capacity == 3);
    TEST_END();

    TEST_START("栈的出栈测试");    
    int a = 123;
    stack_push(&stack, &a);
    assert(stack_size(&stack) == 3);
    assert(*(int *)stack_pop(&stack) == a);
    assert(stack_size(&stack) == 2);
    TEST_END();

    TEST_START("栈的清理测试");
    stack_clear(&stack);
    assert(stack_size(&stack) == 0);
    assert(stack.capacity == 0);
    TEST_END();
}