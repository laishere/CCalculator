#include "parser.h"
#include "grammar.h"
#include "lexer.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>

inline char *parser_get_symbol_name(ParserSymbol symbol)
{
    #define SYMBOL_NAME_CASE(S, N) case S: n = N; break
    char *n = "[δ����ı�ʶ]";
    switch (symbol)
    {
    SYMBOL_NAME_CASE(TOKEN_WHITESPACE, "�հ׷�");
    SYMBOL_NAME_CASE(TOKEN_LEFT_BRACKET, "(");
    SYMBOL_NAME_CASE(TOKEN_RIGHT_BRACKET, ")");
    SYMBOL_NAME_CASE(TOKEN_NUMBER, "����");
    SYMBOL_NAME_CASE(TOKEN_IDENTIFIER, "��ʶ��");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_PLUS, "+");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_MINUS, "-");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_MULTIPLY, "*");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_DIVIDE, "/");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_POW, "^");
    SYMBOL_NAME_CASE(TOKEN_ASSIGN, "=");
    SYMBOL_NAME_CASE(TOKEN_COMMA, ",");
    SYMBOL_NAME_CASE(TOKEN_UNDEFINED, "?");
    SYMBOL_NAME_CASE(TOKEN_UNKNOWN_CHAR, "δ֪�ַ�");
    SYMBOL_NAME_CASE(TOKEN_EOF, "EOF");
    SYMBOL_NAME_CASE(RULE_TARGET, "Ŀ��");
    SYMBOL_NAME_CASE(RULE_EXPR, "���ʽ");
    SYMBOL_NAME_CASE(RULE_ASSIGN_EXPR, "��ֵ���ʽ");
    SYMBOL_NAME_CASE(RULE_ADD_EXPR, "�Ӽ������ʽ");
    SYMBOL_NAME_CASE(RULE_MUL_EXPR, "�˳������ʽ");
    SYMBOL_NAME_CASE(RULE_POW_EXPR, "��������ʽ");
    SYMBOL_NAME_CASE(RULE_VALUE, "ֵ");
    SYMBOL_NAME_CASE(RULE_FUNC, "����");
    SYMBOL_NAME_CASE(RULE_ARG, "����");
    SYMBOL_NAME_CASE(RULE_NO_BRACKETS_VALUE, "����Χ���ŵ�ֵ");
    SYMBOL_NAME_CASE(RULE_REVESE_EXPR, "��׺���ʽ");
    SYMBOL_NAME_CASE(RULE_OPERATOR, "�����");
    SYMBOL_NAME_CASE(RULE_SIGNED_EXPR, "ǰ�ø��ű��ʽ");
    SYMBOL_NAME_CASE(RULE_FUNC_NAME, "������");
    }
    return n;
}

void parser_show_syntax_error(char *expr, ParseNode *node, HashMap *state)
{
    printf("%s\n", expr);
    for (char *c = expr; c < node->start; c++)
        printf(" ");
    if (node->start + 1 < node->end)
    {
        for (char *c = node->start; c < node->end; c++)
            printf("~");
    }
    else 
        printf("^");
    printf(" �˴�������Ϊ %s", parser_get_symbol_name(node->symbol));
    if (node->symbol == TOKEN_UNKNOWN_CHAR)
        printf(" %c", *node->start);
    printf("\n");
}

ParseNode *parse_new_node(Token *t)
{
    ParseNode *node = malloc(sizeof(ParseNode));
    node->children = NULL;
    node->symbol = t->symbol;
    node->start = t->start;
    node->end = t->end;
    node->extraInfo = NULL;
    switch (node->symbol)
    {
    case TOKEN_IDENTIFIER:
        {
            // ֱ�Ӹ����ַ�����Ϣ
            int size = node->end - node->start;
            char *s = malloc(size + 1);
            memcpy(s, node->start, size);
            s[size] = '\0'; // ע�������
            node->extraInfo = s;
        }
        break;
    case TOKEN_NUMBER:
        {
            double *number = malloc(sizeof(double));
            sscanf(node->start, "%lf", number);
            node->extraInfo = number;
        }
        break;
    }
    return node;
}

