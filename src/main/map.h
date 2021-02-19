#if !defined(MAP_H)
#define MAP_H

#define HASH_MAP_INIT_CAPACITY 16
#define HASH_MAP_MIN_INCREMENT 16
#define HASH_MAP_LOAD_FACTOR 0.75f
#define HASH_MAP_LOAD_FACTOR_AFTER_INCREMENT 0.6f

typedef struct HashMapItem
{
    void *key;
    void *value;
    int hash, size;
    struct HashMapItem *next;
} HashMapItem;

typedef struct HashMap
{
    HashMapItem *table;
    int capacity, size;
    int iterating;
} HashMap;

void hash_map_init(HashMap *map);
void *_hash_map_get(HashMap *map, void *key, int size);
int _hash_map_put(HashMap *map, void *key, int size, void *value);
int _hash_map_remove(HashMap *map, void *key, int size);
int _hash_map_contains(HashMap *map, void *key, int size);
void hash_map_clear(HashMap *map);
void hash_map_each_item(HashMap *map, void (*callback)(HashMapItem *, void *), void *arg);

#define hash_map_get(map, key) _hash_map_get(map, &(key), sizeof(key))
#define hash_map_put(map, key, value) _hash_map_put(map, &(key), sizeof(key), value)
#define hash_map_remove(map, key) _hash_map_remove(map, &(key), sizeof(key))
#define hash_map_contains(map, key) _hash_map_contains(map, &(key), sizeof(key))
#endif // MAP_H
