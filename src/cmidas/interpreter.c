#include "utils.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void execute(struct intpr*, struct stm*);
static struct val evaluate(struct intpr*, struct exp*);

static void val_print(struct val);
static struct val binary_op(struct intpr*, struct tok*, struct val, struct val);
static struct val unary_op(struct intpr*, struct tok*, struct val);
static bool val_equal(struct val, struct val);
static bool val_greater(struct intpr*, struct tok*, struct val, struct val);
static struct val val_add(struct intpr*, struct tok*, struct val, struct val);
static struct val val_sub(struct intpr*, struct tok*, struct val, struct val);
static struct val val_mul(struct intpr*, struct tok*, struct val, struct val);
static struct val val_div(struct intpr*, struct tok*, struct val, struct val);

static struct val val_new(struct intpr*, struct tok*);
static const char* val_type_str(enum val_type);

void
interpret(struct intpr* intpr, const char* path)
{
    intpr->path = path;
    struct stm* ast = parse(&intpr->par, path);

    printf("AST\n===\n");
    print_stm(ast);
    printf("\nOutput\n======\n");

    execute(intpr, ast);
}

void
execute(struct intpr* intpr, struct stm* stm)
/* Take a statement and produce a side effect.                               */
{
    switch (stm->type)
    {
        case STM_BLOCK:
            ;
            struct stmlist* list = stm->data.block;
            struct stmnode* node = list->nil;
            for (int i = 0; i < list->length; ++i)
            {
                node = node->next;
                execute(intpr, node->data);
            }
            break;

        case STM_EXPR_STMT:
            /* Evaluate and discard */
            evaluate(intpr, stm->data.expr.exp);
            break;

        case STM_PRINT:
            ;
            struct val val = evaluate(intpr, stm->data.print.exp);
            val_print(val);
            break;
    }
}

static void
val_print(struct val val)
{
    switch (val.type)
    {
        case VAL_BOOLEAN:
            if (val.data.as_bool == true)
                printf("true");
            else
                printf("false");
            break;

        case VAL_INTEGER:
            printf("%li", val.data.as_long);
            break;

        case VAL_DOUBLE:
            printf("%f", val.data.as_double);
            break;

        case VAL_STRING:
            printf("%s", val.data.as_string);
            break;
    }
    putchar('\n');
}

struct val
evaluate(struct intpr* intpr, struct exp* exp)
/* Take an expression and output a value                                     */
{
    switch (exp->type)
    {
        case EXP_BINARY:
            ;
            struct val left = evaluate(intpr, exp->data.binary.left);
            struct val right = evaluate(intpr, exp->data.binary.right);
            return binary_op(intpr, exp->data.binary.op, left, right);

        case EXP_UNARY:
            ;
            struct val operand = evaluate(intpr, exp->data.unary.exp);
            return unary_op(intpr, exp->data.unary.op, operand);

        case EXP_GROUP:
            return evaluate(intpr, exp->data.group.exp);

        case EXP_LITERAL:
            return val_new(intpr, exp->data.literal);

        default:
            break;
    }
}

static struct val
binary_op(struct intpr* intpr, struct tok* tok,
          struct val left, struct val right)
{
    struct val val;
    val.type = VAL_BOOLEAN;

    switch (tok->type)
    {
        case TOK_BANG_EQUAL:
            val.data.as_bool = !val_equal(left, right);
            return val;

        case TOK_EQUAL_EQUAL:
            val.data.as_bool = val_equal(left, right);
            return val;

        case TOK_GREAT:
            val.data.as_bool = val_greater(intpr, tok, left, right);
            return val;

        case TOK_LESS:
            val.data.as_bool = val_greater(intpr, tok, right, left);
            return val;

        case TOK_GREAT_EQUAL:
            /* XXX implement dedicated greater equals operation */
            val.data.as_bool = val_greater(intpr, tok, left, right) ||
                               val_equal(left, right);
            return val;

        case TOK_LESS_EQUAL:
            val.data.as_bool = val_greater(intpr, tok, right, left) ||
                               val_equal(left, right);
            return val;

        case TOK_PLUS:
            return val_add(intpr, tok, left, right);

        case TOK_MINUS:
            return val_sub(intpr, tok, left, right);

        case TOK_STAR:
            return val_mul(intpr, tok, left, right);

        case TOK_SLASH:
            return val_div(intpr, tok, left, right);

        default:
            err_at_tok(intpr->path, tok,
                "\n    Encountered an unknown binary operation `%s`.\n\n",
                tok->lexeme);
    }
}

