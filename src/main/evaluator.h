#if !defined(EVALUATOR_H)
#define EVALUATOR_H

#include "parser.h"

ParseTree evaluate(char *s);
char *evaluator_error();

#endif // EVALUATOR_H
