#include "test.h"
#include "set.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define DATA_SIZE 10000

void printHashMapState(HashMap *map);

void hash_set_test()
{
    int *data = malloc(sizeof(int) * DATA_SIZE);
    for (int i = 0; i < DATA_SIZE; i++)
        data[i] = i;
    HashSet set;
    TEST_START("哈希集合初始化测试");
    hash_set_init(&set);
    assert(set.table == NULL);
    assert(set.capacity == 0);
    assert(set.size == 0);
    TEST_END();

    TEST_START("哈希集合大规模重复插入测试");
    for (int t = 0; t < 100; t++)
    {
        for (int i = 0; i < DATA_SIZE; i++)
            hash_set_add(&set, data[i]);
    }
    assert(set.size == DATA_SIZE);
    TEST_END();
    printHashMapState(&set);

    TEST_START("哈希集合查找测试");
    for (int i = 0; i < DATA_SIZE; i++)
        assert(hash_set_contains(&set, data[i]));
    TEST_END();

    TEST_START("哈希集合删除测试");
    int sizeToDelete = DATA_SIZE / 2;
    int sizeBeforeDelete = DATA_SIZE;
    for (int i = 0; i < sizeToDelete; i ++)
        hash_set_remove(&set, data[i]);
    assert(sizeBeforeDelete - sizeToDelete == set.size);
    TEST_END();

    TEST_START("哈希集合清空测试");
    hash_set_clear(&set);
    assert(set.size == 0);
    assert(set.capacity == 0);
    assert(set.table == NULL);
    TEST_END();

    free(data);
}
