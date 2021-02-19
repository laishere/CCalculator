#if !defined(PARSER_H)
#define PARSER_H

#include "stack.h"

typedef enum
{
    TOKEN_START,
    TOKEN_WHITESPACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR_PLUS,
    TOKEN_OPERATOR_MINUS,
    TOKEN_OPERATOR_MULTIPLY,
    TOKEN_OPERATOR_DIVIDE,
    TOKEN_OPERATOR_POW,
    TOKEN_ASSIGN,
    TOKEN_COMMA,
    TOKEN_UNDEFINED,
    TOKEN_UNKNOWN_CHAR,
    TOKEN_EOF,
    TOKEN_END,
    RULE_TARGET,
    RULE_EXPR,
    RULE_SIGNED_EXPR,
    RULE_ASSIGN_EXPR,
    RULE_ADD_EXPR,
    RULE_MUL_EXPR,
    RULE_POW_EXPR,
    RULE_VALUE,
    RULE_FUNC,
    RULE_ARG,
    RULE_NO_BRACKETS_VALUE,
    RULE_REVESE_EXPR,
    RULE_OPERATOR,
    RULE_FUNC_NAME
} ParserSymbol;

typedef enum
{
    FUNC_SIN,
    FUNC_COS,
    FUNC_TAN,
    FUNC_ASIN,
    FUNC_ACOS,
    FUNC_ATAN,
    FUNC_LN,
    FUNC_ABS,
    FUNC_DIFF,
    FUNC_SQRT
} SupportedFunction;

typedef struct Token
{
    ParserSymbol symbol;
    char *start, *end;
} Token;

typedef struct ParseNode
{
    ParserSymbol symbol;
    Stack *children;
    char *start, *end;
    void *extraInfo;
} ParseNode, *ParseTree;

char *parser_get_symbol_name(ParserSymbol symbol);
ParseTree parser_parse(char *s);
ParseTree parser_ast(ParseTree t);
char *tree2expr(ParseTree ast, char *buf);
void parser_clear_node(ParseNode *node);
void parser_clear_tree(ParseTree t);
void print_expr(ParseTree t);

#endif // PARSER_H
