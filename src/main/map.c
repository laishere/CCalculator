#include "map.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

void hash_map_init(HashMap *map)
{
    if (!map) return;
    memset(map, 0, sizeof(HashMap));
}

/**
 * �����򵥵��ڴ�Ƚ�
 */
int _hash_map_item_equal(HashMapItem *a, HashMapItem *b)
{
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->size != b->size) return 0;
    if (a->key == b->key) return 1;
    return memcmp(a->key, b->key, a->size) == 0;
}

int _hash_map_calculate_hash(void *key, int size)
{
    unsigned char *s = key, *e = s + size;
    if (!s) return 0;
    int hash = 7;
    for (; s < e; s++) 
    {
        hash *= 31;
        hash += *s;
    }
    return hash;
}

HashMapItem *_hash_map_new_item(HashMapItem *item, void *key, int size, void *value)
{
    if (item == NULL) 
    {
        item = malloc(sizeof(HashMapItem)); // ���δ����item��̬����
        item->key = malloc(size);
        memcpy(item->key, key, size); // �������ⲿkey����ͬ���ı�
    }
    else item->key = key;
    item->size = size;
    item->hash = _hash_map_calculate_hash(key, size);
    item->next = NULL;
    item->value = value;
    return item;
}

void _hash_map_free_item(HashMapItem *item)
{
    if (!item) return;
    free(item->key);
    free(item);
}

/**
 * ���ز���λ�ã������Ҫ�Ƚϲ���item�Ѵ�����Ϊ�Ѵ��ڵ�item�ĸ��ڵ�
 */ 
HashMapItem *_hash_map_insert_point(HashMapItem *table, int capacity, HashMapItem *item, int needCmp)
{
    HashMapItem *p = ((unsigned int)item->hash % capacity) + table;
    while (p->next)
    {
        if (needCmp && _hash_map_item_equal(p->next, item))
            return p; // �Ѿ�����
        p = p->next;
    }
    return p;
}

/**
 * ���item
 * @return 0 �޲���, 1 ���, 2 �滻
 */  
int _hash_map_add_item(HashMapItem *table, int capacity, HashMapItem *item, int needCmp)
{
    HashMapItem *p = _hash_map_insert_point(table, capacity, item, needCmp);
    int state = 1;
    if (p->next)
    {
        if (p->next->value == item->value)
            return 0; // �Ѵ�����valueһ�£��������
        item->next = p->next->next; // ��Ҫ�Ѻ����ڵ�ӵ��½ڵ����
        _hash_map_free_item(p->next); // value��һ�£���Ҫ�滻���ڴ����ͷ�
        state = 2;
    }
    else item->next = NULL;
    p->next = item;
    return state;
}

int _hash_map_put(HashMap *map, void *key, int size, void *value)
{
    if (!map || !key) return 0;
    assert(!map->iterating && "�����ڱ�����ʱ��ı��ϣ��");
    if (!map->table)
    {
        map->capacity = HASH_MAP_INIT_CAPACITY;
        map->table = malloc(sizeof(HashMapItem) * map->capacity);
        memset(map->table, 0, sizeof(HashMapItem) * map->capacity);
    }
    else if (map->capacity * HASH_MAP_LOAD_FACTOR < map->size + 1)
    {
        // ���ݣ�ת�Ƶ��±�
        int newSize = map->size + 1;
        int newCapacity = (int) (newSize / HASH_MAP_LOAD_FACTOR_AFTER_INCREMENT + 0.5f);
        if (newCapacity < map->capacity + HASH_MAP_MIN_INCREMENT)
            newCapacity = map->capacity + HASH_MAP_MIN_INCREMENT;
        HashMapItem *newTable = malloc(sizeof(HashMapItem) * newCapacity);
        memset(newTable, 0, sizeof(HashMapItem) * newCapacity);
        HashMapItem *s = map->table, *e = s + map->capacity, *n, *tmp;
        for (; s < e; s++)
        {
            n = s->next;
            while (n)
            {
                tmp = n->next; // n��ת�Ƶ��±�� n->next ��ı䣬��Ҫ��ǰ����
                _hash_map_add_item(newTable, newCapacity, n, 0); // ���һ������ needCmp Ϊ 0����ΪҪ�����Ԫ�ز����ܴ������±�
                n = tmp;
            }
        }
        free(map->table); // ֻ��Ҫ�ͷ�table��Ҳ��������ͷ����Ϊ����Ԫ�ض��ѱ����ã������ͷ�
        map->table = newTable;
        map->capacity = newCapacity;
    }
    HashMapItem *item = _hash_map_new_item(NULL, key, size, value); // ��̬����һ���ڵ�
    int state = _hash_map_add_item(map->table, map->capacity, item, 1);
    if (!state) _hash_map_free_item(item);
    else if (state == 1) 
        map->size++;
    return state != 0;    
}

HashMapItem *_hash_map_get_item(HashMap *map, void *key, int size)
{
    if (!map || !map->table || !key) return NULL;
    HashMapItem item;
    _hash_map_new_item(&item, key, size, NULL);
    HashMapItem *p = _hash_map_insert_point(map->table, map->capacity, &item, 1);
    HashMapItem *found = p->next;
    return found;   
}

void *_hash_map_get(HashMap *map, void *key, int size)
{
    HashMapItem *found = _hash_map_get_item(map, key, size);
    if (!found) return NULL;
    return found->value;
}

int _hash_map_remove(HashMap *map, void *key, int size)
{
    if (!map || !map->table || !key) return 0;
    assert(!map->iterating && "�����ڱ�����ʱ��ı��ϣ��");
    HashMapItem item;
    _hash_map_new_item(&item, key, size, NULL);
    HashMapItem *p = _hash_map_insert_point(map->table, map->capacity, &item, 1);
    HashMapItem *found = p->next;
    if (!found)
        return 0; // ������
    p->next = found->next; // ɾ��found
    _hash_map_free_item(found); // �ͷ�found
    map->size--;
    return 1;    
}

int _hash_map_contains(HashMap *map, void *key, int size)
{
    return _hash_map_get_item(map, key, size) != NULL;
}

void hash_map_clear(HashMap *map)
{
    if (!map) return;
    assert(!map->iterating && "�����ڱ�����ʱ��ı��ϣ��");
    HashMapItem *s = map->table, *e = s + map->capacity, *n, *tmp;
    for (; s < e; s++)
    {
        // �ͷ�����ڵ�
        n = s->next;
        while (n)
        {
            tmp = n->next;
            _hash_map_free_item(n);
            n = tmp;
        }
    }
    free(map->table); // �ͷ�����ͷ
    map->table = NULL;
    map->size = 0;
    map->capacity = 0;
}

void hash_map_each_item(HashMap *map, void (*callback)(HashMapItem *, void *), void *arg)
{
    if (!map) return;
    map->iterating = 1;
    HashMapItem *s = map->table, *e = s + map->capacity, *n;
    for (; s < e; s++)
    {
        n = s->next;
        while (n)
        {
            callback(n, arg);
            n = n->next;
        }
    }
    map->iterating = 0;
}