#include "grammar.h"
#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "stack.h"
#include "set.h"
#include "queue.h"

struct GrammarBuildContext
{
    ParserSymbol currentRuleContext;
    HashMap ruleMap;
    HashMap basicFollowsMap, followsMap, tailParentsMap;
    HashMap keyRuleStateMap;
    Stack stateStack;
    Stack allocatedMem, simpleClearStack, simpleClearMap;
} GRAMMAR_CONTEXT;

HashMap *TARGET_STATE_MAP = NULL;

typedef struct RuleState
{
    ParserSymbol ruleContext;
    Stack *rule;
    int step;
} RuleState;

typedef struct KeyRuleState
{
    int keySize;
    RuleState *ruleStates;
} KeyRuleState;

typedef struct SRActionExtraInfo
{
    RuleState *lastRuleState;
    Stack *lastRuleStates;
} SRActionExtraInfo;


void build_grammar_init_context()
{   
    GRAMMAR_CONTEXT.currentRuleContext = 0;
    hash_map_init(&GRAMMAR_CONTEXT.ruleMap);
    hash_map_init(&GRAMMAR_CONTEXT.basicFollowsMap);
    hash_map_init(&GRAMMAR_CONTEXT.followsMap);
    hash_map_init(&GRAMMAR_CONTEXT.tailParentsMap);
    hash_map_init(&GRAMMAR_CONTEXT.keyRuleStateMap);
    // 为了内存管理更清晰，我们用一些辅助stack存储动态分配的内存、需要clear的Stack和Map
    stack_init(&GRAMMAR_CONTEXT.allocatedMem, 64, 64); // 存储动态分配的指针
    stack_init(&GRAMMAR_CONTEXT.simpleClearStack, 64, 64); // 存储需要统一clear的Stack
    stack_init(&GRAMMAR_CONTEXT.simpleClearMap, 64, 64); // 存储需要统一clear的HashMap或者HashSet
}

void build_grammar_free_context()
{
    hash_map_clear(&GRAMMAR_CONTEXT.ruleMap);
    hash_map_clear(&GRAMMAR_CONTEXT.basicFollowsMap);
    hash_map_clear(&GRAMMAR_CONTEXT.followsMap);
    hash_map_clear(&GRAMMAR_CONTEXT.tailParentsMap);
    hash_map_clear(&GRAMMAR_CONTEXT.keyRuleStateMap);
    stack_for_each(&GRAMMAR_CONTEXT.simpleClearStack)
        stack_clear(*p);
    stack_for_each(&GRAMMAR_CONTEXT.simpleClearMap)
        hash_map_clear(*p);
    stack_for_each(&GRAMMAR_CONTEXT.allocatedMem)
        free(*p);
    stack_clear(&GRAMMAR_CONTEXT.allocatedMem);
    stack_clear(&GRAMMAR_CONTEXT.simpleClearStack);
    stack_clear(&GRAMMAR_CONTEXT.simpleClearMap);
}

#define build_grammar_add_allocated_mem(P) stack_push(&GRAMMAR_CONTEXT.allocatedMem, P)

void build_grammar_add_allocated_stack(void *stack)
{
    build_grammar_add_allocated_mem(stack);
    stack_push(&GRAMMAR_CONTEXT.simpleClearStack, stack);
}

void build_grammar_add_allocated_map(void *map)
{
    build_grammar_add_allocated_mem(map);
    stack_push(&GRAMMAR_CONTEXT.simpleClearMap, map);
}

void build_grammar_switch_rule_context(ParserSymbol ctx)
{
    GRAMMAR_CONTEXT.currentRuleContext = ctx;
}

