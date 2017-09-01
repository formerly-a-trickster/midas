#ifndef MD_PARSER
#define MD_PARSER

#include "lexer.h"
#include "vector.h"

#define T Parser_T

typedef struct T *T;

struct stm
{
    enum
    {
        STM_BLOCK,
        STM_IF,
        STM_WHILE,
        STM_BREAK,
        STM_VAR_DECL,
        STM_FUN_DECL,
        STM_PRINT,
        STM_EXP_STM
    } type;

    union
    {
        Vector_T block; /* (struct stm *) */

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
            const char *name;
            struct exp *exp;
        } var_decl;

        struct
        {
            const char *name;
              Vector_T  formals; /* (const char *) */
            struct stm *body;
        } fun_decl;

        struct exp *print;

        struct exp *exp_stm;
    } data;
};

struct exp
{
    enum
    {
        EXP_ASSIGN,
        EXP_BINARY,
        EXP_UNARY,
        EXP_CALL,
        EXP_IDENT,
        EXP_LITERAL
    } type;

    union
    {
        struct
        {
            const char *name;
            struct exp *exp;
        } assign;

        struct
        {
            enum tok_t  op;
            struct exp *left;
            struct exp *right;
        } binary;

        struct
        {
            enum tok_t  op;
            struct exp *exp;
        } unary;

        struct
        {
            struct exp *callee;
              Vector_T  params; /* (struct exp *) */
        } call;

        const char *name;

        struct tok *literal;
    } data;
};

       T Par_new  (void);
Vector_T Par_parse(T par, const char *path);

    void print_stm(struct stm *, int);
    void print_exp(struct exp *);

#undef T

#endif /* MD_PARSER */

