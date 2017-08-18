#ifndef MD_parser_h_
#define MD_parser_h_

#include "list.h"
#include "lexer.h"

struct stm
{
    enum
    {
        STM_BLOCK,
        STM_VAR_DECL,
        STM_EXPR_STMT,
        STM_PRINT
    } type;

    union
    {
        struct list *block;

        struct
        {
            struct tok *name;
            struct exp *exp;
        } var_decl;

        struct
        {
            struct exp *exp;
            struct tok *last;
        } expr;

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

struct stm *parse    (struct par_state *, const char *);
void        print_stm(struct stm *);
void        print_exp(struct exp *);

#endif

