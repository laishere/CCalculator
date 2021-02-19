#include "test.h"
#include "evaluator.h"

void dump_tree(ParseTree t, int level);

void evaluator_test()
{
    char *exprs[] = {
        "-1 + 3 * x - (6*x - 2^2^3 1 - + 3) - 1",
         "1604.4 6 6 6 20 21 /^-^/",
         "(-5) - (x)",
         "(12*x + ln(x)) * 1",
         "1 + (-2) * x + (-2)^x",
         "diff(x^2, x, 0)",
         "diff(x^2, x, 2)",
         "diff(2.718281828^x, x, 1)",
         "diff(x^x, x, 1)",
         "diff(1 / x + sin(x) + cos(x) + tan(x) + asin(x) + acos(x) + atan(x) + ln(x) + x, x, 1)",
         "diff(x * y, y, 1)",
         "diff(x^2, x, x)",
         "diff(x^2, x, 0.1)",
         "diff(x^2, x + 3, 0)",
         "f = g = h = x^2",
         "f = g = h = ?"
    };
    char buf[10000];
    for (int j = 0; j < 1; j++)
    {
        for (int i = 0; i < sizeof(exprs) / sizeof(char*); i++)
        {
            char* expr = exprs[i];
            printf("%s ", expr);
            ParseTree ans = evaluate(expr);
            // dump_tree(ans, 0);
            *tree2expr(ans, buf) = '\0';
            if (ans)
            {
                printf("= %s\n", buf);
                parser_clear_tree(ans);
            }
            else if (*evaluator_error())
            {
                printf("\n[´íÎó] %s\n", evaluator_error());
            }
            printf("\n");
        }
    }
}