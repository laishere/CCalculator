#include "evaluator.h"
#include "map.h"
#include "set.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <assert.h>

int EVALUATOR_INITIALIZED = 0;
HashMap identifierMap, tmpIdentifierMap, identifierDependencyMap;
Stack allocatedParseNode;
char evaluatorError[500];

void evaluator_init()
{
    if (EVALUATOR_INITIALIZED)
        return;
    EVALUATOR_INITIALIZED = 1;
    hash_map_init(&identifierMap);
    hash_map_init(&tmpIdentifierMap);
    hash_map_init(&identifierDependencyMap);
}

ParseNode *new_parse_node(ParserSymbol symbol, int autoFree)
{
    ParseNode *n = malloc(sizeof(ParseNode));
    if (autoFree)
        stack_push(&allocatedParseNode, n);
    n->symbol = symbol;
    n->start = n->end = NULL;
    n->children = NULL;
    n->extraInfo = NULL;
    return n;
}

ParseTree copy_parse_tree(ParseTree t, int autoFree)
{
    if (!t) return NULL;
    ParseTree s = new_parse_node(t->symbol, autoFree);
    s->start = t->start;
    s->end = t->end;
    int size = 0;
    switch (s->symbol)
    {
    case TOKEN_NUMBER: size = sizeof(double); break;
    case TOKEN_IDENTIFIER: size = t->end - t->start + 1; break;
    case RULE_FUNC_NAME: size = sizeof(SupportedFunction);
    }
    if (size > 0)
    {
        s->extraInfo = malloc(size);
        memcpy(s->extraInfo, t->extraInfo, size);
    }
    if (t->children)
    {
        s->children = malloc(sizeof(Stack));
        stack_init(s->children, stack_size(t->children), 8);
        stack_for_each(t->children)
            stack_push(s->children, copy_parse_tree(*p, autoFree));
    }
    return s;
}

ParseTree parse_tree_add_sign(ParseTree t)
{
    if (t->symbol == RULE_SIGNED_EXPR)
    {
        // 已经是前置负号表达式，负负得正，去掉负号
        return t->children->base[0];
    }
    // 套一个前置负号
    ParseTree s = new_parse_node(RULE_SIGNED_EXPR, 1);
    s->children = malloc(sizeof(Stack));
    stack_init(s->children, 1, 8);
    stack_push(s->children, t);
    return s;
}

int expr_get_number(ParseTree expr, double *number)
{
    if (!expr) return 0;
    int sign = 1;
    if (expr->symbol == RULE_SIGNED_EXPR)
    {
        sign = -1;
        expr = expr->children->base[0];
    }
    if (expr->symbol == TOKEN_NUMBER)
    {
        *number = *(double*)expr->extraInfo;
        if (sign < 0)
            *number = -*number;
        return 1;
    }
    return 0;
}

ParseTree expr_get_x(ParseTree expr)
{
    // expr是经过简化的，简化后，要么是一个常数，要么是一个带未知数的表达式
    int sign = 1;
    ParseTree t = NULL;
    if (expr->symbol == RULE_SIGNED_EXPR)
    {
        sign = -1;
        expr = expr->children->base[0];
    }
    switch (expr->symbol)
    {
    case TOKEN_NUMBER: break; // 常数
    case RULE_ADD_EXPR:
        {
            double n;
            if (expr_get_number(expr->children->base[2], &n))
                t = expr->children->base[0]; // 如果2是常数，那么0必定是含未知数
            else if (expr_get_number(expr->children->base[0], &n))
            {
                // 如果0是常数，那么2必定是含未知数
                ParseTree op = expr->children->base[1];
                if (op->symbol == TOKEN_OPERATOR_MINUS)
                    sign = -sign; // 系数为负
                t = expr->children->base[2];
            }
            else
                t = expr; // 0 2都不是数字，那么整个式子无法分出常数
        }
        break;
    case TOKEN_IDENTIFIER:
    case RULE_MUL_EXPR:
    case RULE_POW_EXPR:
    case RULE_FUNC:
        // 标识符是一个未知数，乘除法表达式、幂运算表达式和函数中的 未知数 和 常数 不可分
        t = expr;
        break;
    }
    if (t && sign < 0)
        t = parse_tree_add_sign(t);
    return t;
}