void _build_grammar_add_rule(int size, ...)
{
    size /= sizeof(ParserSymbol);
    size--;
    HashMap *map = &GRAMMAR_CONTEXT.ruleMap;
    if (!hash_map_contains(map, GRAMMAR_CONTEXT.currentRuleContext)) {
        Stack *rules = malloc(sizeof(Stack));
        build_grammar_add_allocated_stack(rules);
        stack_init(rules, 8, 8);
        hash_map_put(map, GRAMMAR_CONTEXT.currentRuleContext, rules);
    }
    Stack *rule = malloc(sizeof(Stack));
    build_grammar_add_allocated_stack(rule);
    stack_init(rule, 8, 8);
    Stack *rules = hash_map_get(map, GRAMMAR_CONTEXT.currentRuleContext);
    stack_push(rules, rule);
    va_list ap;
    va_start(ap, size);
    ParserSymbol lastSymbol;
    HashMap *basicFollowsMap = &GRAMMAR_CONTEXT.basicFollowsMap;
    HashMap *tailParentsMap = &GRAMMAR_CONTEXT.tailParentsMap;
    for (int i = 0; i < size; i++)
    {
        ParserSymbol *ptr = malloc(sizeof(ParserSymbol)); // 不能放到栈内存里面
        build_grammar_add_allocated_mem(ptr);
        *ptr = va_arg(ap, ParserSymbol);
        stack_push(rule, ptr);
        if (i > 0)
        {
            if (!hash_map_contains(basicFollowsMap, lastSymbol))
            {
                HashSet *symbols = malloc(sizeof(HashSet));
                build_grammar_add_allocated_map(symbols);
                hash_set_init(symbols);
                hash_map_put(basicFollowsMap, lastSymbol, symbols);
            }
            HashSet *symbols = hash_map_get(basicFollowsMap, lastSymbol);
            hash_set_add(symbols, *ptr);
        }
        lastSymbol = *ptr;
    }
    // 记录尾巴parent，为了遍历方便直接用stack存
    if (!hash_map_contains(tailParentsMap, lastSymbol))
    {
        Stack *parents = malloc(sizeof(Stack));
        build_grammar_add_allocated_stack(parents);
        stack_init(parents, 8, 8);
        hash_map_put(tailParentsMap, lastSymbol, parents);
    }
    Stack *parents = hash_map_get(tailParentsMap, lastSymbol);
    ParserSymbol *ctx = malloc(sizeof(ParserSymbol));
    build_grammar_add_allocated_mem(ctx);
    *ctx = GRAMMAR_CONTEXT.currentRuleContext;
    stack_push(parents, ctx);
    va_end(ap);
}

#define build_grammar_add_rule(...) _build_grammar_add_rule(sizeof((ParserSymbol[]){0, ##__VA_ARGS__}), ##__VA_ARGS__)

void add_subrules_to_follows_set(HashSet *set, ParserSymbol ctx)
{
    Stack *rules = hash_map_get(&GRAMMAR_CONTEXT.ruleMap, ctx);
    if (!rules) return;
    stack_for_each(rules)
    {
        Stack *rule = *p;
        ParserSymbol firstSymbol = *(ParserSymbol *)rule->base[0];
        if (!hash_set_contains(set, firstSymbol))
        {
            hash_set_add(set, firstSymbol);
            add_subrules_to_follows_set(set, firstSymbol);
        }
    }
}

void extend_symbols_in_follows_set(HashMapItem *item, void *arg)
{
    HashSet *set = arg;
    ParserSymbol s = *(ParserSymbol *)item->key;
    add_subrules_to_follows_set(set, s);
}

void add_each_item_to_set(HashMapItem *item, void *arg)
{
    HashSet *set = arg;
    _hash_set_add(set, item->key, item->size);
}

void build_grammar_complete_symbol_follows_set(ParserSymbol s)
{
    /*
    完善 S 的连接集 F(S)，如果存在一种序列 .. S U .. 符合规则，那么 U ∈ F(S)，
    也就是说， F(S) 包含所有可以跟在 S 后面的 symbol 
    1.假如存在规则
        R -> .. S U ..
        那么 U ∈ BF(S)，BF为 S 的基本连接集，也就是直接在规则中出现的连接关系，且 F(S) 包含 BF(S)
    2.假如存在规则
        R -> .. S
        那么 F(R) 包含于 F(S)，因为如果存在 .. R U .. 的合法序列，当 R 可以推导为 .. S 时， .. S U .. 也可以是合法序列
    3.假如 U ∈ F(S) 且存在规则
        U -> X ..
        那么 X ∈ F(S)，因为存在 .. S U .. 的合法序列，当 U 可以推导为 X ..时， .. S X .. 也可以是合法序列
    */
    HashMap *followsMap = &GRAMMAR_CONTEXT.followsMap;
    if (hash_map_contains(followsMap, s))
        return; // 已经完成了
    HashSet *follows = malloc(sizeof(HashSet));
    hash_set_init(follows);
    build_grammar_add_allocated_map(follows);
    hash_map_put(followsMap, s, follows);
    
    // 1.建立基本连接集的操作已经在语法规则定义时已存到basicFollowsMap里面，如果存在bf直接复制过来
    HashSet *bf = hash_map_get(&GRAMMAR_CONTEXT.basicFollowsMap, s);
    if (bf)
        hash_map_each_item(bf, add_each_item_to_set, follows);
    
    // 2.我们通过一个Stack记录所有符合条件的R，遍历获取闭包后加到当前连接集
    Stack *parents = hash_map_get(&GRAMMAR_CONTEXT.tailParentsMap, s);
    if (parents)
    {
        stack_for_each(parents)
        {
            ParserSymbol *s = *p;
            build_grammar_complete_symbol_follows_set(*s);
            HashSet *f = hash_map_get(followsMap, *s);
            hash_map_each_item(f, add_each_item_to_set, follows);
        }
    }

    HashSet tmpSet;
    hash_set_init(&tmpSet);
    // 3.我们需要扩展连接集
    hash_map_each_item(follows, extend_symbols_in_follows_set, &tmpSet);
    hash_map_each_item(&tmpSet, add_each_item_to_set, follows);
    hash_set_clear(&tmpSet);
}

