#include "lexer.h"

#define CHAR_IN(c, a, b) (c >= a && c <= b)

void token_whitespace(char *s, Token *t)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    t->symbol = TOKEN_WHITESPACE;
    t->end = s;
}

/**
 * 单符号token
 */ 
void token_simple_symbol(char *s, Token *t)
{
    #define CASE(a, b) case a: s++; t->symbol = b; break
    switch (*s)
    {
    CASE('(', TOKEN_LEFT_BRACKET);
    CASE(')', TOKEN_RIGHT_BRACKET);
    CASE(',', TOKEN_COMMA);
    CASE('+', TOKEN_OPERATOR_PLUS);
    CASE('-', TOKEN_OPERATOR_MINUS);
    CASE('*', TOKEN_OPERATOR_MULTIPLY);
    CASE('/', TOKEN_OPERATOR_DIVIDE);
    CASE('^', TOKEN_OPERATOR_POW);
    CASE('=', TOKEN_ASSIGN);
    CASE('?', TOKEN_UNDEFINED);
    }
    t->end = s;
}

char *match_digits(char *s)
{
    while(CHAR_IN(*s, '0', '9'))
        s++;
    return s;
}

/**
 * 数字可以按顺序可以分为3部分
 * 1、数字串
 * 2、小数点 数字串 (数字串可以不存在)
 * 3、e±数字串
 * 1和2存在任意一项且包含数字串即可表示合法数字，3必须在合法数字存在的时候才能存在
 */ 
char *match_number(char *s)
{
    char *t = match_digits(s); // 尝试匹配第1项
    if (*t == '.') // 尝试匹配第2项
        t = match_digits(++t);
    // 判断是否为合法数字
    if (s == t || t - s == 1 && *s == '.')
        return s; // 当无匹配或仅匹配到一个点号时，不能成为一个合法数字
    // 得到一个合法数字后我们还能接受第3项
    s = t;
    if (*t == 'e' || *t == 'E')
    {
        t++;
        if (*t == '+' || *t == '-') t++;
        char *u = match_digits(t);
        if (u > t) s = u; // 完整匹配到第3项
    }
    return s;
}

void token_number(char *s, Token *t)
{
    t->end = match_number(s);
    t->symbol = TOKEN_NUMBER;
}

/**
 * 标识符以字母或者下划线开头，之后可以连续出现字母、数字、下划线
 */ 
void token_identifier(char *s, Token *t)
{
    #define IS_ALPHABET(c) (CHAR_IN(c, 'a', 'z') || CHAR_IN(c, 'A', 'Z'))
    if (*s == '_' || IS_ALPHABET(*s))
    {
        s++;
        while (*s == '_' || IS_ALPHABET(*s) || CHAR_IN(*s, '0', '9'))
            s++;
    }
    t->symbol = TOKEN_IDENTIFIER;
    t->end = s;
}

Token lexer_next_token(char *s)
{
    Token t;
    t.start = t.end = s;
    if (*s == '\0')
    {
        t.symbol = TOKEN_EOF;
        return t;
    }
    void *pipelines[] = {
        token_whitespace,
        token_simple_symbol,
        token_number,
        token_identifier
    };
    int size = sizeof(pipelines) / sizeof(void *);
    for (int i = 0; i < size; i++)
    {
        void (*f)(char *, Token *) = pipelines[i];
        f(s, &t);
        if (t.end != s) return t;
    }
    t.end = s + 1;
    t.symbol = TOKEN_UNKNOWN_CHAR;
    return t;
}