ParseTree expr_get_c(ParseTree expr)
{
    int sign = 1;
    ParseTree t = NULL;
    if (expr->symbol == RULE_SIGNED_EXPR)
    {
        sign = -1;
        expr = expr->children->base[0];
    }
    if (expr->symbol == TOKEN_NUMBER)
        t = expr;
    else if (expr->symbol == RULE_ADD_EXPR)
    {
        double n;
        if (expr_get_number(expr->children->base[0], &n))
            t = expr->children->base[0];
        else if (expr_get_number(expr->children->base[2], &n))
        {
            ParseTree op = expr->children->base[1];
            if (op->symbol == TOKEN_OPERATOR_MINUS)
                sign = -sign; // 系数为负
            t = expr->children->base[2];
        }
    }
    if (t && sign < 0)
        t = parse_tree_add_sign(t);
    return t;
}

ParseTree parse_tree_new_number(double n)
{
    ParseTree c = new_parse_node(TOKEN_NUMBER, 1);
    c->extraInfo = malloc(sizeof(double));
    *(double*)c->extraInfo = n;
    return c;
}

ParseTree diff(ParseTree f, char *x, int level);
ParseTree evaluator_simplify(ParseTree t);

ParseTree parse_tree_cal_expr(ParseTree expr)
{
    if (!expr) return NULL;
    switch (expr->symbol)
    {
    case RULE_ADD_EXPR:
    case RULE_MUL_EXPR:
    case RULE_POW_EXPR:
        {
            double a, b;
            ParserSymbol op = TOKEN_OPERATOR_POW;
            if (expr->symbol != RULE_POW_EXPR)
            {
                ParseTree opTree = expr->children->base[1];
                op = opTree->symbol;
            }
            int bIndex = (expr->symbol == RULE_POW_EXPR ? 1 : 2);
            int aIsNumber = expr_get_number(expr->children->base[0], &a);
            int bIsNumber = expr_get_number(expr->children->base[bIndex], &b);
            if (aIsNumber && bIsNumber)
            {
                switch (op)
                {
                case TOKEN_OPERATOR_PLUS: a += b; break;
                case TOKEN_OPERATOR_MINUS: a -= b; break;
                case TOKEN_OPERATOR_MULTIPLY: a *= b; break;
                case TOKEN_OPERATOR_DIVIDE: a /= b; break;
                case TOKEN_OPERATOR_POW: a = pow(a, b); break;
                }
                return parse_tree_new_number(a);
            }
            ParseTree x = NULL;
            x = expr->children->base[0];
            int aIsNegative = aIsNumber && a < 0. || !aIsNumber && x->symbol == RULE_SIGNED_EXPR;
            ParseTree positiveA = x;
            if (aIsNegative)
            {
                if (aIsNumber && a < 0.)
                    positiveA = parse_tree_new_number(-a);
                else
                    positiveA = parse_tree_add_sign(x);
            }
            else if (aIsNumber)
                positiveA = parse_tree_new_number(a); // a可能是带前置负号的，直接通过非负数新建一个节点
            x = expr->children->base[bIndex];
            int bIsNegative = bIsNumber && b < 0. || !bIsNumber && x->symbol == RULE_SIGNED_EXPR;
            ParseTree positiveB = x;
            if (bIsNegative)
            {
                if (bIsNumber && b < 0.)
                    positiveB = parse_tree_new_number(-b);
                else
                    positiveB = parse_tree_add_sign(x);
            }
            else if (bIsNumber)
                positiveB = parse_tree_new_number(b); // 同上
            x = NULL;
            if (expr->symbol == RULE_ADD_EXPR)
            {
                // x ± 0 = x, 0 + x = x, 0 - x = -x
                if (aIsNumber && a == 0.)
                {
                    x = expr->children->base[bIndex];
                    if (op == TOKEN_OPERATOR_MINUS)
                        x = parse_tree_add_sign(x);
                }
                else if (bIsNumber && b == 0.)
                    x = expr->children->base[0];
                if (x)
                    return x;
                // 为了简化式子（少点括号），我们不希望 a ± b 中 a或者b为负数
                if (op == TOKEN_OPERATOR_MINUS)
                    bIsNegative = !bIsNegative; // 减号会改变 b 的正负，但不会影响之前求得的positiveB
                ParseTree opTree = expr->children->base[1];
                expr->children->base[0] = positiveA;
                expr->children->base[2] = positiveB;
                if (aIsNegative && bIsNegative)
                {
                    opTree->symbol = TOKEN_OPERATOR_PLUS;
                    return parse_tree_add_sign(expr);
                }
                if (aIsNegative)
                {
                    // 对换 a b
                    expr->children->base[0] = positiveB;
                    expr->children->base[2] = positiveA;
                    opTree->symbol = TOKEN_OPERATOR_MINUS;
                    return expr;
                }
                if (bIsNegative)
                {
                    opTree->symbol = TOKEN_OPERATOR_MINUS;
                    return expr;
                }
            }
            else if (expr->symbol == RULE_MUL_EXPR)
            {
                /*
                x * 0 = 0, 0 * x = 0
                x / 0 = INF, 0 / x = 0 (不考虑x为0),
                x * 1 = x, 1 * x = x, x * (-1) = -x, (-1) * x = -x 
                x / 1 = x, x / (-1) = -x
                */
                int sign = 1;
                if (aIsNumber && a < 0.)
                {
                    sign = -1;
                    a = -a;
                }
                else if (bIsNumber && b < 0.)
                {
                    sign = -1;
                    b = -b;
                }
                if (op == TOKEN_OPERATOR_MULTIPLY)
                {
                    if (aIsNumber && a == 0. || bIsNumber && b == 0.)
                        return parse_tree_new_number(0.);
                    if (aIsNumber && 1. - 1e-6 < a && a < 1. + 1e-6) 
                        x = expr->children->base[bIndex];
                    else if (bIsNumber && 1. - 1e-6 < b && b < 1. + 1e-6)
                        x = expr->children->base[0];
                    if (x)
                    {
                        if (sign < 0)
                            x = parse_tree_add_sign(x);
                        return x;
                    }
                }
                else
                {
                    double c = 100.;
                    if (aIsNumber && a == 0.)
                        c = 0.;
                    else if (bIsNumber && b == 0.)
                        c = INFINITY;
                    if (c != 100.)
                        return parse_tree_new_number(c);
                    if (bIsNumber && 1. - 1e-6 < b && b < 1. + 1e-6)
                    {
                        x = expr->children->base[0];
                        if (sign < 0)
                            x = parse_tree_add_sign(x);
                        return x;
                    }
                }
                // 为了简化式子（少点括号），我们不希望 a */ b 中 a或者b带负号
                expr->children->base[0] = positiveA;
                expr->children->base[2] = positiveB;
                if (aIsNegative ^ bIsNegative)
                {
                    // a b异号，我们需要添加一个前置负号
                    return parse_tree_add_sign(expr);
                }
            }
            else
            {
                // 0 ^ x = 0, x ^ 0 = 1 
                // 1 ^ x = 1, x ^ 1 = x (不考虑x为0)
                double c = 100.;
                if (aIsNumber)
                {
                    if (a == 0.) c = 0.;
                    else if (a == 1.) c = 1.;
                }
                else if (bIsNumber)
                {
                    if (b == 0.) c = 1.;
                    else if(b == 1.) x = expr->children->base[0];
                }
                if (c != 100.)
                    return parse_tree_new_number(c);
                if (x)
                    return x;
            }
        }
        break;
    case RULE_FUNC:
        {
            ParseTree id = expr->children->base[0];
            ParseTree args = expr->children->base[1];
            SupportedFunction *f = id->extraInfo;
            if (*f == FUNC_DIFF)
            {
                double level;
                if (!expr_get_number(args->children->base[2], &level))
                {
                    sprintf(evaluatorError, "求导阶数必须是一个常数");
                    break;
                }
                double lv = round(level);
                if (lv != level || lv < 0.)
                {
                    sprintf(evaluatorError, "求导阶数必须 ≥ 0 且为整数");
                    break;
                }
                ParseTree x = args->children->base[1];
                if (x->symbol != TOKEN_IDENTIFIER)
                {
                    sprintf(evaluatorError, "求导元应是一个未知数");
                    break;
                }
                if (lv == 0.)
                    expr = args->children->base[0];
                else
                    expr = evaluator_simplify(diff(args->children->base[0], x->extraInfo, (int)lv));
            }
            else 
            {
                double a;
                if (expr_get_number(args, &a))
                {
                    switch (*f)
                    {
                    case FUNC_SIN: a = sin(a); break;
                    case FUNC_COS: a = cos(a); break;
                    case FUNC_TAN: a = tan(a); break;
                    case FUNC_ASIN: a = asin(a); break;
                    case FUNC_ACOS: a = acos(a); break;
                    case FUNC_ATAN: a = atan(a); break;
                    case FUNC_LN: a = log(a); break;
                    case FUNC_ABS: a = abs(a); break;
                    case FUNC_SQRT: a = sqrt(a); break;
                    }
                    expr = parse_tree_new_number(a);
                }
            }
        }
        break;
    case RULE_SIGNED_EXPR:
        {
            double a;
            if (expr_get_number(expr->children->base[0], &a))
                expr = parse_tree_new_number(-a);
        }
        break;
    }
    return expr;
}

