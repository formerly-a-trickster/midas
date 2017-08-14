#ifndef MD_parser_h_
#define MD_parser_h_

#include "utils.h"
#include "lexer.h"

struct exp
{
    enum
    {
        EXP_BINARY,
        EXP_UNARY,
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
            struct tok* op;
            struct exp* exp;
        } unary;

        struct
        {
            struct exp* exp;
            struct tok* lparen;
            struct tok* rparen;
        } group;

        struct tok* literal;
    } data;
};

struct stm
{
    enum
    {
        STM_BLOCK,
        STM_EXPR_STMT,
        STM_PRINT
    } type;

    union
    {
        struct stmlist* block;

        struct
        {
            struct exp* exp;
            struct tok* semi;
        } expr;

        struct
        {
            struct exp* exp;
            struct tok* semi;
        } print;
    } data;
};

struct par_state
{
    struct lex_state lex;
    const char* path;
    struct tok* prev_tok;
    struct tok* this_tok;
};

struct stm* parse(struct par_state*, const char*);
void print_stm(struct stm*);
void print_exp(struct exp*);

#endif

