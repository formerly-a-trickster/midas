#include "lexer.h"
#include "parser.h"
#include <stdbool.h>
#include <stdlib.h>

static struct exp* expression(struct par_state*);
static struct exp* equality(struct par_state*);
static struct exp* comparison(struct par_state*);
static struct exp* addition(struct par_state*);
static struct exp* multiplication(struct par_state*);
/* static struct exp* unary(struct par_state*); */
static struct exp* primary(struct par_state*);

static struct tok* tok_next(struct par_state*);
static bool tok_matches(struct par_state*, enum tok_type);

static struct exp* exp_new_binary(struct tok*, struct exp*, struct exp*);
static struct exp* exp_new_group(struct exp*, struct tok*, struct tok*);
static struct exp* exp_new_integer(struct tok*);

void
par_init(struct par_state* par)
{
    lex_init(&par->lex);
    par->had_error = false;
    par->prev_tok = NULL;
    par->this_tok = NULL;
}

struct exp*
par_read(struct par_state* par, FILE* source)
{
    lex_feed(&par->lex, source);
    tok_next(par);

    return expression(par);
}

static struct exp*
expression(struct par_state* par)
/*  expression -> equality                                                   */
{
    return equality(par);
}

static struct exp*
equality(struct par_state* par)
/*  equality -> comparison ( ( "!=" | "==" ) comparison )*                   */
{
    struct exp* left = comparison(par);

    while (tok_matches(par, TOK_BANG_EQUAL) ||
           tok_matches(par, TOK_EQUAL_EQUAL))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = comparison(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
comparison(struct par_state* par)
/*  comparison -> addition ( ( ">" | ">=" | "<" | "<=" ) addition)*          */
{
    struct exp* left = addition(par);

    while (tok_matches(par, TOK_GREAT) || tok_matches(par, TOK_GREAT_EQUAL) ||
           tok_matches(par, TOK_LESS) || tok_matches(par, TOK_LESS_EQUAL))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = addition(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
addition(struct par_state* par)
/*  addition -> multiplication ( ( "-" | "+" ) multiplication)*              */
{
    struct exp* left = multiplication(par);

    while (tok_matches(par, TOK_MINUS) || tok_matches(par, TOK_PLUS))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = multiplication(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
multiplication(struct par_state* par)
/*  multiplication -> primary ( ( "/" | "*" ) primary )*                     */
{
    struct exp* left = primary(par);

    while (tok_matches(par, TOK_SLASH) || tok_matches(par, TOK_STAR))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = primary(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

/*  unary -> ( ("!" | "-") unary )*
           | primary                                                         */

static struct exp*
primary(struct par_state* par)
/* primary -> NUMBER                              | STRING | "false" | "true" |
            | "(" expression ")"                                             */
{
    if (tok_matches(par, TOK_NUMBER))
    {
        return exp_new_integer(par->prev_tok);
    }
    else if (tok_matches(par, TOK_PAREN_LEFT))
    {
        struct exp* exp = expression(par);
        if (!tok_matches(par, TOK_PAREN_RIGHT))
        {
            printf("Error:\n"
                   "Expected closing paren.");
            exit(0);
        }
        else
            return exp;
    }
    else
    {
        printf("Expected number or paren, got %s\n", par->this_tok->lexeme);
        exit(0);
    }
}

static struct tok*
tok_next(struct par_state* par)
{
    par->prev_tok = par->this_tok;
    par->this_tok = lex_get_tok(&par->lex);

    return par->prev_tok;
}

static bool
tok_matches(struct par_state* par, enum tok_type type)
{
    if (par->this_tok->type == type)
    {
        tok_next(par);
        return true;
    }
    else
        return false;
}

static struct exp*
exp_new_binary(struct tok* op, struct exp* left, struct exp* right)
{
    struct exp* e = malloc(sizeof(struct exp));

    e->type = EXP_BINARY;
    e->data.binary.op = op;
    e->data.binary.left = left;
    e->data.binary.right = right;

    return e;
}

static struct exp*
exp_new_group(struct exp* exp, struct tok* lparen, struct tok* rparen)
{
    struct exp* e = malloc(sizeof(struct exp));

    e->type = EXP_GROUP;
    e->data.group.exp = exp;
    e->data.group.lparen = lparen;
    e->data.group.rparen = rparen;

    return e;
}

static struct exp*
exp_new_integer(struct tok* tok)
{
    struct exp* e = malloc(sizeof(struct exp));

    e->type = EXP_INTEGER;
    e->data.integer = tok;

    return e;
}

void
ast_print(struct exp* exp)
{
    switch (exp->type)
    {
        case EXP_BINARY:
            printf("( %s ", exp->data.binary.op->lexeme);
            ast_print(exp->data.binary.left);
            ast_print(exp->data.binary.right);
            printf(") ");
            break;

        case EXP_GROUP:
            printf("[ ");
            ast_print(exp->data.group.exp);
            printf("] ");
            break;

        case EXP_INTEGER:
            printf("%s ", exp->data.integer->lexeme);
            break;
    }
}

