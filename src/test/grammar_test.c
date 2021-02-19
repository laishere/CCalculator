#include "test.h"
#include "stack.h"
#include "map.h"
#include "grammar.h"
#include <stdio.h>
#include "set.h"
#include <stdlib.h>

void printHashMapState(HashMap *map);
void build_grammar_init_context();
void build_grammar_free_context();
HashMap *build_grammar_tree();
HashMap *build_grammar_state_map();

void printItem(HashMapItem *item, void *arg)
{
    printf("%s\n", parser_get_symbol_name(*(ParserSymbol *)item->key));    
    Stack *rules = item->value;
    for (void **i = rules->base; i < rules->top; i++)
    {
        if (i == rules->base) printf("\t:");
        else printf("\t|");
        Stack *rule = *i;
        stack_for_each(rule)
        {
            ParserSymbol *s = *p;
            printf(" %s", parser_get_symbol_name(*s));
        }
        printf("\n");
    }
    printf("\n");
}

void grammar_test()
{
    build_grammar_init_context();
    HashMap *ruleMap = build_grammar_tree();
    printf("********构造的语法树********\n\n");
    hash_map_each_item(ruleMap, printItem, NULL);
    printHashMapState(ruleMap);
    HashMap *stateMap = build_grammar_state_map();
    printHashMapState(stateMap);
    build_grammar_free_context();
}