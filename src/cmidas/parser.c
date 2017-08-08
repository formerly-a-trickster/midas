#include "lexer.h"
#include "parser.h"
#include <stdbool.h>
#include <stdlib.h>

static struct expr*
expr_new_binary(struct token*, struct expr*, struct expr*);
static struct expr*
expr_new_group(struct expr*, struct token*, struct token*);
static struct expr* expr_new_integer(int);

void
par_init(struct par_state* par)
{
    lex_init(&par->lex);
    par->had_error = false;
    par->prev_token = NULL;
    par->this_token = NULL;
}

void
par_read(struct par_state* par, FILE* source)
{
    lex_feed(&par->lex, source);

    do
    {
        par->prev_token = par->this_token;
        par->this_token = lex_get_token(&par->lex);

        print_tok(par->this_token);
    }
    while (par->this_token->type != TOK_EOF);
}

static struct expr*
expr_new_binary(struct token* op, struct expr* left, struct expr* right)
{
    struct expr* e = malloc(sizeof(struct expr));

    e->type = EXPR_BINARY;
    e->data.binary.op = op;
    e->data.binary.left = left;
    e->data.binary.right = right;

    return e;
}

static struct expr*
expr_new_group(struct expr* expr, struct token* lparen, struct token* rparen)
{
    struct expr* e = malloc(sizeof(struct expr));

    e->type = EXPR_GROUP;
    e->data.group.expr = expr;
    e->data.group.lparen = lparen;
    e->data.group.rparen = rparen;

    return e;
}

static struct expr*
expr_new_integer(int integer)
{
    struct expr* e = malloc(sizeof(struct expr));

    e->type = EXPR_INTEGER;
    e->data.integer = integer;

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
            printf("%i ", expr->data.integer);
            break;
    }
}

