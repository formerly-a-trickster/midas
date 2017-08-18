#include <stdlib.h>
#include <string.h>
#include "hash.h"

#define T Hash_T

struct T
{
    struct row
    {
        const char *key;
        void *val;
        int dist;
    } **table;

    int size;
    int slack;
};

static    struct row *Hash_get_row(T hash, const char *key);
static          void  Hash_insert (T hash, struct row *row);
static unsigned long  Hash_fun    (const char *key);
static          void  Hash_grow   (T hash);

T
Hash_new(void)
{
    T hash = malloc(sizeof(struct Hash_T));
    hash->size  = 2;
    hash->slack = 1;
    hash->table = malloc(sizeof(struct row) * 3);

    return hash;
}

void *
Hash_set(T hash, const char *key, void *val)
{
    struct row *row;
    void *prev_val;

    prev_val = NULL;
    row = Hash_get_row(hash, key);

    if (row == NULL)
    {
        row = malloc(sizeof(struct row));
        row->key  = key;
        row->val  = val;
        row->dist = 0;
        Hash_insert(hash, row);
    }
    else
    {
        prev_val = row->val;
        row->val = val;
    }

    return prev_val;
}

void *
Hash_get(T hash, const char *key)
{
    struct row* row;

    row = Hash_get_row(hash, key);

    if (row != NULL)
        return row->val;
    else
        return NULL;
}

static struct row *
Hash_get_row(T hash, const char *key)
{
    unsigned long hkey;
    struct row *row;
    int probes;

    for
    (
        hkey = Hash_fun(key) % hash->size,
        row = hash->table[hkey],
        probes = 0;

        probes < hash->slack && row != NULL;

        ++hkey,
        ++probes,
        row = hash->table[hkey]
    )
    {
        if (strcmp(row->key, key) == 0)
            return row;
    }

    return NULL;
}

void
Hash_insert(T hash, struct row *row)
{
    unsigned long hkey;
    int probes;
    struct row *temp;

    for
    (
        hkey = Hash_fun(row->key) % hash->size,
        probes = 0,
        row->dist = 0;

        ;

        ++hkey,
        ++probes,
        ++row->dist
    )
    {
        if (probes > hash->slack)
        {
            Hash_grow(hash);
            Hash_insert(hash, row);
            break;
        }
        else if (hash->table[hkey] == NULL)
        {
            hash->table[hkey] = row;
            break;
        }
        else if (hash->table[hkey]->dist < probes)
        {
            temp = hash->table[hkey];
            hash->table[hkey] = row;
            row = temp;
        }
    }
}

static unsigned long
Hash_fun(const char *key)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static void
Hash_grow(T hash)
{
    struct row **old_table;
    int old_size;
    int old_slack;
    int i;

    old_table = hash->table;
    old_size  = hash->size;
    old_slack = hash->slack;
    hash->size  *= 2;
    hash->slack += 1;
    hash->table = malloc(sizeof(struct row) * (hash->size + hash->slack));

    for (i = 0; i < old_size + old_slack; ++i)
    {
        if (old_table[i] != NULL)
            Hash_insert(hash, old_table[i]);
    }
    free(old_table);
}

#undef T

