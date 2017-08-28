#include <stdbool.h>

#include "interpreter.h"
#include "lexer.h"
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

struct val Val_new      (struct tok *);
      bool Val_is_truthy(struct val);
struct val Val_binop    (enum tok_t op, struct val left, struct val right);
struct val Val_unop     (enum tok_t op, struct val operand);
      void Val_print    (struct val);

