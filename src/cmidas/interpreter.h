#ifndef MD_interpreter_h_
#define MD_interpreter_h_

#include "lexer.h"
#include "parser.h"

struct val
{
    enum
    {
        VAL_INTEGER,
        VAL_DOUBLE,
        VAL_STRING,
    } type;

    union
    {
        long as_long;
        double as_double;
        const char* as_string;
    } data;
};

struct val val_new(struct tok*);
void execute(struct stm*);
struct val evaluate(struct exp*);

#endif