static bool
val_equal(struct val left, struct val right)
/* Comparison table
   +---+---+---+---+---+  B = boolean
   |   | B | I | D | S |  I = integer
   +---+---+---+---+---+  D = double
   | B | ? | . | . | . |  S = string
   +---+---+---+---+---+
   | I | . | ? | ? | . |  ? = test equality
   +---+---+---+---+---+  . = trivially unequal
   | D | . | ? | ? | . |
   +---+---+---+---+---+
   | S | . | . | . | ? |
   +---+---+---+---+---+  */
{
    switch (left.type)
    {
        case VAL_BOOLEAN:
            switch (right.type)
            {
                case VAL_BOOLEAN:
                    return left.data.as_bool == right.data.as_bool;

                default:
                    return false;
            }

        case VAL_INTEGER:
            switch (right.type)
            {
                case VAL_INTEGER:
                    return left.data.as_long == right.data.as_long;

                case VAL_DOUBLE:
                    return (double)left.data.as_long == right.data.as_double;

                default:
                    return false;
            }

        case VAL_DOUBLE:
            switch (right.type)
            {
                case VAL_INTEGER:
                    return left.data.as_double == (double)right.data.as_long;

                case VAL_DOUBLE:
                    return left.data.as_double == right.data.as_double;

                default:
                    return false;
            }

        case VAL_STRING:
            switch (right.type)
            {
                case VAL_STRING:
                    return strcmp(left.data.as_string,
                                  right.data.as_string) == 0;

                default:
                    return false;
            }
    }
}

static bool
val_greater(struct intpr* intpr, struct tok* tok,
           struct val left, struct val right)
