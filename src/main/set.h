#if !defined(SET_H)
#define SET_H

#include "map.h"

typedef HashMap HashSet;

void hash_set_init(HashSet *set);
int _hash_set_add(HashSet *set, void *key, int size);
int _hash_set_remove(HashSet *set, void *key, int size);
int _hash_set_contains(HashSet *set, void *key, int size);
void hash_set_clear(HashSet *set);

#define hash_set_add(set, key) _hash_set_add(set, &(key), sizeof(key))
#define hash_set_remove(set, key) _hash_set_remove(set, &(key), sizeof(key))
#define hash_set_contains(set, key) _hash_set_contains(set, &(key), sizeof(key))
#define hash_set_each_item(set, callback, arg) hash_map_each_item(set, callback, arg)

#endif // SET_H