ParseTree parse_tree_new_expr(ParseTree a, ParseTree b, ParserSymbol op)
{
    a = copy_parse_tree(a, 1);
    b = copy_parse_tree(b, 1);
    ParseTree expr = new_parse_node(0, 1);
    int childrenSize = 3;
    switch (op)
    {
    case TOKEN_OPERATOR_PLUS:
    case TOKEN_OPERATOR_MINUS:
        expr->symbol = RULE_ADD_EXPR;
        break;
    case TOKEN_OPERATOR_MULTIPLY:
    case TOKEN_OPERATOR_DIVIDE:
        expr->symbol = RULE_MUL_EXPR;
        break;
    case TOKEN_OPERATOR_POW:
        childrenSize = 2;
        expr->symbol = RULE_POW_EXPR;
        break;
    }
    expr->children = malloc(sizeof(Stack));
    stack_init(expr->children, childrenSize, 8);
    stack_push(expr->children, a);
    if (expr->symbol != RULE_POW_EXPR)
    {
        ParseTree opTree = new_parse_node(op, 1);
        stack_push(expr->children, opTree);
    }
    stack_push(expr->children, b);
    return parse_tree_cal_expr(expr);
}

ParseTree parse_tree_new_simple_func(SupportedFunction func, ParseTree arg)
{
    arg = copy_parse_tree(arg, 1);
    ParseTree f = new_parse_node(RULE_FUNC, 1);
    ParseTree id = new_parse_node(RULE_FUNC_NAME, 1);
    id->extraInfo = malloc(sizeof(SupportedFunction));
    *(SupportedFunction*)id->extraInfo = func;
    f->children = malloc(sizeof(Stack));
    stack_init(f->children, 2, 8);
    stack_push(f->children, id);
    stack_push(f->children, arg);
    return f;
}

