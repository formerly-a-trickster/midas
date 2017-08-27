/* XXX replace with custom printinf function */
#define _GNU_SOURCE  /* asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
static const char *val_type_str(enum val_type type);

struct val Val_to_type(struct val val, enum val_type type);
      void Val_adapt  (struct val *left, struct val *right);
      bool Val_is_num (struct val val);

struct val Val_add  (struct val left, struct val right);
struct val Val_sub  (struct val left, struct val right);
struct val Val_mul  (struct val left, struct val right);
struct val Val_div  (struct val left, struct val right);
struct val Val_equal(struct val left, struct val right);
struct val Val_great(struct val left, struct val right);
struct val Val_gr_eq(struct val left, struct val right);

struct val Val_log_negate(struct val val);
struct val Val_num_negate(struct val val);

struct val
val_new(struct tok *tok)
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
            printf("Expected a literal value, but instead got `%s`.\n",
                tok->lexeme);
            exit(1);
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
binary_op(struct tok *tok, struct val left, struct val right)
{
    switch (tok->type)
    {
        case TOK_BANG_EQUAL:
            return Val_log_negate(Val_equal(left, right));

        case TOK_EQUAL_EQUAL:
            return Val_equal(left, right);

        case TOK_GREAT:
            return Val_great(left, right);

        case TOK_LESS:
            return Val_great(right, left);

        case TOK_GREAT_EQUAL:
            return Val_gr_eq(left, right);

        case TOK_LESS_EQUAL:
            return Val_gr_eq(right, left);

        case TOK_PLUS:
            return Val_add(left, right);

        case TOK_MINUS:
            return Val_sub(left, right);

        case TOK_STAR:
            return Val_mul(left, right);

        case TOK_SLASH:
            return Val_div(left, right);

        default:
            printf("Encountered an unknown binary operation `%s`.\n",
                   tok->lexeme);
            exit(1);
            break;
    }
}

struct val
unary_op(struct tok *tok, struct val operand)
{
    switch (tok->type)
    {
        case TOK_MINUS:
            return Val_num_negate(operand);

        case TOK_BANG:
            return Val_log_negate(operand);

        default:
            printf("Encountered an unknown unary operation `%s`.\n",
                tok->lexeme);
            exit(0);
    }
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

struct val
Val_to_type(struct val val, enum val_type to_type)
{
    if (val.type != to_type)
    {
        if (to_type == VAL_BOOLEAN)
        {
            val.type = VAL_BOOLEAN;
            val.data.as_bool = true;
        }
        else if (to_type == VAL_STRING)
        {
            if (val.type == VAL_BOOLEAN)
            {
                val.type = VAL_STRING;
                if (val.data.as_bool)
                    val.data.as_string = "true";
                else
                    val.data.as_string = "false";
            }
            else if (val.type == VAL_INTEGER)
            {
                char *int_string;

                asprintf(&int_string, "%li", val.data.as_long);
                val.type = VAL_STRING;
                val.data.as_string = int_string;
            }
            else /* val.type == VAL_DOUBLE */
            {
                char *double_string;

                asprintf(&double_string, "%f", val.data.as_double);
                val.type = VAL_STRING;
                val.data.as_string = double_string;
            }
        }
        else if (to_type == VAL_INTEGER)
        {
            if (val.type == VAL_DOUBLE)
            {
                val.type = VAL_INTEGER;
                val.data.as_long = (long)val.data.as_double;
            }
            else
            {
                printf("Tried to convert %s value to %s value.\n",
                       val_type_str(val.type), val_type_str(to_type));
                exit(1);
            }
        }
        else /* to_type == VAL_DOUBLE */
        {
            if (val.type == VAL_INTEGER)
            {
                val.type = VAL_DOUBLE;
                val.data.as_double = (double)val.data.as_long;
            }
            else
            {
                printf("Tried to convert %s value to %s value.\n",
                       val_type_str(val.type), val_type_str(to_type));
                exit(1);
            }
        }
    }

    return val;
}

void
Val_adapt(struct val *left, struct val *right)
{
    enum tok_type promotion;

    promotion = left->type > right->type ? left->type : right->type;

    *left = Val_to_type(*left, promotion);
    *right = Val_to_type(*right, promotion);
}

bool
Val_is_num(struct val val)
{
    return val.type == VAL_INTEGER || val.type == VAL_DOUBLE;
}

#define Val_bin_op(fun_name, op_name, op_symbol)                              \
    struct val                                                                \
    Val_##fun_name(struct val left, struct val right)                         \
    {                                                                         \
        if (Val_is_num(left) && Val_is_num(right))                            \
        {                                                                     \
            Val_adapt(&left, &right);                                         \
                                                                              \
            if (left.type == VAL_INTEGER)                                     \
                left.data.as_long op_symbol##= right.data.as_long;            \
            else /* left.type = VAL_DOUBLE */                                 \
                left.data.as_double op_symbol##= right.data.as_double;        \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            printf("Tried to op_name incompatible types "                     \
                   "`%s` op_symbol `%s`.\n",                                  \
                   val_type_str(left.type), val_type_str(right.type));        \
            exit(1);                                                          \
        }                                                                     \
                                                                              \
        return left;                                                          \
    }