void each_item_complete_follows_set(HashMapItem *item, void *arg)
{
    build_grammar_complete_symbol_follows_set(*(ParserSymbol *)item->key);
}

/**
 * 构建语法树
 */ 
HashMap *build_grammar_tree()
{    
    /*
    目标
        : 表达式 EOF
    */
    build_grammar_switch_rule_context(RULE_TARGET);
    build_grammar_add_rule(RULE_EXPR, TOKEN_EOF);

    /*
    表达式
        : 加减法表达式
        | 赋值表达式
    */
    build_grammar_switch_rule_context(RULE_EXPR);
    build_grammar_add_rule(RULE_ADD_EXPR);
    build_grammar_add_rule(RULE_ASSIGN_EXPR);

    /*
    赋值表达式
        : 标识符 = 表达式
        | 标识符 = ?
    */
    build_grammar_switch_rule_context(RULE_ASSIGN_EXPR);
    build_grammar_add_rule(TOKEN_IDENTIFIER, TOKEN_ASSIGN, RULE_EXPR);
    build_grammar_add_rule(TOKEN_IDENTIFIER, TOKEN_ASSIGN, TOKEN_UNDEFINED);

    /*
    无外围括号的值
        : 数字
        | 标识符
        | 函数
        | 后缀表达式
    */
    build_grammar_switch_rule_context(RULE_NO_BRACKETS_VALUE);
    build_grammar_add_rule(TOKEN_NUMBER);
    build_grammar_add_rule(TOKEN_IDENTIFIER);
    build_grammar_add_rule(RULE_FUNC);
    build_grammar_add_rule(RULE_REVESE_EXPR);

    /*
    值
        : 无外围括号的值
        | ( 加减法表达式 )
    */
    build_grammar_switch_rule_context(RULE_VALUE);
    build_grammar_add_rule(RULE_NO_BRACKETS_VALUE);
    build_grammar_add_rule(TOKEN_LEFT_BRACKET, RULE_ADD_EXPR, TOKEN_RIGHT_BRACKET);

    /*
    运算符
        : +
        | -
        | *
        | /
        | ^
    */
    build_grammar_switch_rule_context(RULE_OPERATOR);
    build_grammar_add_rule(TOKEN_OPERATOR_PLUS);
    build_grammar_add_rule(TOKEN_OPERATOR_MINUS);
    build_grammar_add_rule(TOKEN_OPERATOR_MULTIPLY);
    build_grammar_add_rule(TOKEN_OPERATOR_DIVIDE);
    build_grammar_add_rule(TOKEN_OPERATOR_POW);

    /*
    后缀表达式
        : 无外围括号的值 无外围括号的值 运算符
    */
    build_grammar_switch_rule_context(RULE_REVESE_EXPR);
    build_grammar_add_rule(RULE_NO_BRACKETS_VALUE, RULE_NO_BRACKETS_VALUE, RULE_OPERATOR);

    /*
    参数
        : 表达式
        | 参数 , 表达式
    */
    build_grammar_switch_rule_context(RULE_ARG);
    build_grammar_add_rule(RULE_EXPR);
    build_grammar_add_rule(RULE_ARG, TOKEN_COMMA, RULE_EXPR);

    /*
    函数
        : 标识符 ( 参数 )
    */
    build_grammar_switch_rule_context(RULE_FUNC);
    build_grammar_add_rule(TOKEN_IDENTIFIER, TOKEN_LEFT_BRACKET, RULE_ARG, TOKEN_RIGHT_BRACKET);

    /*
    幂运算表达式
        : 值
        | 幂运算表达式 ^ 值
    */
    build_grammar_switch_rule_context(RULE_POW_EXPR);
    build_grammar_add_rule(RULE_VALUE);
    build_grammar_add_rule(RULE_POW_EXPR, TOKEN_OPERATOR_POW, RULE_VALUE);

    /*
    乘除法表达式
        : 幂运算表达式
        | 乘除法表达式 * 幂运算表达式
        | 乘除法表达式 / 幂运算表达式
    */
    build_grammar_switch_rule_context(RULE_MUL_EXPR);
    build_grammar_add_rule(RULE_POW_EXPR);
    build_grammar_add_rule(RULE_MUL_EXPR, TOKEN_OPERATOR_MULTIPLY, RULE_POW_EXPR);
    build_grammar_add_rule(RULE_MUL_EXPR, TOKEN_OPERATOR_DIVIDE, RULE_POW_EXPR);

    /*
    前置负号表达式
        : - 乘除法表达式
    */
    build_grammar_switch_rule_context(RULE_SIGNED_EXPR);
    build_grammar_add_rule(TOKEN_OPERATOR_MINUS, RULE_MUL_EXPR);

    /*
    加减法表达式
        : 乘除法表达式
        | 前置负号表达式
        | 加减法表达式 + 乘除法表达式
        | 加减法表达式 - 乘除法表达式
    */
    build_grammar_switch_rule_context(RULE_ADD_EXPR);
    build_grammar_add_rule(RULE_MUL_EXPR);
    build_grammar_add_rule(RULE_SIGNED_EXPR);
    build_grammar_add_rule(RULE_ADD_EXPR, TOKEN_OPERATOR_PLUS, RULE_MUL_EXPR);
    build_grammar_add_rule(RULE_ADD_EXPR, TOKEN_OPERATOR_MINUS, RULE_MUL_EXPR);

    // 在规则定义结束后完善 连接集 的闭包
    hash_map_each_item(&GRAMMAR_CONTEXT.basicFollowsMap, each_item_complete_follows_set, NULL);
    hash_map_each_item(&GRAMMAR_CONTEXT.tailParentsMap, each_item_complete_follows_set, NULL);

    return &GRAMMAR_CONTEXT.ruleMap;
}

