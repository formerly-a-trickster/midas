#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "interpreter.h"
#include "parser.h"
#include "value.h"

/*
enum val_type
{
    VAL_BOOLEAN,
    VAL_INTEGER,
    VAL_DOUBLE,
    VAL_STRING
};

struct val
{
    enum val_type type;

    union
    {
        bool as_bool;
        long as_long;
        double as_double;
        const char* as_string;
    } data;
};
*/
static struct val val_add(Interpr_T intpr, struct tok *, struct val, struct val);
static struct val val_sub(Interpr_T intpr, struct tok *, struct val, struct val);
static struct val val_mul(Interpr_T intpr, struct tok *, struct val, struct val);
static struct val val_div(Interpr_T intpr, struct tok *, struct val, struct val);

static bool val_equal  (struct val, struct val);
static bool val_greater(Interpr_T intpr, struct tok *, struct val, struct val);

static const char *val_type_str(enum val_type type);

struct val
val_new(Interpr_T intpr, struct tok *tok)
{
    struct val val;

    switch (tok->type)
    {
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

        case TOK_TRUE:
            val.type = VAL_BOOLEAN;
            val.data.as_bool = true;
            break;

        case TOK_FALSE:
            val.type = VAL_BOOLEAN;
            val.data.as_bool = false;
            break;

        default:
            err_at_tok("", tok,
                "\n    Expected a literal value, but instead got `%s`.\n\n",
                tok->lexeme);
            break;
    }

    return val;
}

bool
val_is_truthy(struct val val)
{
    return !(val.type == VAL_BOOLEAN && val.data.as_bool == false);
}

struct val
binary_op(Interpr_T intpr, struct tok *tok,
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
            err_at_tok("", tok,
                "\n    Encountered an unknown binary operation `%s`.\n\n",
                tok->lexeme);
    }
}

struct val
unary_op(Interpr_T intpr, struct tok *tok, struct val operand)
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
                    err_at_tok("", tok,
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
            err_at_tok("", tok,
                "\n    Encountered an unknown unary operation `%s`.\n\n",
                tok->lexeme);
            break;
    }

    return operand;
}

void
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

bool
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

bool
val_greater(Interpr_T intpr, struct tok *tok,
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

    err_at_tok("", tok,
        "\n    Tried to compare unorderable types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));
}

/* XXX the binary operators contain horrendous amounts of repetition */
struct val
val_add(Interpr_T intpr, struct tok *tok,
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

    err_at_tok("", tok,
        "\n    Failed to add incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

struct val
val_sub(Interpr_T intpr, struct tok *tok,
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

    err_at_tok("", tok,
        "\n    Failed to substract incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

struct val
val_mul(Interpr_T intpr, struct tok *tok,
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

    err_at_tok("", tok,
        "\n    Failed to multiply incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

struct val
val_div(Interpr_T intpr, struct tok *tok,
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

    err_at_tok("", tok,
        "\n    Failed to divide incompatible types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));

    return val; /* Unreachable */
}

static const char *
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

