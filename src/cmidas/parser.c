#include "error.h"
#include "lexer.h"
#include "parser.h"

#include <stdbool.h>
#include <stdlib.h>

static struct exp* expression(struct par_state*);
static struct exp* equality(struct par_state*);
static struct exp* comparison(struct par_state*);
static struct exp* addition(struct par_state*);
static struct exp* multiplication(struct par_state*);
static struct exp* unary(struct par_state*);
static struct exp* primary(struct par_state*);

static struct tok* tok_next(struct par_state*);
static bool tok_matches(struct par_state*, enum tok_type);

static struct exp* exp_new_binary(struct tok*, struct exp*, struct exp*);
static struct exp* exp_new_unary(struct tok*, struct exp*);
static struct exp* exp_new_group(struct exp*, struct tok*, struct tok*);
static struct exp* exp_new_literal(struct tok*);

void
par_init(struct par_state* par)
{
    lex_init(&par->lex);
    par->had_error = false;
    par->prev_tok = NULL;
    par->this_tok = NULL;
}

struct exp*
par_read(struct par_state* par, const char* path)
{
    lex_feed(&par->lex, path);
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
/*  multiplication -> unary ( ( "/" | "*" ) unary )*                     */
{
    struct exp* left = unary(par);

    while (tok_matches(par, TOK_SLASH) || tok_matches(par, TOK_STAR))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = unary(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
unary(struct par_state* par)
/*  unary -> ( ("!" | "-") unary )
           | primary                                                         */
{
    if (tok_matches(par, TOK_BANG) || tok_matches(par, TOK_MINUS))
    {
        struct tok* op = par->prev_tok;
        struct exp* exp = unary(par);
        return exp_new_unary(op, exp);
    }
    else
        return primary(par);
}

static struct exp*
primary(struct par_state* par)
/* primary -> NUMBER                              | STRING | "false" | "true" |
            | "(" expression ")"                                             */
{
    if (tok_matches(par, TOK_INTEGER))
    {
        return exp_new_literal(par->prev_tok);
    }
    else if (tok_matches(par, TOK_PAREN_LEFT))
    {
        struct exp* exp = expression(par);
        if (!tok_matches(par, TOK_PAREN_RIGHT))
        {
            err_at_tok(par->lex.path, par->this_tok,
                "\n    Expected a closing paren, instead got `%s`.\n\n",
                par->this_tok->lexeme);
        }
        else
            return exp;
    }
    else
    {
        err_at_tok(par->lex.path, par->this_tok,
            "\n    Expected number or paren, instead got `%s`.\n\n",
            par->this_tok->lexeme);
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
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_BINARY;
    exp->data.binary.op = op;
    exp->data.binary.left = left;
    exp->data.binary.right = right;

    return exp;
}

static struct exp*
exp_new_unary(struct tok* op, struct exp* operand)
{
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_UNARY;
    exp->data.unary.op = op;
    exp->data.unary.exp = operand;

    return exp;
}

static struct exp*
exp_new_group(struct exp* group, struct tok* lparen, struct tok* rparen)
// XXX group codepaths are neither used nor tested
{
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_GROUP;
    exp->data.group.exp = group;
    exp->data.group.lparen = lparen;
    exp->data.group.rparen = rparen;

    return exp;
}

static struct exp*
exp_new_literal(struct tok* tok)
{
    struct exp* exp = malloc(sizeof(struct exp));

    switch (tok->type)
    {
        case TOK_INTEGER:
            exp->type = EXP_LITERAL;
            exp->data.literal = tok;
            break;

        default:
            printf("Parser error:\n"
                   "Literal expression received unexped token type\n");
            exit(0);
    }

    return exp;
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

        case EXP_UNARY:
            printf("( %s ", exp->data.unary.op->lexeme);
            ast_print(exp->data.unary.exp);
            printf(")");
            break;

        case EXP_GROUP:
            printf("[ ");
            ast_print(exp->data.group.exp);
            printf("] ");
            break;

        case EXP_LITERAL:
            printf("%s ", exp->data.literal->lexeme);
            break;
    }
}