void build_grammar_print_rule_state(RuleState *s)
{
    printf("%s ->", parser_get_symbol_name(s->ruleContext));
    void **next = s->rule->base + s->step;
    stack_for_each(s->rule)
    {
        if (p == next)
            printf(" .%s", parser_get_symbol_name(*(ParserSymbol *)*p));
        else
            printf(" %s", parser_get_symbol_name(*(ParserSymbol *)*p));
    }
    if (next == s->rule->top)
        printf(".");
}

void build_grammar_print_action_rule_state(SRAction *a)
{
    if (a->action == ACTION_SHIFT)
        printf("[shift %s]: ", parser_get_symbol_name(a->shiftSymbol));
    else if (a->action == ACTION_ACCEPT)
        printf("[accept]: ");
    else
        printf("[reduce]: ");
    SRActionExtraInfo *info = a->extraInfo;
    if (info->lastRuleStates)
    {
        stack_for_each(info->lastRuleStates)
            build_grammar_print_rule_state(*p);
    }
    else 
        build_grammar_print_rule_state(info->lastRuleState);        
    printf("\n");
}

void build_grammar_show_action_conflicts(SRAction *a, SRAction *b)
{
    // 有两种冲突 shift/reduce 和 reduce/reduce
    char *s;
    if (a->action == ACTION_SHIFT || b->action == ACTION_SHIFT)
        s = "shift/reduce 冲突";
    else
        s = "reduce/reduce 冲突";
    printf("%s:\n", s);
    build_grammar_print_action_rule_state(a);
    build_grammar_print_action_rule_state(b);
    printf("\n");
    exit(1);
}

SRActionExtraInfo *build_grammar_new_action_extra_info(RuleState *lastRuleState, Stack *lastRuleStates)
{
    SRActionExtraInfo *info = malloc(sizeof(SRActionExtraInfo));
    build_grammar_add_allocated_mem(info);
    info->lastRuleState = lastRuleState;
    info->lastRuleStates = lastRuleStates;
    return info;
}