Val_bin_op(add, add      , +)
Val_bin_op(sub, substract, -)
Val_bin_op(mul, multiply , *)
Val_bin_op(div, divide   , /)

#undef Val_bin_op

struct val
Val_equal(struct val left, struct val right)
{
    struct val val;

    val.type = VAL_BOOLEAN;
    if (left.type == right.type)
    {
        if (left.type == VAL_STRING)
            val.data.as_bool = strcmp
            (
                left.data.as_string,
                right.data.as_string
            ) == 0;
        else
            val.data.as_bool = left.data.as_long == right.data.as_long;
    }
    else if (Val_is_num(left) && Val_is_num(right))
    {
        Val_adapt(&left, &right);

        val.data.as_bool = left.data.as_long == right.data.as_long;
    }
    else
        val.data.as_bool = false;

    return val;
}

struct val
Val_great(struct val left, struct val right)
{
    struct val val;

    val.type = VAL_BOOLEAN;
    if (left.type == right.type)
    {
        if (left.type == VAL_STRING)
            val.data.as_bool = strcmp
            (
                left.data.as_string,
                right.data.as_string
            ) > 0;
        else if (left.type == VAL_INTEGER)
            val.data.as_bool = left.data.as_long > right.data.as_long;
        else if (left.type == VAL_DOUBLE)
            val.data.as_bool = left.data.as_double > right.data.as_double;
    }
    else if (Val_is_num(left) && Val_is_num(right))
    {
        Val_adapt(&left, &right);

        val.data.as_bool = left.data.as_long > right.data.as_long;
    }
    else
    {
        printf("Tried to compare incompatible types `%s` > `%s`.\n",
               val_type_str(left.type), val_type_str(right.type));
        exit(1);
    }

    return val;
}

struct val
Val_gr_eq(struct val left, struct val right)
{
    struct val val;

    val.type = VAL_BOOLEAN;
    if (left.type == right.type)
    {
        if (left.type == VAL_STRING)
            val.data.as_bool = strcmp
            (
                left.data.as_string,
                right.data.as_string
            ) >= 0;
        else if (left.type == VAL_INTEGER)
            val.data.as_bool = left.data.as_long >= right.data.as_long;
        else if (left.type == VAL_DOUBLE)
            val.data.as_bool = left.data.as_double >= right.data.as_double;
    }
    else if (Val_is_num(left) && Val_is_num(right))
    {
        Val_adapt(&left, &right);

        val.data.as_bool = left.data.as_long >= right.data.as_long;
    }
    else
    {
        printf("Tried to compare incompatible types `%s` >= `%s`.\n",
               val_type_str(left.type), val_type_str(right.type));
        exit(1);
    }

    return val;
}

struct val
Val_log_negate(struct val val)
{
    if (val.type == VAL_BOOLEAN)
        val.data.as_bool = !val.data.as_bool;
    else
    {
        val.type = VAL_BOOLEAN;
        val.data.as_bool = false;
    }

    return val;
}

struct val
Val_num_negate(struct val val)
{
    if (val.type == VAL_INTEGER)
        val.data.as_long = -val.data.as_long;
    else if (val.type == VAL_DOUBLE)
        val.data.as_double = -val.data.as_double;
    else
    {
        printf("Tried to negate non number type `%s`.\n",
               val_type_str(val.type));
        exit(1);
    }

    return val;
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
        default:
            return "<unknown>";
    }
}

