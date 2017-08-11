#ifndef MD_interpreter_h
#define MD_interpreter_h

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
        long int as_long;
        double as_double;
        const char* as_string;
    } data;
};


struct val val_new(struct tok*);
struct val evaluate(struct exp* exp);

#endif
