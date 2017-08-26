#include <stdbool.h>

#include "interpreter.h"
#include "parser.h"

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


struct val val_new      (Interpr_T interpr, struct tok *);
      bool val_is_truthy(struct val);
struct val binary_op    (Interpr_T interpr, struct tok *, struct val, struct val);
struct val unary_op     (Interpr_T interpr, struct tok *, struct val);
      void val_print    (struct val);

