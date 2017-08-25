#ifndef MD_interpreter_h_
#define MD_interpreter_h_

#include "env.h"
#include "vector.h"

#include <stdbool.h>

#define T Interpreter_T

typedef struct T *T;

   T Intpr_new(void);
void Intpr_run(T intpr, const char *path, Vector_T ast);

#undef T

#endif

