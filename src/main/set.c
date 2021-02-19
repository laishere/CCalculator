#include "set.h"
#include <stdlib.h>

void hash_set_init(HashSet *set)
{
    hash_map_init(set);
}

int _hash_set_add(HashSet *set, void *key, int size)
{
    return _hash_map_put(set, key, size, NULL);
}

int _hash_set_remove(HashSet *set, void *key, int size)
{
    return _hash_map_remove(set, key, size);
}

int _hash_set_contains(HashSet *set, void *key, int size)
{
    return _hash_map_contains(set, key, size);
}

void hash_set_clear(HashSet *set)
{
    hash_map_clear(set);
}