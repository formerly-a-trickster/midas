#ifndef MD_interpreter_h
#define MD_interpreter_h

#include "lexer.h"
#include "parser.h"

struct value
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


struct value val_new(struct tok*);
struct value evaluate(struct expr* expr);
struct value bin_op(struct tok*, struct value, struct value);

#endif
