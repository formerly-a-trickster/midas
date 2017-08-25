#ifndef MD_parser_h_
#define MD_parser_h_

#include "vector.h"
#include "lexer.h"

struct stm
{
    enum
    {
        STM_BLOCK,
        STM_IF,
        STM_WHILE,
        STM_VAR_DECL,
        STM_EXP_STM,
        STM_PRINT
    } type;

    union
    {
        Vector_T block;

        struct
        {
            struct exp *cond;
            struct stm *then_block;
            struct stm *else_block;
        } if_cond;

        struct
        {
            struct exp *cond;
            struct stm *body;
        } while_cond;

        struct
        {
            struct tok *name;
            struct exp *exp;
        } var_decl;

        struct
        {
            struct exp *exp;
            struct tok *last;
        } exp;

        struct
        {
            struct exp *exp;
            struct tok *last;
        } print;
    } data;
};

struct exp
{
    enum
    {
        EXP_ASSIGN,
        EXP_BINARY,
        EXP_UNARY,
        EXP_GROUP,
        EXP_VAR,
        EXP_LITERAL
    } type;

    union
    {
        struct
        {
            struct tok *name;
            struct exp *exp;
        } assign;

        struct
        {
            struct tok *op;
            struct exp *left;
            struct exp *right;
        } binary;

        struct
        {
            struct tok *op;
            struct exp *exp;
        } unary;

        struct
        {
            struct exp *exp;
            struct tok *lparen;
            struct tok *rparen;
        } group;

        struct tok *name;

        struct tok *literal;
    } data;
};

struct par_state
{
    struct lex_state lex;
    const char *path;
    struct tok *prev_tok;
    struct tok *this_tok;
};

Vector_T parse    (struct par_state *, const char *);
void     print_stm(struct stm *);
void     print_exp(struct exp *);

#endif