void parser_clear_node(ParseNode *node)
{
    if (!node) return;
    if (node->children)
    {
        stack_clear(node->children);
        free(node->children);
    }
    free(node->extraInfo);
    free(node);
}

void parser_clear_tree(ParseTree t)
{
    if (!t) return;
    if (t->children)
    {
        stack_for_each(t->children)
            parser_clear_tree(*p);
    }
    parser_clear_node(t);
}

void print_action(SRAction *a)
{
    if (a->action == ACTION_SHIFT)
        printf("[shift %s %p]\n", parser_get_symbol_name(a->shiftSymbol), a->shiftTo);
    else if (a->action == ACTION_ACCEPT)
        printf("[accept]\n");
    else printf("[reduce to %s]\n", parser_get_symbol_name(a->reduceTo));
}

ParseTree parser_simple_ast(ParseTree t)
{
    if (!t->children || stack_size(t->children) == 0)
        return t; // Ҷ�ӽڵ�
    if (stack_size(t->children) == 1)
    {
        ParseTree ast = parser_ast(t->children->base[0]);
        parser_clear_node(t);
        return ast;
    }
    stack_for_each(t->children)
        *p = parser_ast(*p);
    return t;
}

void parser_unwrap_args(ParseTree t, Stack *args, Stack *nodesToClearOnSuccess, int isRoot)
{
    int size = stack_size(t->children);
    if (stack_size(t->children) == 3)
    {
        // ���� , ���ʽ
        stack_push(nodesToClearOnSuccess, t->children->base[1]); // �����Ҫɾ���Ķ���
        parser_unwrap_args(t->children->base[0], args, nodesToClearOnSuccess, 0);
    }
    // ���һ���ض��Ǳ��ʽ��ȡ����
    stack_push(args, t->children->base[size - 1]);
    if (!isRoot)
        stack_push(nodesToClearOnSuccess, t);
}

/**
 * ���﷨��ת��Ϊ�����﷨��
 */ 
ParseTree parser_ast(ParseTree t)
{
    if (!t) return NULL;
    if (t->symbol == RULE_REVESE_EXPR)
    {
        // �Ѻ�׺���ʽת��Ϊ������ʽ�ṹ
        ParseTree op = parser_simple_ast(t->children->base[2]);
        if (op->symbol == TOKEN_OPERATOR_PLUS || op->symbol == TOKEN_OPERATOR_MINUS)
            t->symbol = RULE_ADD_EXPR; // �Ӽ������ʽ
        else if (op->symbol == TOKEN_OPERATOR_MULTIPLY || op->symbol == TOKEN_OPERATOR_DIVIDE)
            t->symbol = RULE_MUL_EXPR; // �˳������ʽ
        else t->symbol = RULE_POW_EXPR; // ��������ʽ
        // ������ʽ����ʽ��: a ����� b������������Ҫ��2��3λ����
        t->children->base[2] = t->children->base[1];
        t->children->base[1] = op;
    }
    switch(t->symbol)
    {
    case RULE_TARGET:
        // ���ʽ EOF
        {
            parser_clear_tree(t->children->base[1]); // ɾ�� EOF
            stack_pop(t->children);
        }
        break;
    case RULE_ASSIGN_EXPR:
        // ��ʶ�� = ���ʽ
        {
            parser_clear_tree(t->children->base[1]); // ɾ���Ⱥ�
            t->children->base[1] = t->children->base[2]; // �Ⱥŵ�λ���滻Ϊ���ʽ
            stack_pop(t->children);
        }
        break;
    case RULE_POW_EXPR:
        // ֵ | ��������ʽ ^ ֵ
        if (stack_size(t->children) > 1) 
        {
            parser_clear_tree(t->children->base[1]); // ɾ�� ^
            t->children->base[1] = t->children->base[2];
            stack_pop(t->children);
        }
        break;
    case RULE_FUNC:
        // ��ʶ�� ( ���� )
        {
            parser_clear_tree(t->children->base[1]); // ɾ��������
            parser_clear_tree(t->children->base[3]); // ɾ��������
            t->children->base[1] = t->children->base[2]; // �����ŵ��ڶ���λ��
            t->children->top = t->children->base + 2; // ֻʣ����Ԫ�أ�Ųһ��topָ��
        }
        break;
    case RULE_VALUE:
        // ����Χ���ŵ�ֵ | ( ���ʽ )
        if (stack_size(t->children) > 1)
        {
            parser_clear_tree(t->children->base[0]); // ɾ��������
            parser_clear_tree(t->children->base[2]); // ɾ��������
            t->children->base[0] = t->children->base[1]; // ���ʽŲ����λ
            t->children->top = t->children->base + 1; // ��ʣһ��Ԫ�� 
        }
        break;
    case RULE_SIGNED_EXPR:
        // - �˳������ʽ
        parser_clear_tree(t->children->base[0]); // ɾ������
        ParseTree expr = t->children->base[0] = parser_ast(t->children->base[1]);
        stack_pop(t->children);
        if (expr->symbol == RULE_SIGNED_EXPR)
        {
            // ����������ȥ������
            ParseTree tmp = expr->children->base[0];
            parser_clear_node(expr);
            parser_clear_node(t);
            t = tmp;
        }
        return t; // ����Ҫ��������ֱ�ӷ���
    }
    t = parser_simple_ast(t);
    if (t->symbol == RULE_ADD_EXPR)
    {
        // a �� b ����ʽ��������Ҫ���� b Ϊǰ�ø��ű��ʽ�����
        ParseTree b = t->children->base[2];
        if (b->symbol == RULE_SIGNED_EXPR)
        {
            ParseTree op = t->children->base[1];
            // �任�����
            if (op->symbol == TOKEN_OPERATOR_PLUS)
                op->symbol = TOKEN_OPERATOR_MINUS;
            else
                op->symbol = TOKEN_OPERATOR_PLUS;
            t->children->base[2] = b->children->base[0];
            parser_clear_node(b);
        }
    }
    return t;
}