typedef struct 
{
    HashMap *stateMap;
    SRAction *action;
} EachItemAddReduceActionArg;

void each_item_add_reduce_action(HashMapItem *item, void *arg)
{
    EachItemAddReduceActionArg *arg2 = arg;
    ParserSymbol s = *(ParserSymbol *)item->key;
    SRAction *existedAction = hash_map_get(arg2->stateMap, s);
    if (existedAction)
        build_grammar_show_action_conflicts(existedAction, arg2->action);
    else
        hash_map_put(arg2->stateMap, s, arg2->action);
}

HashMap *build_grammar_walk_state(KeyRuleState *keyRuleState);

void each_item_add_shift_action(HashMapItem *item, void *arg)
{
    HashMap *stateMap = arg;
    ParserSymbol s = *(ParserSymbol *)item->key;
    Stack *states = item->value;
    SRAction *existedAction = hash_map_get(stateMap, s);
    SRAction *action = malloc(sizeof(SRAction)); // 此内存只分配一次且常驻程序不需要主动释放
    action->action = ACTION_SHIFT;
    action->shiftSymbol = s;
    action->extraInfo = build_grammar_new_action_extra_info(NULL, states);
    if (existedAction)
        build_grammar_show_action_conflicts(existedAction, action);
    KeyRuleState keyRuleState;
    keyRuleState.keySize = stack_size(states);
    RuleState *arr = malloc(sizeof(RuleState) * keyRuleState.keySize);
    build_grammar_add_allocated_mem(arr);
    keyRuleState.ruleStates = arr;
    for (int i = 0; i < keyRuleState.keySize; i++)
    {
        arr[i] = *(RuleState *)states->base[i];
        arr[i].step++;
    }
    if (keyRuleState.keySize == 1 
        && arr[0].ruleContext == RULE_TARGET 
        && arr[0].step == stack_size(arr[0].rule))
    {
        // 目标规则且是最后一个symbol，accept
        action->action = ACTION_ACCEPT;
        action->reduceTo = RULE_TARGET;
        action->reduceNodeSize = stack_size(arr[0].rule);
    }
    else
        action->shiftTo = build_grammar_walk_state(&keyRuleState);
    hash_map_put(stateMap, s, action);
}

void test_print_state_map_each_item(HashMapItem *item, void *arg)
{
    ParserSymbol *s = item->key;
    SRAction *a = item->value;
    printf("%s: ", parser_get_symbol_name(*s));
    if (a->action == ACTION_SHIFT)
        printf("shift to %p", a->shiftTo);
    else if (a->action == ACTION_ACCEPT)
        printf("accept");
    else
        printf("reduce %d nodes to %s", a->reduceNodeSize, parser_get_symbol_name(a->reduceTo));
    printf("\n");
}

void test_print_key_rule_state(KeyRuleState *keyRuleState)
{
    printf("KEY RULES: \n");
    for (int i = 0; i < keyRuleState->keySize; i++)
    {
        build_grammar_print_rule_state(keyRuleState->ruleStates + i);
        printf("\n");
    }
}

void test_print_state_map(HashMap *stateMap)
{
    printf("STATE %p\n", stateMap);
    printf("--------------------------------\n");
    hash_map_each_item(stateMap, test_print_state_map_each_item, NULL);
    printf("--------------------------------\n\n");
}

/**
 * 根据keyRuleState获取闭包，递归遍历构建所有state
 */ 
