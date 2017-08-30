#ifndef MD_INTERPRETER
#define MD_INTERPRETER

#include "vector.h"

#define T Interpr_T

typedef struct T *T;

   T Interpr_new(void);
void Interpr_run(T intpr, const char *path, Vector_T ast);

#undef T

#endif /* MD_INTERPRETER */