/**
 * @return �ɹ�������1������0
 */ 
int parser_handle_func(ParseTree t, Stack *nodesToClearOnSuccess)
{
    // ������
    // ���� -> ��ʶ�� ( ���� )
    #define FUNC0(NAME, TYPE) if (memcmp(NAME, funcName, size) == 0) func = TYPE
    #define FUNC(NAME, TYPE) else FUNC0(NAME, TYPE)
    SupportedFunction func;
    ParseTree id = t->children->base[0];
    int size = id->end - id->start;
    char *funcName = id->extraInfo;
    FUNC0("sin", FUNC_SIN);
    FUNC("cos", FUNC_COS);
    FUNC("tan", FUNC_TAN);
    FUNC("asin", FUNC_ASIN);
    FUNC("arcsin", FUNC_ASIN);
    FUNC("acos", FUNC_ACOS);
    FUNC("arccos", FUNC_ACOS);
    FUNC("atan", FUNC_ATAN);
    FUNC("arctan", FUNC_ATAN);
    FUNC("ln", FUNC_LN);
    FUNC("abs", FUNC_ABS);
    FUNC("sqrt", FUNC_SQRT);
    FUNC("diff", FUNC_DIFF);
    else 
    {
        printf("��֧�ֵĺ��� %s\n", funcName);
        return 0;
    }
    int success = 1;
    ParseTree args = t->children->base[2];
    Stack *newChildren = malloc(sizeof(Stack));
    stack_init(newChildren, 8, 8);
    parser_unwrap_args(args, newChildren, nodesToClearOnSuccess, 1);
    stack_clear(args->children); // ɾ���ɵ��ӽڵ�
    free(args->children);
    args->children = newChildren;
    int argc = stack_size(newChildren);
    /*if (func == FUNC_DIFF)
    {
        if (argc != 3)
        {
            printf("diff �����Ĳ�����ʽ�� (�������ʽ, ������, �󵼽���)\n");     
            success = 0;
        }
    }
    else if (argc != 1)
    {
        printf("%s ����ֻ����1������\n", funcName);
        success = 0;
    }*/
    if (success)
    {
        free(id->extraInfo);
        id->extraInfo = malloc(sizeof(SupportedFunction));
        *(SupportedFunction*)id->extraInfo = func;
        id->symbol = RULE_FUNC_NAME;
    }
    return success;
}

