#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>

struct value
val_new(struct tok* tok)
{
    struct value val;

    switch (tok->type)
    {
        case TOK_NUMBER:
            val.type = VAL_INTEGER;
            // XXX this will silently fail for large enough numbers
            val.data.as_long = atol(tok->lexeme);
            break;

        default:
            printf("Interpreter error:\n"
                   "Tried to envalue un unrecognized token type\n");
            exit(0);
    }

    return val;
}

struct value
evaluate(struct expr* expr)
{
    switch (expr->type)
    {
        case EXPR_BINARY:
            ;
            struct value left = evaluate(expr->data.binary.left);
            struct value right = evaluate(expr->data.binary.right);
            return bin_op(expr->data.binary.op, left, right);

        case EXPR_GROUP:
            return evaluate(expr->data.group.expr);

        case EXPR_INTEGER:
            return val_new(expr->data.integer);

        default:
            printf("Interpreter error:\n"
                   "Tried to evaluate unexpected expression type.\n");
            exit(0);
    }
}

struct value
bin_op(struct tok* tok, struct value left, struct value right)
{
    switch (tok->type)
    {
        case TOK_PLUS:
            left.data.as_long = left.data.as_long + right.data.as_long;
            return left;

        case TOK_MINUS:
            left.data.as_long = left.data.as_long - right.data.as_long;
            return left;

        case TOK_STAR:
            left.data.as_long = left.data.as_long * right.data.as_long;
            return left;

        case TOK_SLASH:
            left.data.as_long = left.data.as_long / right.data.as_long;
            return left;

        default:
            printf("Interpreter error:\n"
                   "Tried to apply an unexpected binary operation.\n");
            exit(0);
    }
}

#include <stdio.h>
#include <stdlib.h>

