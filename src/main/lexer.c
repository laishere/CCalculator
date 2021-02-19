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
 * ������token
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
 * ���ֿ��԰�˳����Է�Ϊ3����
 * 1�����ִ�
 * 2��С���� ���ִ� (���ִ����Բ�����)
 * 3��e�����ִ�
 * 1��2��������һ���Ұ������ִ����ɱ�ʾ�Ϸ����֣�3�����ںϷ����ִ��ڵ�ʱ����ܴ���
 */ 
char *match_number(char *s)
{
    char *t = match_digits(s); // ����ƥ���1��
    if (*t == '.') // ����ƥ���2��
        t = match_digits(++t);
    // �ж��Ƿ�Ϊ�Ϸ�����
    if (s == t || t - s == 1 && *s == '.')
        return s; // ����ƥ����ƥ�䵽һ�����ʱ�����ܳ�Ϊһ���Ϸ�����
    // �õ�һ���Ϸ����ֺ����ǻ��ܽ��ܵ�3��
    s = t;
    if (*t == 'e' || *t == 'E')
    {
        t++;
        if (*t == '+' || *t == '-') t++;
        char *u = match_digits(t);
        if (u > t) s = u; // ����ƥ�䵽��3��
    }
    return s;
}

void token_number(char *s, Token *t)
{
    t->end = match_number(s);
    t->symbol = TOKEN_NUMBER;
}

/**
 * ��ʶ������ĸ�����»��߿�ͷ��֮���������������ĸ�����֡��»���
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