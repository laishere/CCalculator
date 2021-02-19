#include <stdio.h>
#include "test.h"
#include "string.h"

void printTestName(char *s)
{
    int len = strlen(s);
    int a = MAX_TEST_NAME_LENGTH - len;
    printf("%s", s);
    for (int i = 0; i < a; i++)
        putchar(' ');
}

void stack_test();
void queue_test();
void hash_map_test();
void hash_set_test();
void lexer_test();
void grammar_test();
void parser_test();
void evaluator_test();

int main()
{
    stack_test();
    hash_map_test();
    hash_set_test();
    queue_test();
    lexer_test();
    grammar_test();
    parser_test();
    evaluator_test();
    return 0;
}
