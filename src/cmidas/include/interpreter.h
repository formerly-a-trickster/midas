#ifndef MD_interpreter_h_
#define MD_interpreter_h_

#include "env.h"
#include "vector.h"

#include <stdbool.h>

enum val_type
{
    VAL_BOOLEAN,
    VAL_INTEGER,
    VAL_DOUBLE,
    VAL_STRING
};

struct val
{
    enum val_type type;

    union
    {
        bool as_bool;
        long as_long;
        double as_double;
        const char* as_string;
    } data;
};

struct intpr
{
          const char *path;
               Env_T  globals;
               Env_T  context;
};

struct intpr *intpr_new(void);
void          interpret(struct intpr *, const char *, Vector_T);

#endif
