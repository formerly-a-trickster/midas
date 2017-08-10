#ifndef MD_parser_h
#define MD_parser_h

#include "lexer.h"
#include <stdio.h>

struct expr
{
    enum
    {
        EXPR_BINARY,
        EXPR_GROUP,
        EXPR_INTEGER
    } type;

    union
    {
        struct
        {
            struct tok* op;
            struct expr* left;
            struct expr* right;
        } binary;

        struct
        {
            struct expr* expr;
            struct tok* lparen;
            struct tok* rparen;
        } group;

        struct tok* integer;
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
struct expr* par_read(struct par_state*, FILE*);

void ast_print(struct expr*);

#endif

