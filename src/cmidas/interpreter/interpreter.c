#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "environ.h"
#include "interpreter.h"
#include "lexer.h"
#include "value.h"
#include "vector.h"

#define T Interpr_T

struct brkpnt
{
    struct brkpnt *prev;
        Environ_T  context;
          jmp_buf  point;
};

struct T
{
    struct brkpnt *brkpnt;
        Environ_T  globals;
        Environ_T  context;
       struct val  ret_val;
};

static void ctx_push(T intpr);
static void ctx_pop (T intpr);
static void brk_push(T intpr, Environ_T env);
static void brk_pop (T intpr);

static       void execute (T intpr, struct stm *);
static struct val evaluate(T intpr, struct exp *);
static struct val exp_call(T intpr, struct exp *);

static void var_decl(T intpr, const char *name, struct val *val);

T
Interpr_new(void)
{
    struct val val;
         T intpr;

    val.type = VAL_NIL;

    intpr = malloc(sizeof(struct T));
    intpr->globals = Environ_new(NULL);
    intpr->context = intpr->globals;
    intpr->brkpnt = NULL;
    intpr->ret_val = val;

    return intpr;
}

void
Interpr_run(T intpr, Vector_T ast)
{
    int i, len;

    len = Vector_length(ast);
    for (i = 0; i < len; ++i)
        execute(intpr, *(struct stm **)Vector_get(ast, i));
}

static void
ctx_push(T intpr)
{
    Environ_T new_ctx;

    new_ctx = Environ_new(intpr->context);
    intpr->context = new_ctx;
}

static void
ctx_pop(T intpr)
{
    Environ_T old_ctx;

    old_ctx = intpr->context;
    intpr->context = Environ_parent(old_ctx);

    Environ_free(old_ctx);
}

static void
brk_push(T intpr, Environ_T env)
{
    struct brkpnt *brkpnt;

    brkpnt = malloc(sizeof(struct brkpnt));
    brkpnt->prev = intpr->brkpnt;
    brkpnt->context = env;
    intpr->brkpnt = brkpnt;
}

static void
brk_pop(T intpr)
{
    struct brkpnt *oldpnt;

    oldpnt = intpr->brkpnt;
    intpr->brkpnt = intpr->brkpnt->prev;

    free(oldpnt);
}

static void
execute(T intpr, struct stm *stm)
/*
 *  Take a statement and produce a side effect.
 */
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
            if (Val_is_truthy(cond))
                execute(intpr, stm->data.if_cond.then_block);
            else if (stm->data.if_cond.else_block != NULL)
                execute(intpr, stm->data.if_cond.else_block);
        } break;

        case STM_WHILE:
        {
            brk_push(intpr, intpr->context);

            if (setjmp(intpr->brkpnt->point) == 0)
            {
                while
                (
                    Val_is_truthy(evaluate(intpr, stm->data.while_cond.cond))
                )
                {
                    execute(intpr, stm->data.while_cond.body);
                }
            }
            else
            {
                while (intpr->context != intpr->brkpnt->context)
                    ctx_pop(intpr);
            }

            brk_pop(intpr);
        }
        break;

        case STM_BREAK:
            longjmp(intpr->brkpnt->point, 1);
        break;

        case STM_RETURN:
        {
            intpr->ret_val = evaluate(intpr, stm->data.ret_exp);
            longjmp(intpr->brkpnt->point, 1);
        } break;

        case STM_VAR_DECL:
        {
            struct val *val;

            val = malloc(sizeof(struct val));
            *val = evaluate(intpr, stm->data.var_decl.exp);
            var_decl(intpr, stm->data.var_decl.name, val);
        } break;

        case STM_FUN_DECL:
        {
            struct val *fun;

            fun = Val_new_fun(stm);
            var_decl(intpr, stm->data.fun_decl.name, fun);
        } break;

        case STM_PRINT:
        {
            struct val val = evaluate(intpr, stm->data.print);
            Val_print(val);
        } break;

        case STM_EXP_STM:
            evaluate(intpr, stm->data.exp_stm); /* Evaluate and discard */
        break;

        default:
            printf("Encoutered unknown statement\n");
            exit(1);
        break;
    }
}