ParseTree parser_parse(char *expr)
{
    ParseTree target = NULL;
    Stack nodes, maps, allocatedNodes, nodesToClearOnSuccess;
    stack_init(&nodes, 16, 16);
    stack_init(&maps, 16, 16);
    stack_init(&allocatedNodes, 16, 16);
    stack_init(&nodesToClearOnSuccess, 16, 16);
    HashMap *map = build_grammar();
    char *nextExpr = expr, lastS;
    ParseNode *node, *reduceNode, *lookAheadNode;
    reduceNode = lookAheadNode = NULL;
    Token token;
    while (1)
    {
        if (reduceNode)
        {
            // ��һ�������reduce��������Ҫ���������reduceNode
            node = reduceNode;
            reduceNode = NULL;
        }
        else if (lookAheadNode)
        {
            // ֮ǰ�����reduce��������ô���ǿ�����Ҫ����lookAheadNode
            node = lookAheadNode;
            lookAheadNode = NULL;
        }
        else
        {
            // û�д������reduceNode��lookAheadNode������ֱ�Ӵ�����һ��token
            token = lexer_next_token(nextExpr);
            nextExpr = token.end;
            if (token.symbol == TOKEN_WHITESPACE)
                continue; // �Թ��հ׷�
            node = parse_new_node(&token);
            stack_push(&allocatedNodes, node);
        }
        SRAction *action = hash_map_get(map, node->symbol);
        if (!action)
        {
            parser_show_syntax_error(expr, node, map);
            goto ERROR;
        }
        if (action->action == ACTION_REDUCE || action->action == ACTION_ACCEPT)
        {
            // ACCEPT �൱�� shift �� reduce��������
            if (action->action == ACTION_ACCEPT)
                stack_push(&nodes, node); // ��shift
            assert(stack_size(&nodes) >= action->reduceNodeSize);
            void **p = nodes.top - action->reduceNodeSize;
            ParseNode *pNode = *p;
            reduceNode = malloc(sizeof(ParseNode));
            stack_push(&allocatedNodes, reduceNode);
            reduceNode->symbol = action->reduceTo;
            reduceNode->children = malloc(sizeof(Stack));
            stack_init(reduceNode->children, action->reduceNodeSize, 8);
            reduceNode->extraInfo = NULL;
            reduceNode->start = pNode->start;
            for (; p < nodes.top; p++)
            {
                pNode = *p;
                stack_push(reduceNode->children, pNode);
            }
            reduceNode->end = pNode->end;
            if (reduceNode->symbol == RULE_FUNC)
            {
                if (!parser_handle_func(reduceNode, &nodesToClearOnSuccess))
                    goto ERROR;
            }
            nodes.top -= action->reduceNodeSize; // ֱ�Ӳ���topָ���ջ
            maps.top -= action->reduceNodeSize - 1; // state���ų�ջ
            map = stack_pop(&maps);
            if (action->action == ACTION_ACCEPT)
            {
                target = reduceNode;
                break;
            }
            lookAheadNode = node; // ��ǰ�����node��ΪlookAheadNode
        } 
        else 
        {
            stack_push(&nodes, node);
            stack_push(&maps, map);
            map = action->shiftTo;
        }
    }
    stack_for_each(&nodesToClearOnSuccess)
        parser_clear_node(*p);
    goto SKIP_ERROR;
ERROR:
    stack_for_each(&allocatedNodes)
        parser_clear_node(*p);
SKIP_ERROR:
    stack_clear(&nodes);
    stack_clear(&maps);
    stack_clear(&allocatedNodes);
    stack_clear(&nodesToClearOnSuccess);
    return target;
}

char *simple_tree2expr(ParseTree ast, char *buf)
{
    stack_for_each(ast->children)
        buf = tree2expr(*p, buf);
    return buf;
}

ParserSymbol expr_tree_get_operator(ParseTree expr)
{
    ParseTree tmp;
    switch (expr->symbol)
    {
    case RULE_ADD_EXPR:
    case RULE_MUL_EXPR:
        tmp = expr->children->base[1];
        return tmp->symbol;
    case RULE_REVESE_EXPR:
        tmp = expr->children->base[2];
        return tmp->symbol;
    case RULE_SIGNED_EXPR:
        return TOKEN_OPERATOR_MINUS;
    }
    return TOKEN_OPERATOR_POW; // ������ֵ����������ʽͬ��һ�����
}

