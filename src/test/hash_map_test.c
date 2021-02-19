#include "test.h"
#include "map.h"
#include <stdio.h>
#include <assert.h>

void printHashMapState(HashMap *map)
{
    int totalLinkedListLength = 0;
    int maxLinkedListLength = 0;
    HashMapItem *s = map->table, *e = s + map->capacity, *n;
    for (; s < e; s++)
    {
        n = s->next;
        int len = 0;
        while (n)
        {
            n = n->next;
            len++;
        }
        if (len > maxLinkedListLength)
            maxLinkedListLength = len;
        totalLinkedListLength += len;
    }
    printf("\n******** ��ϣ��״̬ ********\n");
    printf("������\t\t%d\n", map->capacity);
    printf("ʵ�ʳ��ȣ�\t%d\n", map->size);
    printf("������ȣ�\t%d\n", maxLinkedListLength);
    printf("�������ȣ�\t%d\n", totalLinkedListLength);
    if (map->capacity != 0)
        printf("ƽ�������ȣ�\t%.2f\n", (float) totalLinkedListLength / map->capacity);
    printf("\n");
}

void hash_map_test()
{
    HashMap map;
    
    TEST_START("��ϣ���ʼ������");
    hash_map_init(&map);
    assert(map.table == NULL);
    assert(map.capacity == 0);
    assert(map.size == 0);
    TEST_END();

    TEST_START("��ϣ�������޸Ĳ���");
    int keyA = 12, keyB = 23;
    int valA = 2344, valB = 23123;
    hash_map_put(&map, *(&keyA), &valA);
    hash_map_put(&map, keyB, &valB);
    assert(map.size == 2);
    assert(valA == *(int *)(hash_map_get(&map, keyA)));
    assert(valB == *(int *)(hash_map_get(&map, keyB)));
    // �޸�
    hash_map_put(&map, keyA, &valB);
    assert(valB == *(int *)(hash_map_get(&map, keyA)));
    assert(map.size == 2);
    TEST_END();

    TEST_START("��ϣ��ɾ������");
    hash_map_remove(&map, keyA);
    assert(map.size == 1);
    assert(!hash_map_contains(&map, keyA));
    TEST_END();

    TEST_START("��ϣ����ղ���");
    hash_map_clear(&map);
    assert(map.table == NULL);
    assert(map.capacity == 0);
    assert(map.size == 0);
    TEST_END();
}