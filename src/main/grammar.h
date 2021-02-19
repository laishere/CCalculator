#if !defined(GRAMMAR_H)
#define GRAMMAR_H

#include "parser.h"
#include "map.h"

typedef enum
{
    ACTION_SHIFT, ACTION_REDUCE, ACTION_ACCEPT
} ActionEnum;

typedef struct SRAction
{
    ActionEnum action;
    ParserSymbol shiftSymbol;
    HashMap *shiftTo;
    ParserSymbol reduceTo;
    int reduceNodeSize;
    void *extraInfo;
} SRAction;

HashMap *build_grammar();

#endif // GRAMMAR_H