struct val
evaluate(T intpr, struct exp *exp)
/* Take an expression and output a value */
{
    struct val val;

    switch (exp->type)
    {
        case EXP_ASSIGN:
        {
            const char *name;
            struct val *valp, *prev;

            name = exp->data.assign.name;
            val = evaluate(intpr, exp->data.assign.exp);

            valp = malloc(sizeof(struct val));
            *valp = val;

            /* XXX should deallocate prev value */
            prev = Environ_var_set(intpr->context, name, valp);
            if (prev == NULL)
            {
                printf("Cannot assign to undeclared variable `%s`.\n",
                       name);
                exit(1);
            }
        } break;

        case EXP_BINARY:
        {
            struct val left, right;

            left = evaluate(intpr, exp->data.binary.left);
            right = evaluate(intpr, exp->data.binary.right);
            val = Val_binop(exp->data.binary.op, left, right);
        } break;

        case EXP_UNARY:
        {
            struct val operand;

            operand = evaluate(intpr, exp->data.unary.exp);
            val = Val_unop(exp->data.unary.op, operand);
        } break;

        case EXP_IDENT:
        {
            const char *name;
            struct val *valp;

            name = exp->data.name;
            valp = Environ_var_get(intpr->context, name);
            if (valp != NULL)
                val = *valp;
            else
            {
                printf("`%s` is not declared in this scope. "
                       "A varible needs to be declared prior to its usage.\n",
                       name);
                exit(1);
            }
        } break;

        case EXP_CALL:
            val = exp_call(intpr, exp);
        break;

        case EXP_LITERAL:
            val = Val_new(exp->data.literal);
        break;
    }

    return val;
}

void
var_decl(T intpr, const char *name, struct val *val)
{
    struct val *prev;

    prev = Environ_var_new(intpr->context, name, val);
    if (prev != NULL)
    {
        printf("`%s` is already declared in this scope."
               "Use assigment if you want to change the variable's value."
               "Use a different name if you want to declare a new variable.\n",
               name);
        exit(1);
    }
}

struct val
exp_call(T intpr, struct exp *exp)
{
    struct val fun, ret_val;
           int i, arity;
      Vector_T args;
     Environ_T prev_env;

    fun = evaluate(intpr, exp->data.call.callee);
    if (fun.type != VAL_FUNCTION)
    {
        printf("Tried to call a non-function variable: ");
        Val_print(fun);
        exit(1);
    }

    arity = Vector_length(exp->data.call.params);
    if (arity != fun.data.as_fun->arity)
    {
        puts("Called function with wrong number of arguments.");
        exit(1);
    }

    args = Vector_new(sizeof(struct val));
    for (i = 0; i < arity; ++i)
    {
        struct val arg;

        arg = evaluate(intpr,
                       *(struct exp **)Vector_get(exp->data.call.params, i));
        Vector_push(args, &arg);
    }

    prev_env = intpr->context;
    intpr->context = Environ_new(intpr->globals);

    for (i = 0; i < arity; ++i)
    {
        var_decl(intpr,
                 *(const char **)Vector_get(fun.data.as_fun->params, i),
                 Vector_get(args, i));
    }

    brk_push(intpr, intpr->globals);
    if (setjmp(intpr->brkpnt->point) == 0)
        execute(intpr, fun.data.as_fun->body);
    else
    {
        while (intpr->context != intpr->brkpnt->context)
            ctx_pop(intpr);
    }
    brk_pop(intpr);

    intpr->context = prev_env;
    ret_val = intpr->ret_val;
    intpr->ret_val.type = VAL_NIL;

    return ret_val;
}

#undef T

