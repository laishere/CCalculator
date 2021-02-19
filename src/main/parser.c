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
    char *n = "[未定义的标识]";
    switch (symbol)
    {
    SYMBOL_NAME_CASE(TOKEN_WHITESPACE, "空白符");
    SYMBOL_NAME_CASE(TOKEN_LEFT_BRACKET, "(");
    SYMBOL_NAME_CASE(TOKEN_RIGHT_BRACKET, ")");
    SYMBOL_NAME_CASE(TOKEN_NUMBER, "数字");
    SYMBOL_NAME_CASE(TOKEN_IDENTIFIER, "标识符");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_PLUS, "+");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_MINUS, "-");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_MULTIPLY, "*");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_DIVIDE, "/");
    SYMBOL_NAME_CASE(TOKEN_OPERATOR_POW, "^");
    SYMBOL_NAME_CASE(TOKEN_ASSIGN, "=");
    SYMBOL_NAME_CASE(TOKEN_COMMA, ",");
    SYMBOL_NAME_CASE(TOKEN_UNDEFINED, "?");
    SYMBOL_NAME_CASE(TOKEN_UNKNOWN_CHAR, "未知字符");
    SYMBOL_NAME_CASE(TOKEN_EOF, "EOF");
    SYMBOL_NAME_CASE(RULE_TARGET, "目标");
    SYMBOL_NAME_CASE(RULE_EXPR, "表达式");
    SYMBOL_NAME_CASE(RULE_ASSIGN_EXPR, "赋值表达式");
    SYMBOL_NAME_CASE(RULE_ADD_EXPR, "加减法表达式");
    SYMBOL_NAME_CASE(RULE_MUL_EXPR, "乘除法表达式");
    SYMBOL_NAME_CASE(RULE_POW_EXPR, "幂运算表达式");
    SYMBOL_NAME_CASE(RULE_VALUE, "值");
    SYMBOL_NAME_CASE(RULE_FUNC, "函数");
    SYMBOL_NAME_CASE(RULE_ARG, "参数");
    SYMBOL_NAME_CASE(RULE_NO_BRACKETS_VALUE, "无外围括号的值");
    SYMBOL_NAME_CASE(RULE_REVESE_EXPR, "后缀表达式");
    SYMBOL_NAME_CASE(RULE_OPERATOR, "运算符");
    SYMBOL_NAME_CASE(RULE_SIGNED_EXPR, "前置负号表达式");
    SYMBOL_NAME_CASE(RULE_FUNC_NAME, "函数名");
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
    printf(" 此处不可以为 %s", parser_get_symbol_name(node->symbol));
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
            // 直接复制字符串信息
            int size = node->end - node->start;
            char *s = malloc(size + 1);
            memcpy(s, node->start, size);
            s[size] = '\0'; // 注意结束符
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
        return t; // 叶子节点
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
        // 参数 , 表达式
        stack_push(nodesToClearOnSuccess, t->children->base[1]); // 添加需要删除的逗号
        parser_unwrap_args(t->children->base[0], args, nodesToClearOnSuccess, 0);
    }
    // 最后一个必定是表达式，取出来
    stack_push(args, t->children->base[size - 1]);
    if (!isRoot)
        stack_push(nodesToClearOnSuccess, t);
}

/**
 * 把语法树转换为抽象语法树
 */ 