ParseTree diff(ParseTree f, char *x, int level)
{
    f = copy_parse_tree(f, 1);
    if (level > 1)
    {
        while (level-- >= 1)
            f = diff(f, x, 1);
        return f;
    }
    switch (f->symbol)
    {
    case RULE_ADD_EXPR:
        // (a ± b)' = a' ± b'
        f->children->base[0] = diff(f->children->base[0], x, 1);
        f->children->base[2] = diff(f->children->base[2], x, 1);
        return parse_tree_cal_expr(f);
    case RULE_MUL_EXPR:
        {
            ParseTree a = f->children->base[0];
            ParseTree b = f->children->base[2];
            ParseTree a_ = diff(a, x, 1); // a'
            ParseTree b_ = diff(b, x, 1); // b'
            ParseTree a_b = parse_tree_new_expr(a_, b, TOKEN_OPERATOR_MULTIPLY); // a'b
            ParseTree ab_ = parse_tree_new_expr(a, b_, TOKEN_OPERATOR_MULTIPLY); // ab'
            ParseTree s = NULL;
            ParseTree op = f->children->base[1];
            if (op->symbol == TOKEN_OPERATOR_MULTIPLY)
            {
                // (a * b)' = a'b + ab'
                s = parse_tree_new_expr(a_b, ab_, TOKEN_OPERATOR_PLUS); // a'b + ab'
            }
            else
            {
                // (a / b)' = (a'b - ab') / b ^ 2
                s = parse_tree_new_expr(
                    parse_tree_new_expr(a_b, ab_, TOKEN_OPERATOR_MINUS),
                    parse_tree_new_expr(b, parse_tree_new_number(2.), TOKEN_OPERATOR_POW),
                    TOKEN_OPERATOR_DIVIDE
                );
            }
            return s;
        }
    case RULE_POW_EXPR:
        {
            // (a ^ b)' = b * a^(b - 1) * a', b为常数
            // (a ^ b)' = a ^ b * (a'b / a + ln(a) * b')
            ParseTree a = f->children->base[0];
            ParseTree b = f->children->base[1];
            ParseTree a_ = diff(a, x, 1); // a'
            double c;
            if (expr_get_number(b, &c))
            {
                return parse_tree_new_expr(
                    parse_tree_new_expr(
                        b,
                        a_,
                        TOKEN_OPERATOR_MULTIPLY
                    ),
                    parse_tree_new_expr(
                        a,
                        parse_tree_new_number(c - 1.),
                        TOKEN_OPERATOR_POW
                    ),
                    TOKEN_OPERATOR_MULTIPLY
                );
            }
            ParseTree b_ = diff(b, x, 1); // b'
            ParseTree a_b = parse_tree_new_expr(a_, b, TOKEN_OPERATOR_MULTIPLY);
            ParseTree ln_a = parse_tree_new_simple_func(FUNC_LN, a);
            return parse_tree_new_expr(
                f,
                parse_tree_new_expr(
                    parse_tree_new_expr(a_b, a, TOKEN_OPERATOR_DIVIDE),
                    parse_tree_new_expr(ln_a, b_, TOKEN_OPERATOR_MULTIPLY),
                    TOKEN_OPERATOR_PLUS
                ),
                TOKEN_OPERATOR_MULTIPLY
            );
        }
    case RULE_FUNC:
        {
            ParseTree id = f->children->base[0];
            ParseTree arg = f->children->base[1];
            SupportedFunction *func = id->extraInfo;
            assert(*func != FUNC_DIFF); // 化简后的表达式不含求导函数
            ParseTree a_ = diff(arg, x, 1);
            ParseTree g = NULL;
            switch (*func)
            {
            case FUNC_SIN: 
                // sin'(a) = cos(a) * a'
                g = parse_tree_new_simple_func(FUNC_COS, arg);
                break;
            case FUNC_COS:
                // cos'(a) = -sin(a) * a'
                g = parse_tree_new_simple_func(FUNC_SIN, arg);
                g = parse_tree_add_sign(g);
                break;
            case FUNC_TAN:
                // tan'(a) = sec^2(a) * a'
                g = parse_tree_new_expr(
                    parse_tree_new_number(1.),
                    parse_tree_new_expr(
                        parse_tree_new_simple_func(FUNC_COS, arg),
                        parse_tree_new_number(2.),
                        TOKEN_OPERATOR_POW
                    ),
                    TOKEN_OPERATOR_DIVIDE
                );
                break;
            case FUNC_ASIN: 
            case FUNC_ACOS:
                // asin'(a) = 1 / sqrt(1 - a^2) * a'
                // acos'(a) = -1 / sqrt(1 - a^2) * a'
                g = parse_tree_new_expr(
                    parse_tree_new_number(*func == FUNC_ASIN ? 1. : -1.),
                    parse_tree_new_simple_func(
                        FUNC_SQRT,
                        parse_tree_new_expr(
                            parse_tree_new_number(1.),
                            parse_tree_new_expr(
                                arg,
                                parse_tree_new_number(2.),
                                TOKEN_OPERATOR_POW
                            ),
                            TOKEN_OPERATOR_MINUS
                        )
                    ),
                    TOKEN_OPERATOR_DIVIDE
                );
                break;
            case FUNC_ATAN:
                // atan'(a) = 1 / (1 + a^2) * a'
                g = parse_tree_new_expr(
                    parse_tree_new_number(1.),
                    parse_tree_new_expr(
                        parse_tree_new_number(1.),
                        parse_tree_new_expr(
                            arg,
                            parse_tree_new_number(2.),
                            TOKEN_OPERATOR_POW
                        ),
                        TOKEN_OPERATOR_PLUS
                    ),
                    TOKEN_OPERATOR_DIVIDE
                );
                break;
            case FUNC_LN:
                // ln'(a) = 1 / a * a'
                g = parse_tree_new_expr(
                    parse_tree_new_number(1.),
                    arg,
                    TOKEN_OPERATOR_DIVIDE
                );
                break;
            case FUNC_ABS:
                sprintf(evaluatorError, "绝对值函数abs无法求导");
                return f;
            }
            return parse_tree_new_expr(g, a_, TOKEN_OPERATOR_MULTIPLY);
        }
    case RULE_SIGNED_EXPR:
        // (-x)' = -x'
        return parse_tree_add_sign(diff(f->children->base[0], x, 1));
    case TOKEN_NUMBER:
        return parse_tree_new_number(0.);
    case TOKEN_IDENTIFIER:
        if (strcmp(x, f->extraInfo) == 0)
            return parse_tree_new_number(1.);
        return parse_tree_new_number(0.);
    }
    return parse_tree_new_number(0.);
}