/* Ordering table
   +---+---+---+---+---+  B = boolean
   |   | B | I | D | S |  I = integer
   +---+---+---+---+---+  D = double
   | B | . | . | . | . |  S = string
   +---+---+---+---+---+
   | I | . | ? | ? | . |  ? = test ordering
   +---+---+---+---+---+  . = unorderable (error)
   | D | . | ? | ? | . |
   +---+---+---+---+---+
   | S | . | . | . | ? |
   +---+---+---+---+---+ */
{
    switch (left.type)
    {
        case VAL_INTEGER:
            switch (right.type)
            {
                case VAL_INTEGER:
                    return left.data.as_long > right.data.as_long;

                case VAL_DOUBLE:
                    return (double)left.data.as_long > right.data.as_double;

                default:
                    break;
            }
            break;

        case VAL_DOUBLE:
            switch (right.type)
            {
                case VAL_INTEGER:
                    return left.data.as_double > (double)right.data.as_long;

                case VAL_DOUBLE:
                    return left.data.as_double > right.data.as_double;

                default:
                    break;
            }
            break;

        case VAL_STRING:
            switch (right.type)
            {
                case VAL_STRING:
                    return strcmp(left.data.as_string,
                                  right.data.as_string) > 0;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    err_at_tok(intpr->path, tok,
        "\n    Tried to compare unorderable types "
        "`%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));
}

/* XXX the binary operators contain horrendous amounts of repetition */
static struct val
val_add(struct intpr* intpr, struct tok* tok,
        struct val left, struct val right)
{
    struct val val;

    switch (left.type)
    {
        case VAL_INTEGER:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_INTEGER;
                    val.data.as_long = left.data.as_long + right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        (double)left.data.as_long + right.data.as_double;
                    return val;

                default:
                    break;
            }

        case VAL_DOUBLE:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double + (double)right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double + right.data.as_double;
                    return val;

                default:
                    break;
            }

        default:
            break;
    }

    err_at_tok(intpr->path, tok,
        "\n    Failed to add incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

static struct val
val_sub(struct intpr* intpr, struct tok* tok,
        struct val left, struct val right)
{
    struct val val;

    switch (left.type)
    {
        case VAL_INTEGER:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_INTEGER;
                    val.data.as_long = left.data.as_long - right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        (double)left.data.as_long - right.data.as_double;
                    return val;

                default:
                    break;
            }

        case VAL_DOUBLE:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double - (double)right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double - right.data.as_double;
                    return val;

                default:
                    break;
            }

        default:
            break;
    }

    err_at_tok(intpr->path, tok,
        "\n    Failed to substract incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

static struct val
val_mul(struct intpr* intpr, struct tok* tok,
        struct val left, struct val right)
{
    struct val val;

    switch (left.type)
    {
        case VAL_INTEGER:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_INTEGER;
                    val.data.as_long = left.data.as_long * right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        (double)left.data.as_long * right.data.as_double;
                    return val;

                default:
                    break;
            }

        case VAL_DOUBLE:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double * (double)right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double * right.data.as_double;
                    return val;

                default:
                    break;
            }

        default:
            break;
    }

    err_at_tok(intpr->path, tok,
        "\n    Failed to multiply incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

static struct val
val_div(struct intpr* intpr, struct tok* tok,
        struct val left, struct val right)
{
    struct val val;

    switch (left.type)
    {
        case VAL_INTEGER:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_INTEGER;
                    val.data.as_long = left.data.as_long / right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        (double)left.data.as_long / right.data.as_double;
                    return val;

                default:
                    break;
            }

        case VAL_DOUBLE:
            switch (right.type)
            {
                case VAL_INTEGER:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double / (double)right.data.as_long;
                    return val;

                case VAL_DOUBLE:
                    val.type = VAL_DOUBLE;
                    val.data.as_double =
                        left.data.as_double / right.data.as_double;
                    return val;

                default:
                    break;
            }

        default:
            break;
    }

    err_at_tok(intpr->path, tok,
        "\n    Failed to divide incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

static struct val
unary_op(struct intpr* intpr, struct tok* tok, struct val operand)
/*  Unary table
    +---+---+---+---+---+  B = boolean
    |   | B | I | D | S |  I = integer
    +---+---+---+---+---+  D = double
    | - | . | ? | ? | . |  S = string
    +---+---+---+---+---+
    | ! | ? | ? | ? | ? |  ? = test unary
    +---+---+---+---+---+  . = unnegatable (error)                           */
{
    switch (tok->type)
    {
        case TOK_MINUS:
            switch (operand.type)
            {
                case VAL_INTEGER:
                    operand.data.as_long = -operand.data.as_long;
                    break;

                case VAL_DOUBLE:
                    operand.data.as_double = -operand.data.as_double;
                    break;

                default:
                    err_at_tok(intpr->path, tok,
                        "\n    Tried to negate the unnegatable type `%s`.\n\n",
                        val_type_str(operand.type));
                    break;
            }
            break;

        case TOK_BANG:
            switch (operand.type)
            {
                case VAL_BOOLEAN:
                    operand.data.as_bool = !operand.data.as_bool;
                    break;

                default:
                    operand.type = VAL_BOOLEAN;
                    operand.data.as_bool = false;
                    break;
            }
            break;

        default:
            err_at_tok(intpr->path, tok,
                "\n    Encountered an unknown unary operation `%s`.\n\n",
                tok->lexeme);
            break;
    }

    return operand;
}

struct val
val_new(struct intpr* intpr, struct tok* tok)
{
    struct val val;

    switch (tok->type)
    {
        case TOK_TRUE:
            val.type = VAL_BOOLEAN;
            val.data.as_bool = true;
            break;

        case TOK_FALSE:
            val.type = VAL_BOOLEAN;
            val.data.as_bool = false;
            break;

        case TOK_INTEGER:
            val.type = VAL_INTEGER;
            /* XXX this will silently fail for large enough numbers */
            val.data.as_long = atol(tok->lexeme);
            break;

        case TOK_DOUBLE:
            val.type = VAL_DOUBLE;
            val.data.as_double = atof(tok->lexeme);
            break;

        case TOK_STRING:
            val.type = VAL_STRING;
            val.data.as_string = tok->lexeme;
            break;

        default:
            err_at_tok(intpr->path, tok,
                "\n    Expected a literal value, but instead got `%s`.\n\n",
                tok->lexeme);
            break;
    }

    return val;
}

static const char*
val_type_str(enum val_type type)
{
    switch (type)
    {
        case VAL_BOOLEAN:
            return "boolean";
        case VAL_INTEGER:
            return "integer";
        case VAL_DOUBLE:
            return "fractional";
        case VAL_STRING:
            return "string";
    }
}

