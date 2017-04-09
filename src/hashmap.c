#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hashmap.h"
#include "util.h"

struct Hashmap
{
    const char **keys;
    void **data;   // array of void*
    int size;
    int amount_size;
};

typedef unsigned int uint;

static uint calc_hash(uint size, int c, const char *key);
static void rehash(Hashmap *hmap);

Hashmap *
make_hashmap(int size)
{
    Hashmap *hmap;

    assert(size > 0);

    hmap = (Hashmap*)mmalloc(sizeof(Hashmap));
    hmap->keys = (const char **)mcalloc(size, sizeof(char *));
    hmap->data = (void**)mcalloc(size, sizeof(void*));

    hmap->size = size;
    hmap->amount_size = 0;
    return hmap;
}

void
free_hashmap(Hashmap *map)
{
    int i;
    for (i = 0; i < map->size; ++i)
    {
        if (*(map->keys+i)) mfree(map->keys+i);
    }
    mfree(map->keys);
    mfree(map->data);
    mfree(map);
}

int
add_hashmap(Hashmap *hmap, const char *key, void *data)
{
    unsigned int hash;
    int i;

    if (exists_hashmap(hmap, key)) return 0;
    if (hmap->amount_size > (int)(hmap->size*0.7)) rehash(hmap);

    for (i = 0; ; ++i)
    {
        hash = calc_hash(hmap->size, i, key);
        if (*(hmap->keys+hash) == NULL)
        {
            *(hmap->keys+hash) = new_string(key);
            *(hmap->data+hash) = data;
            break;
        }
    }

    hmap->amount_size++;
    return 1;
}

void *
remove_hashmap(Hashmap *hmap, const char *key)
{
    uint hash;
    int i;

    for (i = 0; i < hmap->amount_size; ++i)
    {
        hash = calc_hash(hmap->size, i, key);
        if (*(hmap->keys+hash) == NULL) return NULL;
        if (strcmp(*(hmap->keys+hash), key) == 0)
        {
            void *d = *(hmap->data+hash);
            mfree(hmap->keys+hash);
            *(hmap->keys+hash) = NULL;
            *(hmap->data+hash) = NULL;
            hmap->amount_size--;
            return d;
        }
    }
    return NULL;
}

int
exists_hashmap(const Hashmap *hmap, const char *key)
{
    return search_hashmap(hmap, key) != NULL;
}

void *
search_hashmap(const Hashmap *hmap, const char *key)
{
    uint hash;
    int i;
    
    for (i = 0; i < hmap->amount_size; ++i)
    {
        hash = calc_hash(hmap->size, i, key);
        if (*(hmap->keys+hash) == NULL) return NULL;
        if (strcmp(*(hmap->keys+hash), key) == 0)
        {
            return *(hmap->data+hash);
        }
    }
    return NULL;
}

static uint
calc_hash(uint size, int c, const char *key)
{
    // FNV Hash
    uint hash = 2166136261u;
    int i, len = strlen(key);

    for (i = 0 ; i < len; ++i)
    {
        hash = (16777619u*hash) ^ *(key+i);
    }
    return (hash+c) % size;
}

static void
rehash(Hashmap *hmap)
{
    uint hash;
    int i, j;
    int new_size;
    const char **new_keys;
    void **new_data;

    new_size = hmap->size * 2;
    new_keys = (const char**)mcalloc(new_size, sizeof(char*));
    new_data = (void**)mcalloc(new_size, sizeof(void*));

    for (i = 0; i < hmap->size; ++i)
    {
        if (*(hmap->keys+i) == NULL) continue;
        for (j = 0; ; ++j)
        {
            hash = calc_hash(new_size, j, *(hmap->keys+i));
            if (*(new_keys+hash) == NULL)
            {
                *(new_keys+hash) = *(hmap->keys+i);
                *(new_data+hash) = *(hmap->data+i);
                break;
            }
        }
    }

    mfree(hmap->keys);
    mfree(hmap->data);
    hmap->size = new_size;
    hmap->keys = new_keys;
    hmap->data = new_data;
}

#ifdef TEST_HASHMAP
static void
debug_print(const Hashmap *hmap)
{
    int i;
    for (i = 0; i < hmap->size; ++i)
    {
        if (*(hmap->keys+i) != NULL)
        {
            printf("key=%s, val=%d, hash=%d\n",
                    *(hmap->keys+i),
                    *(int*)(*(hmap->data+i)),
                    calc_hash(hmap->size, 0, *(hmap->keys+i)));
        }
    }
}

int
main(int argc, char *argv[])
{
    Hashmap *map;
    const int BUFSIZE = 128;
    char buf[BUFSIZE], key[BUFSIZE];
    int val, *valp, size;

    if (argc != 2)
    {
        fprintf(stderr, "%s <Hashmap size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    size = atoi(argv[1]);
    if (size <= 0) size = 16;
    map = make_hashmap(size);

    printf("Hashmap size: %d\n", size);

    for (;;)
    {
        printf("command> ");
        fgets(buf, BUFSIZE, stdin);
        if (strncmp(buf, "add", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("val: "); fgets(buf, BUFSIZE, stdin);
            valp = (int*)mmalloc(sizeof(int));
            *valp = atoi(buf);
            printf("Add to hashmap (%s, %d)\n", key, *valp);
            add_hashmap(map, key, valp) ? puts("Success") : puts("Failed");
        }
        else if (strncmp(buf, "del", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("Delete from hashmap (%s)\n", key);
            valp = remove_hashmap(map, key);
            valp ? puts("Success") : puts("Failed");
            free(valp);
        }
        else if (strncmp(buf, "set", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("val: "); fgets(buf, BUFSIZE, stdin);
            val = atoi(buf);
            printf("Set new value to hashmap (%s, %d)\n", key, val);
            valp = search_hashmap(map, key);
            if (valp)
            {
                puts("Success");
                *valp = val;
            }
            else
            {
                puts("Failed");
            }
        }
        else if (strncmp(buf, "prn", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("Search from hashmap (%s)\n", key);
            valp = search_hashmap(map, key);
            if (valp)
            {
                puts("Success");
                printf("val = %d\n", *(int*)valp);
            }
            else
            {
                puts("Failed");
            }
        }
        else if (strncmp(buf, "all", 3) == 0)
        {
            printf("Hashmap size = %d\nAll items = %d\n",
                    map->size, map->amount_size);
            debug_print(map);
        }
        else if (strncmp(buf, "help", 4) == 0)
        {
            printf("add\tappend new item.\n"
                    "del\tdelete item.\n"
                    "set\tset new value.\n"
                    "prn\tprint specific key value.\n"
                    "all\tprint all keys and values.\n");
        }
        else
        {
            free_hashmap(map);
            return EXIT_SUCCESS;
        }
    }
}
#endif