ParseTree evaluator_simplify(ParseTree ast)
{
    if (!ast) return NULL;
    if (ast->symbol == TOKEN_IDENTIFIER)
    {
        int size = ast->end - ast->start;
        char *id = ast->extraInfo;
        if (_hash_map_contains(&tmpIdentifierMap, id, size))
            return _hash_map_get(&tmpIdentifierMap, id, size);
        if (!_hash_map_contains(&identifierMap, id, size))
            return ast; // 未定义，已经是最简形式
        ast = _hash_map_get(&identifierMap, id, size);
        ast = evaluator_simplify(copy_parse_tree(ast, 1));
        if (*evaluatorError) return NULL;
        _hash_map_put(&tmpIdentifierMap, id, size, ast);
        return ast;
    }
    if (ast->children)
    {
        stack_for_each(ast->children)
        {
            *p = evaluator_simplify(*p);
            if (*evaluatorError) return NULL;
        }
    }    
    if (ast->symbol == RULE_ADD_EXPR)
    {
        // 加减法化简
        ParseTree x1, x2, c1, c2;
        x1 = expr_get_x(ast->children->base[0]);
        c1 = expr_get_c(ast->children->base[0]);
        x2 = expr_get_x(ast->children->base[2]);
        c2 = expr_get_c(ast->children->base[2]);
        ParseTree op = ast->children->base[1];
        if (!x1) x1 = parse_tree_new_number(0.);
        if (!x2) x2 = parse_tree_new_number(0.);
        if (!c1) c1 = parse_tree_new_number(0.);
        if (!c2) c2 = parse_tree_new_number(0.);
        ast = parse_tree_new_expr(
            parse_tree_new_expr(x1, x2, op->symbol),
            parse_tree_new_expr(c1, c2, op->symbol),
            TOKEN_OPERATOR_PLUS
        );
        return ast;
    }
    ast = parse_tree_cal_expr(ast);
    if (*evaluatorError) 
        return NULL;
    return ast;
}

