#include <stdio.h>
#include <stdlib.h>

#include "environment.h"
#include "error.h"
#include "interpreter.h"
#include "value.h"
#include "vector.h"

#define T Interpr_T

struct T
{
    const char *path;
     Environ_T  globals;
     Environ_T  context;
};

static void ctx_push(T intpr);
static void ctx_pop (T intpr);

static       void execute (T intpr, struct stm *);
static struct val evaluate(T intpr, struct exp *);

static void var_decl(T intpr, struct tok *name, struct val *val);

T
Interpr_new(void)
{
    T intpr = malloc(sizeof(struct T));
    intpr->path = NULL;
    intpr->globals = Env_new(NULL);
    intpr->context = intpr->globals;

    return intpr;
}

void
Interpr_run(T intpr, const char *path, Vector_T ast)
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
    Environ_T new_ctx;

    new_ctx = Env_new(intpr->context);
    intpr->context = new_ctx;
}

static void
ctx_pop(T intpr)
{
    Environ_T old_ctx;

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

void
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

#undef T

