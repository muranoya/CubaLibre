#ifndef HASHMAP_H
#define HASHMAP_H

struct Hashmap;
typedef struct Hashmap Hashmap;

Hashmap *
make_hashmap(int size);

void
free_hashmap(Hashmap *map);

int 
add_hashmap(Hashmap *map, const char *key, void *data);

void *
remove_hashmap(Hashmap *map, const char *key);

int
exists_hashmap(const Hashmap *map, const char *key);

void *
search_hashmap(const Hashmap *map, const char *key);

#endif /* HASHMAP_H */