void expr_add_dependency(ParseTree expr, HashSet *set)
{
    if (!expr) return;
    if (expr->symbol == TOKEN_IDENTIFIER)
    {
        int size = expr->end - expr->start;
        _hash_set_add(set, expr->extraInfo, size);
    }
    if (expr->children)
    {
        stack_for_each(expr->children)
            expr_add_dependency(*p, set);
    }
}

void expr_check_deep_dependency(HashMapItem *item, void *arg)
{
    if (*evaluatorError) return;
    HashSet *allDependencies = arg;
    if (_hash_set_contains(allDependencies, item->key, item->size))
    {
        sprintf(evaluatorError, "标识符出现循环依赖");
        return;
    }
    _hash_set_add(allDependencies, item->key, item->size);
    HashSet *deepDependency = _hash_map_get(&identifierDependencyMap, item->key, item->size);
    if (deepDependency)
        hash_map_each_item(deepDependency, expr_check_deep_dependency, allDependencies);
}

void expr_check_dependency(HashSet *initDependency)
{
    HashSet allDependencies;
    hash_set_init(&allDependencies);
    hash_map_each_item(initDependency, expr_check_deep_dependency, &allDependencies);
    hash_set_clear(&allDependencies);
}

void identifier_remove_dependency(char *id, int size)
{
    HashSet *dependency = _hash_map_get(&identifierDependencyMap, id, size);
    if (dependency)
    {
        _hash_map_remove(&identifierDependencyMap, id, size);
        hash_set_clear(dependency);
        free(dependency);
    }
}

