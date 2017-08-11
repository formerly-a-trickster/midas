#ifndef MD_parser_h
#define MD_parser_h

#include "lexer.h"

struct exp
{
    enum
    {
        EXP_BINARY,
        EXP_GROUP,
        EXP_LITERAL
    } type;

    union
    {
        struct
        {
            struct tok* op;
            struct exp* left;
            struct exp* right;
        } binary;

        struct
        {
            struct exp* exp;
            struct tok* lparen;
            struct tok* rparen;
        } group;

        struct tok* literal;
    } data;
};

struct par_state
{
    struct lex_state lex;
    bool had_error;
    struct tok* prev_tok;
    struct tok* this_tok;
};

void par_init(struct par_state*);
struct exp* par_read(struct par_state*, const char*);

void ast_print(struct exp*);

#endif

