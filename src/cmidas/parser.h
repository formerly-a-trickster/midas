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
            struct token* op;
            struct expr* left;
            struct expr* right;
        } binary;

        struct
        {
            struct expr* expr;
            struct token* lparen;
            struct token* rparen;
        } group;

        int integer;
    } data;
};

struct par_state
{
    struct lex_state lex;
    bool had_error;
    struct token* prev_token;
    struct token* this_token;
};

void par_init(struct par_state*);
void par_read(struct par_state*, FILE*);

void ast_print(struct expr*);

#endif

