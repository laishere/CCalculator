#include "test.h"
#include "parser.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

void print_node(ParseNode *node)
{
    
    printf("%s", parser_get_symbol_name(node->symbol));
}

void dump_tree(ParseTree t, int level)
{
    if (!t) return;
    for (int i = 1; i < level; i++)
        printf("|  ");
    if (level > 0)
        printf("|--");
    print_node(t);
    printf("\n");
    if (t->children)
    {
        stack_for_each(t->children)
            dump_tree(*p, level + 1);
    }
}

void parser_test()
{
    char *expr = "f = g = -x^2^3 2 ^ ^ 1 2 - * ((-(5+1)) / 2) / (-3 * (-(-4))) + (diff(x, x, 1) - (-(-1))) - (sin(x) + sin(y))";
    ParseTree t = parser_parse(expr);
    t = parser_ast(t);
    // dump_tree(t, 0);
    char res[500];
    *tree2expr(t, res) = '\0';
    char *expected = "f = g = -x ^ 2 ^ 3 ^ 2 ^ (1 - 2) * (-(5 + 1)) / 2 / (-3 * 4) + diff(x, x, 1) - 1 - (sin(x) + sin(y))";
    assert(strcmp(res, expected) == 0);
}