ParseTree parser_ast(ParseTree t)
{
    if (!t) return NULL;
    if (t->symbol == RULE_REVESE_EXPR)
    {
        // 把后缀表达式转换为常规表达式结构
        ParseTree op = parser_simple_ast(t->children->base[2]);
        if (op->symbol == TOKEN_OPERATOR_PLUS || op->symbol == TOKEN_OPERATOR_MINUS)
            t->symbol = RULE_ADD_EXPR; // 加减法表达式
        else if (op->symbol == TOKEN_OPERATOR_MULTIPLY || op->symbol == TOKEN_OPERATOR_DIVIDE)
            t->symbol = RULE_MUL_EXPR; // 乘除法表达式
        else t->symbol = RULE_POW_EXPR; // 幂运算表达式
        // 常规表达式的形式是: a 运算符 b，所以我们需要把2、3位互换
        t->children->base[2] = t->children->base[1];
        t->children->base[1] = op;
    }
    switch(t->symbol)
    {
    case RULE_TARGET:
        // 表达式 EOF
        {
            parser_clear_tree(t->children->base[1]); // 删除 EOF
            stack_pop(t->children);
        }
        break;
    case RULE_ASSIGN_EXPR:
        // 标识符 = 表达式
        {
            parser_clear_tree(t->children->base[1]); // 删除等号
            t->children->base[1] = t->children->base[2]; // 等号的位置替换为表达式
            stack_pop(t->children);
        }
        break;
    case RULE_POW_EXPR:
        // 值 | 幂运算表达式 ^ 值
        if (stack_size(t->children) > 1) 
        {
            parser_clear_tree(t->children->base[1]); // 删除 ^
            t->children->base[1] = t->children->base[2];
            stack_pop(t->children);
        }
        break;
    case RULE_FUNC:
        // 标识符 ( 参数 )
        {
            parser_clear_tree(t->children->base[1]); // 删除左括号
            parser_clear_tree(t->children->base[3]); // 删除右括号
            t->children->base[1] = t->children->base[2]; // 参数放到第二个位置
            t->children->top = t->children->base + 2; // 只剩两个元素，挪一下top指针
        }
        break;
    case RULE_VALUE:
        // 无外围括号的值 | ( 表达式 )
        if (stack_size(t->children) > 1)
        {
            parser_clear_tree(t->children->base[0]); // 删除左括号
            parser_clear_tree(t->children->base[2]); // 删除右括号
            t->children->base[0] = t->children->base[1]; // 表达式挪到首位
            t->children->top = t->children->base + 1; // 仅剩一个元素 
        }
        break;
    case RULE_SIGNED_EXPR:
        // - 乘除法表达式
        parser_clear_tree(t->children->base[0]); // 删除负号
        ParseTree expr = t->children->base[0] = parser_ast(t->children->base[1]);
        stack_pop(t->children);
        if (expr->symbol == RULE_SIGNED_EXPR)
        {
            // 负负得正，去掉负号
            ParseTree tmp = expr->children->base[0];
            parser_clear_node(expr);
            parser_clear_node(t);
            t = tmp;
        }
        return t; // 不需要继续处理，直接返回
    }
    t = parser_simple_ast(t);
    if (t->symbol == RULE_ADD_EXPR)
    {
        // a ± b 的形式，我们需要处理 b 为前置负号表达式的情况
        ParseTree b = t->children->base[2];
        if (b->symbol == RULE_SIGNED_EXPR)
        {
            ParseTree op = t->children->base[1];
            // 变换运算符
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
 * @return 成功处理返回1，否则0
 */ 
int parser_handle_func(ParseTree t, Stack *nodesToClearOnSuccess)
{
    // 处理函数
    // 函数 -> 标识符 ( 参数 )
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
        printf("不支持的函数 %s\n", funcName);
        return 0;
    }
    int success = 1;
    ParseTree args = t->children->base[2];
    Stack *newChildren = malloc(sizeof(Stack));
    stack_init(newChildren, 8, 8);
    parser_unwrap_args(args, newChildren, nodesToClearOnSuccess, 1);
    stack_clear(args->children); // 删除旧的子节点
    free(args->children);
    args->children = newChildren;
    int argc = stack_size(newChildren);
    /*if (func == FUNC_DIFF)
    {
        if (argc != 3)
        {
            printf("diff 函数的参数形式是 (函数表达式, 增量名, 求导阶数)\n");     
            success = 0;
        }
    }
    else if (argc != 1)
    {
        printf("%s 函数只接受1个参数\n", funcName);
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
            // 上一步如果是reduce，我们需要处理产生的reduceNode
            node = reduceNode;
            reduceNode = NULL;
        }
        else if (lookAheadNode)
        {
            // 之前如果有reduce操作，那么我们可能需要处理lookAheadNode
            node = lookAheadNode;
            lookAheadNode = NULL;
        }
        else
        {
            // 没有待处理的reduceNode和lookAheadNode，我们直接处理下一个token
            token = lexer_next_token(nextExpr);
            nextExpr = token.end;
            if (token.symbol == TOKEN_WHITESPACE)
                continue; // 略过空白符
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
            // ACCEPT 相当于 shift 和 reduce两个操作
            if (action->action == ACTION_ACCEPT)
                stack_push(&nodes, node); // 先shift
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
            nodes.top -= action->reduceNodeSize; // 直接操作top指针出栈
            maps.top -= action->reduceNodeSize - 1; // state跟着出栈
            map = stack_pop(&maps);
            if (action->action == ACTION_ACCEPT)
            {
                target = reduceNode;
                break;
            }
            lookAheadNode = node; // 当前处理的node变为lookAheadNode
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
    return TOKEN_OPERATOR_POW; // 独立的值与幂运算表达式同属一种情况
}

char *tree2expr_signed_expr_child(ParseTree t, char *buf)
{
    // 判断输出t之前是否需要括号，相当于判断 0 - t是否需要括号，当t为加减法运算时
    if (t->symbol == RULE_ADD_EXPR)
    {
        // t除了是加减法表达式外，如果是前置负号表达式也应加括号，但是我们在转ast的时候已经把去掉连续前置负号的情况了
        // 比如 本来是 -(-(-t))，经过ast处理后变为了-t
        *(buf++) = '(';
        buf = tree2expr(t, buf);
        *(buf++) = ')';
    }
    else buf = tree2expr(t, buf);
    return buf;
}

/**
 * 按照需要添加括号再输出
 */ 
char *tree2expr_with_priority_check(ParseTree parent, int child, int isFirst, char *buf)
{
    /*
    1、幂运算表达式：a ^ b，如果a也是幂运算表达式，那么不需要添加括号，否则需要，b的处理方式一致
    2、乘法表达式 a * b，如果a是乘除法表达式或者幂运算表达式，那么a不需要括号，否则需要，b的处理方式一致
    3、除法表达式 a / b
        如果a是乘除法表达式或者幂运算表达式那么a不需要括号，否则需要
        如果b是幂运算表达式那么b不需要括号，否则需要
    4、加法表达式 a + b，a 和 b 都不需要括号
    5、减法表达式 a - b，a不需要括号，如果b是加减法表达式那么b需要括号，否则不需要
    需要注意的是，独立的值视为幂运算表达式
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
            如果是数字且为负，我们需要添加括号，比如 (-1) ^ x 如果不添加括号就会输出为 -1 ^ x
            但是如果是加减法表达式或者乘除法表达式，我们并不需要考虑数字为负的情况，因为在运算过程中，
            遇到数字为负的时候我们会把式子转换为没有负常数的形式，在运算中转化式子比直接遇到负数就加括号更有利于得到一个简洁的式子
            */
           needBrackets = 1;
        }
        else needBrackets = 0;
    }
    else if (op1 == TOKEN_OPERATOR_MULTIPLY && ch->symbol == RULE_MUL_EXPR)
        needBrackets = 0; // 如果是 a * b 的形式 且a或b为乘除表达式，那么不需要括号（上面的if语句排除了a或b为幂运算表达式的情况）
    else if (op1 == TOKEN_OPERATOR_DIVIDE)
    {
        if (isFirst && ch->symbol == RULE_MUL_EXPR)
            needBrackets = 0; // 如果 为 a * b的形式，a如果为乘除表达式，那么不需要括号，b排除掉幂运算表达式的可能，所以需要括号
    }
    else if (op1 == TOKEN_OPERATOR_PLUS)
        needBrackets = 0; // a + b 的情况下肯定不需要括号
    else if (op1 == TOKEN_OPERATOR_MINUS)
    {
        if (isFirst) needBrackets = 0; // a - b 的情况下a不需要括号
        else if (ch->symbol != RULE_ADD_EXPR)
            needBrackets = 0; // a - b 且 b 不是加减表达式时不需括号
    }
    if (!isFirst)
    {
        // 先输出运算符
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
    // ast中不含后缀表达式，不考虑
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