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
    } *table;

    int size;
    int slack;
};

static    struct row *Hash_get_row(T hash, const char *key);
static          void  Hash_insert (T hash, const char *key, void *val);
static unsigned long  Hash_fun    (const char *key);
static          void  Hash_grow   (T hash);

T
Hash_new(void)
{
    T hash = malloc(sizeof(struct Hash_T));
    hash->size  = 2;
    hash->slack = 1;
    hash->table = calloc(hash->size + hash->slack, sizeof(struct row));

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
        Hash_insert(hash, key, val);
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

void
Hash_free(T hash)
{
    free(hash->table);
    free(hash);
}

static struct row *
Hash_get_row(T hash, const char *key)
{
    unsigned long hkey;
    struct row *row;
    int probes;

    hkey = Hash_fun(key) % hash->size;
    for
    (
        row = hash->table + hkey,
        probes = 0;

        probes < hash->slack && row->key != NULL;

        ++probes,
        ++row
    )
    {
        if (strcmp(row->key, key) == 0)
            return row;
    }

    return NULL;
}

static void
Hash_insert(T hash, const char *key, void *val)
{
    struct row *row;
    unsigned long hkey;
    int probes, dist;

    hkey = Hash_fun(key) % hash->size;
    for
    (
        row = hash->table + hkey,
        probes = 0,
        dist = 0;

        ;

        ++probes,
        ++dist,
        ++row
    )
    {
        if (probes > hash->slack)
        {
            Hash_grow(hash);
            Hash_insert(hash, key, val);
            break;
        }
        else if (row->key == NULL)
        {
            row->key  = key;
            row->val  = val;
            row->dist = dist;
            break;
        }
        else if (row->dist < probes)
        {
            struct row temp_row;

            temp_row = *row;

            row->key  = key;
            row->val  = val;
            row->dist = dist;

            key  = temp_row.key;
            val  = temp_row.val;
            dist = temp_row.dist;

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
    struct row *old_table, *old_row;
    int old_size, old_slack, i;

    old_table = hash->table;
    old_row   = hash->table;
    old_size  = hash->size;
    old_slack = hash->slack;

    hash->size  *= 2;
    hash->slack += 1;
    hash->table = calloc(hash->size + hash->slack, sizeof(struct row));

    for (i = 0; i < old_size + old_slack; ++i, ++old_row)
    {
        if (old_row->key != NULL)
            Hash_insert(hash, old_row->key, old_row->val);
    }
    free(old_table);
}

#undef T

