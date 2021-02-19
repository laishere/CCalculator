#include <assert.h>
#include <string.h>
#include "lexer.h"
#include "test.h"

typedef struct
{
    char *str;
    ParserSymbol symbol;
} TestCase;

void lexer_test_get_tokens()
{
    TestCase cases[] = {
        {" \t \n  ", TOKEN_WHITESPACE},
        {"(", TOKEN_LEFT_BRACKET},
        {")", TOKEN_RIGHT_BRACKET},
        {"1", TOKEN_NUMBER},
        {".1", TOKEN_NUMBER},
        {"1.", TOKEN_NUMBER},
        {".1e+3", TOKEN_NUMBER},
        {".1e-3", TOKEN_NUMBER},
        {"_a1_b2_c3__", TOKEN_IDENTIFIER},
        {"+", TOKEN_OPERATOR_PLUS},
        {"-", TOKEN_OPERATOR_MINUS},
        {"*", TOKEN_OPERATOR_MULTIPLY},
        {"/", TOKEN_OPERATOR_DIVIDE},
        {"^", TOKEN_OPERATOR_POW},
        {"=", TOKEN_ASSIGN},
        {",", TOKEN_COMMA},
        {".", TOKEN_UNKNOWN_CHAR},
        {"?", TOKEN_UNDEFINED},
        {"", TOKEN_EOF}
    };
    int size = sizeof(cases) / sizeof(TestCase);
    for (int i = 0; i < size; i++)
    {
        char *s = cases[i].str;
        char *e = s + strlen(s);
        Token t = lexer_next_token(s);
        assert(t.start == s);
        assert(t.end == e);
        assert(t.symbol == cases[i].symbol);
    }
}

void lexer_test()
{
    TEST_START("·Ö´Ê²âÊÔ");
    lexer_test_get_tokens();
    TEST_END();
}