#ifndef MD_env_h_
#define MD_env_h_

#define T Environ_T

typedef struct T *T;

extern    T  Env_new    (T parent);
extern    T  Env_parent (T env);
extern void *Env_var_new(T env, const char *key, void *val);
extern void *Env_var_set(T env, const char *key, void *vel);
extern void *Env_var_get(T env, const char *key);
extern void  Env_free   (T env);

#undef T

#endif

