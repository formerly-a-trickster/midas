#include <stdlib.h>
#include <stdio.h>

#include "environ.h"
#include "hash.h"

#define T Environ_T

struct T
{
         T parent;
    Hash_T hash;
};

T
Environ_new(T parent)
{
    T env;

    env = malloc(sizeof(struct T));
    env->parent = parent;
    env->hash = Hash_new();

    return env;
}

T
Environ_parent(T env)
{
    return env->parent;
}

void *
Environ_var_new(T env, const char *key, void *val)
{
    return Hash_set(env->hash, key, val);
}

void *
Environ_var_set(T env, const char *key, void *val)
{
    T ctx;

    for (ctx = env; ctx != NULL; ctx = ctx->parent)
    {
        if (Hash_get(ctx->hash, key))
            return Hash_set(ctx->hash, key, val);
    }

    return NULL;
}

void *
Environ_var_get(T env, const char* key)
{
    T ctx;
    void *val;

    for (ctx = env; ctx != NULL; ctx = ctx->parent)
    {
        val = Hash_get(ctx->hash, key);

        if (val != NULL)
            return val;
    }

    return NULL;
}

void
Environ_map(T env, void map_fun(const char *key, void *val))
{
    T ctx;

    for (ctx = env; ctx != NULL; ctx = ctx->parent)
        Hash_map(ctx->hash, map_fun);
}

void
Environ_free(T env)
{
    Hash_free(env->hash);
    free(env);
}

#undef T