char *tree2expr_signed_expr_child(ParseTree t, char *buf)
{
    // �ж����t֮ǰ�Ƿ���Ҫ���ţ��൱���ж� 0 - t�Ƿ���Ҫ���ţ���tΪ�Ӽ�������ʱ
    if (t->symbol == RULE_ADD_EXPR)
    {
        // t�����ǼӼ������ʽ�⣬�����ǰ�ø��ű��ʽҲӦ�����ţ�����������תast��ʱ���Ѿ���ȥ������ǰ�ø��ŵ������
        // ���� ������ -(-(-t))������ast������Ϊ��-t
        *(buf++) = '(';
        buf = tree2expr(t, buf);
        *(buf++) = ')';
    }
    else buf = tree2expr(t, buf);
    return buf;
}

/**
 * ������Ҫ������������
 */ 
char *tree2expr_with_priority_check(ParseTree parent, int child, int isFirst, char *buf)
{
    /*
    1����������ʽ��a ^ b�����aҲ����������ʽ����ô����Ҫ������ţ�������Ҫ��b�Ĵ���ʽһ��
    2���˷����ʽ a * b�����a�ǳ˳������ʽ������������ʽ����ôa����Ҫ���ţ�������Ҫ��b�Ĵ���ʽһ��
    3���������ʽ a / b
        ���a�ǳ˳������ʽ������������ʽ��ôa����Ҫ���ţ�������Ҫ
        ���b����������ʽ��ôb����Ҫ���ţ�������Ҫ
    4���ӷ����ʽ a + b��a �� b ������Ҫ����
    5���������ʽ a - b��a����Ҫ���ţ����b�ǼӼ������ʽ��ôb��Ҫ���ţ�������Ҫ
    ��Ҫע����ǣ�������ֵ��Ϊ��������ʽ
    */
    ParseTree ch = parent->children->base[child];
    ParserSymbol op1 = expr_tree_get_operator(parent);
    ParserSymbol op2 = expr_tree_get_operator(ch);
    ParserSymbol outputOp = op1;
    int needBrackets = 1;
    if (op2 == TOKEN_OPERATOR_POW)
    {
        if (op1 == TOKEN_OPERATOR_POW && ch->symbol == TOKEN_NUMBER && *(double*)ch->extraInfo < 0.)
        {
            /* 
            �����������Ϊ����������Ҫ������ţ����� (-1) ^ x �����������žͻ����Ϊ -1 ^ x
            ��������ǼӼ������ʽ���߳˳������ʽ�����ǲ�����Ҫ��������Ϊ�����������Ϊ����������У�
            ��������Ϊ����ʱ�����ǻ��ʽ��ת��Ϊû�и���������ʽ����������ת��ʽ�ӱ�ֱ�����������ͼ����Ÿ������ڵõ�һ������ʽ��
            */
           needBrackets = 1;
        }
        else needBrackets = 0;
    }
    else if (op1 == TOKEN_OPERATOR_MULTIPLY && ch->symbol == RULE_MUL_EXPR)
        needBrackets = 0; // ����� a * b ����ʽ ��a��bΪ�˳����ʽ����ô����Ҫ���ţ������if����ų���a��bΪ��������ʽ�������
    else if (op1 == TOKEN_OPERATOR_DIVIDE)
    {
        if (isFirst && ch->symbol == RULE_MUL_EXPR)
            needBrackets = 0; // ��� Ϊ a * b����ʽ��a���Ϊ�˳����ʽ����ô����Ҫ���ţ�b�ų�����������ʽ�Ŀ��ܣ�������Ҫ����
    }
    else if (op1 == TOKEN_OPERATOR_PLUS)
        needBrackets = 0; // a + b ������¿϶�����Ҫ����
    else if (op1 == TOKEN_OPERATOR_MINUS)
    {
        if (isFirst) needBrackets = 0; // a - b �������a����Ҫ����
        else if (ch->symbol != RULE_ADD_EXPR)
            needBrackets = 0; // a - b �� b ���ǼӼ����ʽʱ��������
    }
    if (!isFirst)
    {
        // ����������
        *(buf++) = ' ';
        ParseNode node;
        node.symbol = op1;
        buf = tree2expr(&node, buf);        
        *(buf++) = ' ';
    }
    if (needBrackets)
    {
        *(buf++) = '(';
        buf = tree2expr(ch, buf);
        *(buf++) = ')';
    }
    else buf = tree2expr(ch, buf);
    return buf;
}

