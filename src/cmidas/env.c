#include <stdlib.h>

#include "env.h"
#include "hash.h"

#define T Env_T

struct T
{
         T parent;
    Hash_T hash;
};

T
Env_new(T parent)
{
    T env;

    env = malloc(sizeof(struct T));
    env->parent = parent;
    env->hash = Hash_new();

    return env;
}

T
Env_parent(T env)
{
    return env->parent;
}

void *
Env_var_new(T env, const char *key, void *val)
{
    return Hash_set(env->hash, key, val);
}

void *
Env_var_set(T env, const char *key, void *val)
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
Env_var_get(T env, const char* key)
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
Env_free(T env)
{
    Hash_free(env->hash);
    free(env);
}

#undef T

