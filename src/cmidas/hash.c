#include "hash.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long hash_fun(const char*);
static struct entry* entry_new(const char*, void*);
static void hash_insert_entry(struct hash*, struct entry*);
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
hash_insert(struct hash* hash, const char* key, void* val)
{
    struct entry* entry = entry_new(key, val);
    hash_insert_entry(hash, entry);
}

struct entry*
hash_search(struct hash* hash, const char* key)
{
    unsigned long hkey = hash_fun(key) % hash->size;
    struct entry* cell = hash->table[hkey];
    unsigned long len = hash->size + hash->slack;

    for (; hkey < len && cell != NULL; ++hkey, cell = hash->table[hkey])
    {
        if (strcmp(cell->key, key) == 0)
            return cell;
    }
    return NULL;
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
entry_new(const char* key, void* val)
{
    struct entry* entry = malloc(sizeof(struct entry));
    entry->dist = 0;
    entry->key = key;
    entry->val = val;

    return entry;
}

static void
hash_insert_entry(struct hash* hash, struct entry* entry)
{
    unsigned long hkey;
    int probes;
    for
    (
        hkey = hash_fun(entry->key) % hash->size,
        probes = 0,
        entry->dist = 0;
        ;
        ++hkey,
        ++probes,
        ++entry->dist
    )
    {
        if (probes > hash->slack)
        {
            hash_enlarge(hash);
            hash_insert_entry(hash, entry);
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
            hash_insert_entry(hash, old_table[i]);
    }
    free(old_table);
}

