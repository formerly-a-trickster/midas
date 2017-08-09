#include "lexer.h"
#include "parser.h"
#include <stdbool.h>
#include <stdlib.h>

static struct expr* expression(struct par_state*);
static struct expr* equality(struct par_state*);
static struct expr* comparison(struct par_state*);
static struct expr* addition(struct par_state*);
static struct expr* multiplication(struct par_state*);
/* static struct expr* unary(struct par_state*); */
static struct expr* primary(struct par_state*);

static struct tok* tok_next(struct par_state*);
static bool tok_matches(struct par_state*, enum tok_type);

static struct expr* expr_new_binary(struct tok*, struct expr*, struct expr*);
static struct expr* expr_new_group(struct expr*, struct tok*, struct tok*);
static struct expr* expr_new_integer(struct tok*, int);

void
par_init(struct par_state* par)
{
    lex_init(&par->lex);
    par->had_error = false;
    par->prev_tok = NULL;
    par->this_tok = NULL;
}

struct expr*
par_read(struct par_state* par, FILE* source)
{
    lex_feed(&par->lex, source);
    tok_next(par);

    return expression(par);
}

static struct expr*
expression(struct par_state* par)
/*  expression -> equality                                                   */
{
    return equality(par);
}

static struct expr*
equality(struct par_state* par)
/*  equality -> comparison ( ( "!=" | "==" ) comparison )*                   */
{
    struct expr* left = comparison(par);

    while (tok_matches(par, TOK_BANG_EQUAL) ||
           tok_matches(par, TOK_EQUAL_EQUAL))
    {
        struct tok* op = par->prev_tok;
        struct expr* right = comparison(par);
        left = expr_new_binary(op, left, right);
    }

    return left;
}

static struct expr*
comparison(struct par_state* par)
/*  comparison -> addition ( ( ">" | ">=" | "<" | "<=" ) addition)*          */
{
    struct expr* left = addition(par);

    while (tok_matches(par, TOK_GREAT) || tok_matches(par, TOK_GREAT_EQUAL) ||
           tok_matches(par, TOK_LESS) || tok_matches(par, TOK_LESS_EQUAL))
    {
        struct tok* op = par->prev_tok;
        struct expr* right = addition(par);
        left = expr_new_binary(op, left, right);
    }

    return left;
}

static struct expr*
addition(struct par_state* par)
/*  addition -> multiplication ( ( "-" | "+" ) multiplication)*              */
{
    struct expr* left = multiplication(par);

    while (tok_matches(par, TOK_MINUS) || tok_matches(par, TOK_PLUS))
    {
        struct tok* op = par->prev_tok;
        struct expr* right = multiplication(par);
        left = expr_new_binary(op, left, right);
    }

    return left;
}

static struct expr*
multiplication(struct par_state* par)
/*  multiplication -> primary ( ( "/" | "*" ) primary )*                     */
{
    struct expr* left = primary(par);

    while (tok_matches(par, TOK_SLASH) || tok_matches(par, TOK_STAR))
    {
        struct tok* op = par->prev_tok;
        struct expr* right = primary(par);
        left = expr_new_binary(op, left, right);
    }

    return left;
}

/*  unary -> ( ("!" | "-") unary )*
           | primary                                                         */

static struct expr*
primary(struct par_state* par)
/* primary -> NUMBER                              | STRING | "false" | "true" |
            | "(" expression ")"                                             */
{
    if (tok_matches(par, TOK_NUMBER))
    {
        int value = atoi(par->prev_tok->lexeme);
        return expr_new_integer(par->prev_tok, value);
    }
    else if (tok_matches(par, TOK_PAREN_LEFT))
    {
        struct expr* expr = expression(par);
        if (!tok_matches(par, TOK_PAREN_RIGHT))
        {
            printf("Error:\n"
                   "Expected closing paren.");
            exit(0);
        }
        else
            return expr;
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

static struct expr*
expr_new_binary(struct tok* op, struct expr* left, struct expr* right)
{
    struct expr* e = malloc(sizeof(struct expr));

    e->type = EXPR_BINARY;
    e->data.binary.op = op;
    e->data.binary.left = left;
    e->data.binary.right = right;

    return e;
}

static struct expr*
expr_new_group(struct expr* expr, struct tok* lparen, struct tok* rparen)
{
    struct expr* e = malloc(sizeof(struct expr));

    e->type = EXPR_GROUP;
    e->data.group.expr = expr;
    e->data.group.lparen = lparen;
    e->data.group.rparen = rparen;

    return e;
}

static struct expr*
expr_new_integer(struct tok* literal, int value)
{
    struct expr* e = malloc(sizeof(struct expr));

    e->type = EXPR_INTEGER;
    e->data.integer.literal = literal;
    e->data.integer.value = value;

    return e;
}

void
ast_print(struct expr* expr)
{
    switch (expr->type)
    {
        case EXPR_BINARY:
            printf("( %s ", expr->data.binary.op->lexeme);
            ast_print(expr->data.binary.left);
            ast_print(expr->data.binary.right);
            printf(") ");
            break;

        case EXPR_GROUP:
            printf("[ ");
            ast_print(expr->data.group.expr);
            printf("] ");
            break;

        case EXPR_INTEGER:
            printf("%i ", expr->data.integer.value);
            break;
    }
}

