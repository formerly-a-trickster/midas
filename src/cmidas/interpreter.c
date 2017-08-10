#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>

struct val
val_new(struct tok* tok)
{
    struct val val;

    switch (tok->type)
    {
        case TOK_NUMBER:
            val.type = VAL_INTEGER;
            // XXX this will silently fail for large enough numbers
            val.data.as_long = atol(tok->lexeme);
            break;

        default:
            printf("Interpreter error:\n"
                   "Tried to enval un unrecognized token type\n");
            exit(0);
    }

    return val;
}

struct val
evaluate(struct exp* exp)
{
    switch (exp->type)
    {
        case EXP_BINARY:
            ;
            struct val left = evaluate(exp->data.binary.left);
            struct val right = evaluate(exp->data.binary.right);
            return bin_op(exp->data.binary.op, left, right);

        case EXP_GROUP:
            return evaluate(exp->data.group.exp);

        case EXP_INTEGER:
            return val_new(exp->data.integer);

        default:
            printf("Interpreter error:\n"
                   "Tried to evaluate unexpected expression type.\n");
            exit(0);
    }
}

struct val
bin_op(struct tok* tok, struct val left, struct val right)
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

