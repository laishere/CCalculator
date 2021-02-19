#include "evaluator.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    char expr[1000];
    char buf[10000];
    printf("输入表达式开始吧!\n\n");
    while (1)
    {
        printf("> ");
        gets(expr);
        if (strcmp(expr, "exit") == 0)
        {
            printf("\nBye~\n");
            break;
        }
        ParseTree ans = evaluate(expr);
        if (!ans)
        {
            char *err = evaluator_error();
            if (*err)
                printf("[错误] %s\n\n", err);
            else
                printf("\n");
        }
        else
        {
            *tree2expr(ans, buf) = 0;
            printf("< %s\n\n", buf);
            parser_clear_tree(ans);
        }
    }
    return 0;
}

