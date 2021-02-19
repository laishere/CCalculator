#if !defined(TEST_H)
#define TEST_H

#include <stdio.h>

#define MAX_TEST_NAME_LENGTH 40

void printTestName(char *s);

#define TEST(NAME, FUNC) \
    printTestName(NAME); \
    FUNC;        \
    printf("[ͨ��]\n")

#define TEST_START(NAME) printTestName(NAME)

#define TEST_END() printf("[ͨ��]\n")

#endif // TEST_H
