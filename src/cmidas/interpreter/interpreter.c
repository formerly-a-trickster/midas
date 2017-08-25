#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "error.h"
#include "interpreter.h"
#include "parser.h"
#include "vector.h"

#define T Interpreter_T

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

struct T
{
    const char *path;
         Env_T  globals;
         Env_T  context;
};

static void ctx_push(T intpr);
static void ctx_pop (T intpr);

static void execute      (T intpr, struct stm *);
static bool val_is_truthy(struct val);
static void var_decl     (T intpr, struct tok *, struct val *);
static void val_print    (struct val);

static struct val evaluate (T intpr, struct exp *);
static struct val binary_op(T intpr, struct tok *, struct val, struct val);
static struct val unary_op (T intpr, struct tok *, struct val);
static bool       val_equal(struct val, struct val);
static bool       val_greater(T intpr, struct tok *, struct val, struct val);
static struct val val_add(T intpr, struct tok *, struct val, struct val);
static struct val val_sub(T intpr, struct tok *, struct val, struct val);
static struct val val_mul(T intpr, struct tok *, struct val, struct val);
static struct val val_div(T intpr, struct tok *, struct val, struct val);

static struct val val_new(T , struct tok *);
static const char *val_type_str(enum val_type);

T
Intpr_new(void)
{
    T intpr = malloc(sizeof(struct T));
    intpr->path = NULL;
    intpr->globals = Env_new(NULL);
    intpr->context = intpr->globals;

    return intpr;
}

void
Intpr_run(T intpr, const char *path, Vector_T ast)
{
    int i, len;

    intpr->path = path;

    len = Vector_length(ast);
    for (i = 0; i < len; ++i)
        execute(intpr, *(struct stm **)Vector_get(ast, i));
}

static void
ctx_push(T intpr)
{
    Env_T new_ctx;

    new_ctx = Env_new(intpr->context);
    intpr->context = new_ctx;
}

static void
ctx_pop(T intpr)
{
    Env_T old_ctx;

    old_ctx = intpr->context;
    intpr->context = Env_parent(old_ctx);

    Env_free(old_ctx);
}

static void
execute(T intpr, struct stm *stm)
/* Take a statement and produce a side effect.                               */
{
    switch (stm->type)
    {
        case STM_BLOCK:
        {
            Vector_T vector;
            int i, len;

            ctx_push(intpr);

            vector = stm->data.block;
            len = Vector_length(vector);
            for (i = 0; i < len; ++i)
                execute(intpr, *(struct stm**)Vector_get(vector, i));

            ctx_pop(intpr);
        } break;

        case STM_IF:
        {
            struct val cond;

            cond = evaluate(intpr, stm->data.if_cond.cond);
            if (val_is_truthy(cond))
                execute(intpr, stm->data.if_cond.then_block);
            else if (stm->data.if_cond.else_block != NULL)
                execute(intpr, stm->data.if_cond.else_block);
        } break;

        case STM_WHILE:
        {
            while(val_is_truthy(evaluate(intpr, stm->data.while_cond.cond)))
                execute(intpr, stm->data.while_cond.body);
        } break;

        case STM_VAR_DECL:
        {
            struct val *var;

            var = malloc(sizeof(struct val));
            *var = evaluate(intpr, stm->data.var_decl.exp);
            var_decl(intpr, stm->data.var_decl.name, var);
        } break;

        case STM_EXP_STM:
            /* Evaluate and discard */
            evaluate(intpr, stm->data.exp.exp);
        break;

        case STM_PRINT:
        {
            struct val val = evaluate(intpr, stm->data.print.exp);
            val_print(val);
        } break;
    }
}

static bool
val_is_truthy(struct val val)
{
    return !(val.type == VAL_BOOLEAN && val.data.as_bool == false);
}

static void
var_decl(T intpr, struct tok *name, struct val *val)
{
    struct val *prev = Env_var_new(intpr->context, name->lexeme, val);
    if (prev != NULL)
        err_at_tok(intpr->path, name,
            "\n    `%s` is already declared in this scope."
            "\n    Use assigment if you want to change the variable's value."
            "\n    Use a different name if you want to declare a new variable."
            "\n\n", name->lexeme);
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
evaluate(T intpr, struct exp *exp)
/* Take an expression and output a value                                     */
{
    struct val val;

    switch (exp->type)
    {
        case EXP_ASSIGN:
        {
            struct tok *name;
            struct val *valp, *prev;

            name = exp->data.assign.name;
            val = evaluate(intpr, exp->data.assign.exp);

            valp = malloc(sizeof(struct val));
            *valp = val;

            /* XXX should deallocate prev value */
            prev = Env_var_set(intpr->context, name->lexeme, valp);
            if (prev == NULL)
                err_at_tok(intpr->path, name,
                    "\n    Cannot assign to undeclared variable `%s`.\n\n",
                    name->lexeme);
        } break;

        case EXP_BINARY:
        {
            struct val left, right;

            left = evaluate(intpr, exp->data.binary.left);
            right = evaluate(intpr, exp->data.binary.right);
            val = binary_op(intpr, exp->data.binary.op, left, right);
        } break;

        case EXP_UNARY:
        {
            struct val operand;

            operand = evaluate(intpr, exp->data.unary.exp);
            val = unary_op(intpr, exp->data.unary.op, operand);
        } break;

        case EXP_GROUP:
            val = evaluate(intpr, exp->data.group.exp);
        break;

        case EXP_VAR:
        {
            struct tok *name;
            struct val *valp;

            name = exp->data.name;
            valp = Env_var_get(intpr->context, name->lexeme);
            if (valp != NULL)
                val = *valp;
            else
                err_at_tok(intpr->path, name,
                    "\n    `%s` is not declared in this scope."
                    "\n    A varible needs to be declared prior to its "
                        "usage.\n\n",
                    name->lexeme);
        } break;

        case EXP_LITERAL:
            val = val_new(intpr, exp->data.literal);
        break;
    }

    return val;
}

static struct val
binary_op(T intpr, struct tok *tok,
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
val_greater(T intpr, struct tok *tok,
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
        "\n    Tried to compare unorderable types `%s` and `%s`.\n\n",
        val_type_str(left.type), val_type_str(right.type));
}

/* XXX the binary operators contain horrendous amounts of repetition */
static struct val
val_add(T intpr, struct tok *tok,
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
val_sub(T intpr, struct tok *tok,
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
val_mul(T intpr, struct tok *tok,
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
val_div(T intpr, struct tok *tok,
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
unary_op(T intpr, struct tok *tok, struct val operand)
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
val_new(T intpr, struct tok *tok)
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
            err_at_tok(intpr->path, tok,
                "\n    Expected a literal value, but instead got `%s`.\n\n",
                tok->lexeme);
            break;
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
        /* unreachable */
        default:
            return NULL;
    }
}

#undef T

