#ifndef MD_ENVIRON
#define MD_ENVIRON

#define T Environ_T

typedef struct T *T;

extern    T  Environ_new    (T parent);
extern    T  Environ_parent (T env);
extern void *Environ_var_new(T env, const char *key, void *val);
extern void *Environ_var_set(T env, const char *key, void *vel);
extern void *Environ_var_get(T env, const char *key);
extern void  Environ_free   (T env);

#undef T

#endif /* MD_ENVIRON */