HashSet *identifier_add_dependency(char *id, int size)
{
    identifier_remove_dependency(id, size);
    HashSet *dependency = malloc(sizeof(HashSet));
    hash_set_init(dependency);
    _hash_map_put(&identifierDependencyMap, id, size, dependency);
    return dependency;
}

ParseTree evaluator_handle_expr(ParseTree t)
{
    if (!t) return NULL;
    if (t->symbol == RULE_ASSIGN_EXPR)
    {
        ParseTree id = t->children->base[0];
        ParseTree expr = evaluator_handle_expr(t->children->base[1]);
        if (*evaluatorError) 
            return NULL;
        char *s = id->extraInfo;
        int size = id->end - id->start;
        ParseTree old = _hash_map_get(&identifierMap, s, size);
        if (old)
        {
            parser_clear_tree(old);
            _hash_map_remove(&identifierMap, s, size);
            identifier_remove_dependency(s, size);
        }
        if (expr->symbol != TOKEN_UNDEFINED)
        {
            HashSet *dependency = identifier_add_dependency(s, size);
            expr_add_dependency(expr, dependency);
            if (_hash_set_contains(dependency, s, size))
                sprintf(evaluatorError, "标识符出现循环依赖");
            if (!*evaluatorError)
                expr_check_dependency(dependency);
            if (*evaluatorError)
            {
                identifier_remove_dependency(s, size);
                return NULL;
            }
            _hash_map_put(&identifierMap, s, size, copy_parse_tree(expr, 0));
            return expr;
        }
        return expr;
    }
    return t;
}

ParseTree evaluate(char *s)
{
    *evaluatorError = 0;
    ParseTree parseTree = parser_parse(s);
    if (!parseTree) return NULL;
    ParseTree ast = parser_ast(parseTree);
    evaluator_handle_expr(ast);
    if (*evaluatorError)
    {
        parser_clear_tree(ast);
        goto ERROR;
    }
    if (ast->symbol == RULE_ASSIGN_EXPR)
    {
        parser_clear_tree(ast); // 赋值表达式总是需要回收
        return NULL;
    }
    stack_init(&allocatedParseNode, 1024, 1024);
    ParseTree sim = evaluator_simplify(copy_parse_tree(ast, 1));
    sim = copy_parse_tree(sim, 0);
    parser_clear_tree(ast);
    stack_for_each(&allocatedParseNode)
        parser_clear_node(*p);
    stack_clear(&allocatedParseNode);
    hash_map_clear(&tmpIdentifierMap);
    if (*evaluatorError) goto ERROR;
    return sim;

ERROR:
    // printf("[错误] %s\n", evaluatorError);
    return NULL;
}

char *evaluator_error()
{
    return evaluatorError;
}