HashMap *build_grammar_walk_state(KeyRuleState *keyRuleState)
{
    int keySize = keyRuleState->keySize;
    if (_hash_map_contains(&GRAMMAR_CONTEXT.keyRuleStateMap, keyRuleState->ruleStates, keySize * sizeof(RuleState)))
        return _hash_map_get(&GRAMMAR_CONTEXT.keyRuleStateMap, keyRuleState->ruleStates, keySize * sizeof(RuleState));
    HashSet extendedRules;
    HashMap *stateMap = malloc(sizeof(HashMap)); // 此内存只分配一次且常驻程序不需要主动释放
    _hash_map_put(&GRAMMAR_CONTEXT.keyRuleStateMap, keyRuleState->ruleStates, keySize * sizeof(RuleState), stateMap);
    HashMap keyStateMap;
    Queue queue;
    hash_set_init(&extendedRules);
    hash_map_init(stateMap);
    hash_map_init(&keyStateMap);
    queue_init(&queue, 8, 8);
    for (int i = 0; i < keySize; i++)
    {
        RuleState *s = keyRuleState->ruleStates + i;
        queue_add(&queue, s);
    }
    while (queue.size > 0)
    {
        RuleState *s = queue_pull(&queue);
        ParserSymbol ctx = s->ruleContext;
        int ruleSize = stack_size(s->rule);
        // 处理当前RuleState，我们有两个方向
        // 1、往前走，即RuleState的step + 1，此时会有两种情况，一是reduce，二是shift/accept
        // 2、往里走，即扩展下一个symbol，比如：S -> .P + V ， 我们需要扩展 P 来逐步完善此状态的闭包
        if (s->step == ruleSize)
        {
            // reduce
            HashSet *follows = hash_map_get(&GRAMMAR_CONTEXT.followsMap, ctx);
            if (follows)
            {
                // follows 存在，说明存在symbol可以接在ctx后面，我们可以reduce到ctx
                SRAction *action = malloc(sizeof(SRAction)); // 此内存只分配一次且常驻程序不需要主动释放
                action->action = ACTION_REDUCE;
                action->reduceNodeSize = ruleSize;
                action->reduceTo = ctx;
                action->extraInfo = build_grammar_new_action_extra_info(s, NULL);
                EachItemAddReduceActionArg arg = {stateMap, action};
                hash_map_each_item(follows, each_item_add_reduce_action, &arg); // 为连接集的所有元素添加reduce操作
            }
            else if (ctx != RULE_TARGET)
            {
                printf("reduce to %s 出错，只有目标规则后面可不接symbol\n", parser_get_symbol_name(ctx));
                exit(1);
            }
            continue;
        }
        
        // shift or accept
        ParserSymbol nextSymbol = *(ParserSymbol *)s->rule->base[s->step];
        if (!hash_map_contains(&keyStateMap, nextSymbol))
        {
            Stack *states = malloc(sizeof(Stack));
            build_grammar_add_allocated_stack(states);
            stack_init(states, 8, 8);
            hash_map_put(&keyStateMap, nextSymbol, states);
        }
        Stack *states = hash_map_get(&keyStateMap, nextSymbol);
        RuleState *nextRuleState = malloc(sizeof(RuleState));
        build_grammar_add_allocated_mem(nextRuleState);
        *nextRuleState = *s;
        stack_push(states, nextRuleState);

        // 扩展nextSymbol
        Stack *rules = hash_map_get(&GRAMMAR_CONTEXT.ruleMap, nextSymbol);
        if (!rules || hash_set_contains(&extendedRules, nextSymbol))
            continue;
        hash_set_add(&extendedRules, nextSymbol);
        stack_for_each(rules)
        {
            RuleState *s = malloc(sizeof(RuleState));
            build_grammar_add_allocated_mem(s);
            s->ruleContext = nextSymbol;
            s->rule = *p;
            s->step = 0;
            queue_add(&queue, s);
        }
    }
    hash_map_each_item(&keyStateMap, each_item_add_shift_action, stateMap);
    
    #ifdef BUILD_GRAMMAR_TEST
    test_print_key_rule_state(keyRuleState);
    test_print_state_map(stateMap);
    #endif
    
    hash_set_clear(&extendedRules);
    hash_map_clear(&keyStateMap);
    queue_clear(&queue);

    return stateMap;
}

HashMap *build_grammar_state_map()
{
    ParserSymbol targetSymbol = RULE_TARGET;
    Stack *targetRules = hash_map_get(&GRAMMAR_CONTEXT.ruleMap, targetSymbol);
    Stack *targetRule = targetRules->base[0];
    RuleState targetRuleState = {targetSymbol, targetRule, 0};
    RuleState ruleStates[] = {targetRuleState};
    KeyRuleState targetKeyRuleState = {1, ruleStates};
    TARGET_STATE_MAP = build_grammar_walk_state(&targetKeyRuleState);
    return TARGET_STATE_MAP;
}

HashMap *build_grammar()
{
    if (TARGET_STATE_MAP) 
        return TARGET_STATE_MAP;
    build_grammar_init_context();
    build_grammar_tree();
    build_grammar_state_map();
    build_grammar_free_context();
    return TARGET_STATE_MAP;
}