char *tree2expr_func_name(ParseTree id, char *buf)
{
    #define FUNC_NAME_CASE(F, N) case F: buf += sprintf(buf, N); break
    switch (*(SupportedFunction*)id->extraInfo)
    {
    FUNC_NAME_CASE(FUNC_SIN, "sin");
    FUNC_NAME_CASE(FUNC_COS, "cos");
    FUNC_NAME_CASE(FUNC_TAN, "tan");
    FUNC_NAME_CASE(FUNC_ASIN, "asin");
    FUNC_NAME_CASE(FUNC_ACOS, "acos");
    FUNC_NAME_CASE(FUNC_ATAN, "atan");
    FUNC_NAME_CASE(FUNC_LN, "ln");
    FUNC_NAME_CASE(FUNC_ABS, "abs");
    FUNC_NAME_CASE(FUNC_SQRT, "sqrt");
    FUNC_NAME_CASE(FUNC_DIFF, "diff");
    }
    return buf;
}

char *tree2expr(ParseTree ast, char *buf)
{
    if (!ast) return buf;
    #define TOKEN_CASE(T, N) case T: buf += sprintf(buf, N); break
    switch (ast->symbol)
    {
    case TOKEN_NUMBER:
        {
            double d = *(double *)ast->extraInfo;
            if (isnan(d))
                buf += sprintf(buf, "NAN");
            else if (isinf(d))
                buf += sprintf(buf, "INFINITY");
            else
                buf += sprintf(buf, "%g", d);
        }
        break;
    TOKEN_CASE(TOKEN_IDENTIFIER, ast->extraInfo);
    TOKEN_CASE(TOKEN_OPERATOR_PLUS, "+");
    TOKEN_CASE(TOKEN_OPERATOR_MINUS, "-");
    TOKEN_CASE(TOKEN_OPERATOR_MULTIPLY, "*");
    TOKEN_CASE(TOKEN_OPERATOR_DIVIDE, "/");
    TOKEN_CASE(TOKEN_OPERATOR_POW, "^");
    case RULE_ARG:
        stack_for_each(ast->children)
        {
            if (p - ast->children->base > 0)
                buf += sprintf(buf, ", ");
            buf = tree2expr(*p, buf);
        }
        break;
    case RULE_ASSIGN_EXPR:
        buf = tree2expr(ast->children->base[0], buf);
        buf += sprintf(buf, " = ");
        buf = tree2expr(ast->children->base[1], buf);
        break;
    case RULE_FUNC:
        buf = tree2expr_func_name(ast->children->base[0], buf);
        *(buf++) = '(';
        buf = tree2expr(ast->children->base[1], buf);
        *(buf++) = ')';
        break;
    case RULE_POW_EXPR:
        buf = tree2expr_with_priority_check(ast, 0, 1, buf);
        buf = tree2expr_with_priority_check(ast, 1, 0, buf);
        break;
    case RULE_MUL_EXPR:
    case RULE_ADD_EXPR:
        buf = tree2expr_with_priority_check(ast, 0, 1, buf);
        buf = tree2expr_with_priority_check(ast, 2, 0, buf);
        break;
    case RULE_SIGNED_EXPR:
        *(buf++) = '-';
        buf = tree2expr_signed_expr_child(ast->children->base[0], buf);
        break;
    // ast�в�����׺���ʽ��������
    default:
        buf = simple_tree2expr(ast, buf);
    }
    return buf;
}

void print_expr(ParseTree t)
{
    char buf[10000];
    *tree2expr(t, buf) = '\0';
    printf("%s\n", buf);
}