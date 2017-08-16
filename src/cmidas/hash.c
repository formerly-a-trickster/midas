#include "hash.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long hash_fun(const char*);
static struct entry* entry_new(const char*, struct val*);
static unsigned long hash_loc_plus1(struct hash*, const char*);
static void hash_add_entry(struct hash*, struct entry*);
static void hash_enlarge(struct hash*);

struct hash*
hash_new(void)
{
    struct hash* hash = malloc(sizeof(struct hash));
    hash->size = 2;
    hash->slack = 1;
    hash->table = malloc(sizeof(struct entry) * 3);

    return hash;
}

void
hash_insert(struct hash* hash, const char* key, struct val* val)
{
    unsigned long index = hash_loc_plus1(hash, key);
    if (index == 0)
    {
        struct entry* entry = entry_new(key, val);
        hash_add_entry(hash, entry);
    }
    else
    {
        --index;
        hash->table[index]->val = val;
    }
}

struct val*
hash_get(struct hash* hash, const char* key)
{
    unsigned long hkey = hash_fun(key) % hash->size;
    struct entry* cell = hash->table[hkey];
    unsigned long len = hash->size + hash->slack;

    for (; hkey < len && cell != NULL; ++hkey, cell = hash->table[hkey])
    {
        if (strcmp(cell->key, key) == 0)
            return cell->val;
    }
    return NULL;
}

void
hash_print(struct hash* hash)
{
    float items = 0.0;
    struct entry* cell;
    for (int i = 0; i < hash->size; ++i)
    {
        cell = hash->table[i];
        if (cell == NULL)
        {
            printf("%2i: nil\n", i);
        }
        else
        {
            printf("%2i. d:%i, %s => ", i, cell->dist, cell->key);
            val_print(*cell->val);
            ++items;
        }
    }
    printf("----------\n");
    for (int i = hash->size; i < hash->size + hash->slack; ++i)
    {
        cell = hash->table[i];
        if (cell == NULL)
        {
            printf("%2i: nil\n", i);
        }
        else
        {
            printf("%2i. d:%i, %s => ", i, cell->dist, cell->key);
            val_print(*cell->val);
            ++items;
        }
    }

    printf("Load: %f\n\n", items / (hash->size + hash->slack));
}

static unsigned long
hash_fun(const char* key)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static struct entry*
entry_new(const char* key, struct val* val)
{
    struct entry* entry = malloc(sizeof(struct entry));
    entry->dist = 0;
    entry->key = key;
    entry->val = val;

    return entry;
}

static unsigned long
hash_loc_plus1(struct hash* hash, const char* key)
/* A return value of 0 means that the key is not in the hash table, otherwise,
   it returns the index + 1.                                                 */
/* XXX as this searching function is called by `hash_insert`, it could trigger
   a resize in order to save time */
{
    unsigned long hkey = hash_fun(key) % hash->size;
    struct entry* cell = hash->table[hkey];
    unsigned long len = hash->size + hash->slack;

    for (; hkey < len && cell != NULL; ++hkey, cell = hash->table[hkey])
    {
        if (strcmp(cell->key, key) == 0)
            return hkey + 1;
    }
    return 0;
}

static void
hash_add_entry(struct hash* hash, struct entry* entry)
{
    unsigned long hkey;
    int probes;
    entry->dist = 0;
    for
    (
        hkey = hash_fun(entry->key) % hash->size,
        probes = 0;
        ;
        ++hkey,
        ++probes,
        ++entry->dist
    )
    {
        if (probes > hash->slack)
        {
            hash_enlarge(hash);
            hash_add_entry(hash, entry);
            break;
        }
        else if (hash->table[hkey] == NULL)
        {
            hash->table[hkey] = entry;
            break;
        }
        else if (hash->table[hkey]->dist < probes)
        {
            struct entry* temp = hash->table[hkey];
            hash->table[hkey] = entry;
            entry = temp;
        }
    }
}

static void
hash_enlarge(struct hash* hash)
{
    struct entry** old_table = hash->table;
    int old_size = hash->size;
    int old_slack = hash->slack;
    hash->size *= 2;
    hash->slack += 1;
    hash->table = malloc(sizeof(struct entry) * (hash->size + hash->slack));

    for (int i = 0; i < old_size + old_slack; ++i)
    {
        if (old_table[i] != NULL)
            hash_add_entry(hash, old_table[i]);
    }
    free(old_table);
}

