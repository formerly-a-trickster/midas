#include <stdbool.h>
#include <stdlib.h>

#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "vector.h"

         Vector_T  program    (struct par_state *);
static struct stm *declaration(struct par_state *);
static struct stm *statement  (struct par_state *);
static struct stm *block      (struct par_state *);
static struct stm *if_cond    (struct par_state *);
static struct stm *while_cond (struct par_state *);
static struct stm *var_decl   (struct par_state *);
static struct stm *expr_stmt  (struct par_state *);
static struct stm *print      (struct par_state *);

static struct exp *expression    (struct par_state *);
static struct exp *assignment    (struct par_state *);
static struct exp *equality      (struct par_state *);
static struct exp *ordering      (struct par_state *);
static struct exp *addition      (struct par_state *);
static struct exp *multiplication(struct par_state *);
static struct exp *unary         (struct par_state *);
static struct exp *primary       (struct par_state *);

static struct tok *tok_next   (struct par_state *);
       static bool tok_matches(struct par_state *, enum tok_type);
inline static bool tok_is     (struct par_state *, enum tok_type);
inline static bool tok_was    (struct par_state *, enum tok_type);

static struct stm *stm_new_block    (Vector_T);
static struct stm *stm_new_if       (struct exp *, struct stm *, struct stm *);
static struct stm *stm_new_while    (struct exp *, struct stm *);
static struct stm *stm_new_var_decl (struct tok *, struct exp *);
static struct stm *stm_new_print    (struct exp *, struct tok *);
static struct stm *stm_new_expr_stmt(struct exp *, struct tok *);

static struct exp *exp_new_assign (struct tok *, struct exp *);
static struct exp *exp_new_binary (struct tok *, struct exp *, struct exp *);
static struct exp *exp_new_unary  (struct tok *, struct exp *);
static struct exp *exp_new_group  (struct exp *, struct tok *, struct tok *);
static struct exp *exp_new_var    (struct tok *);
static struct exp *exp_new_literal(struct tok *);

Vector_T
parse(struct par_state *par, const char *path)
{
    lex_init(&par->lex);
    par->path = path;
    par->prev_tok = NULL;
    par->this_tok = NULL;

    lex_feed(&par->lex, path);
    tok_next(par);

    return program(par);
}

Vector_T
program(struct par_state *par)
/*  program -> statement* "EOF"                                              */
{
    Vector_T program;

    program = Vector_new(sizeof(struct stm *));
    /* XXX if we would have used !tok_matches(par, TOK_EOF), we would force the
       lexer to read beyond the EOF upon finally matching it */
    while(!tok_is(par, TOK_EOF))
    {
        struct stm *stm = declaration(par);
        Vector_push(program, &stm);
    }

    return program;
}

static struct stm *
declaration(struct par_state *par)
/*  declaration -> var_decl ";"
                 | statement                                                 */
{
    struct stm *stm;

    if (tok_matches(par, TOK_VAR))
        stm = var_decl(par);
    else
        stm = statement(par);

    return stm;
}

static struct stm *
var_decl(struct par_state *par)
/*  var_decl -> ^var^ identifier "=" expression ";"                          */
{
    struct stm *stm;

    if (tok_matches(par, TOK_IDENTIFIER))
    {
        struct tok *name = par->prev_tok;
        if (tok_matches(par, TOK_EQUAL))
        {
            struct exp *value = expression(par);
            stm = stm_new_var_decl(name, value);
        }
        else
            err_at_tok(par->path, par->prev_tok,
                "\n    An equal sign should follow the variable's name.\n\n");
    }
    else
        err_at_tok(par->path, par->prev_tok,
            "\n    A variable name should follow the `var` keyword.\n\n");

    if (!tok_matches(par, TOK_SEMICOLON))
        err_at_tok(par->path, par->prev_tok,
            "\n    Missing semicolon after variable declaration.\n\n");

    return stm;
}

static struct stm *
statement(struct par_state *par)
/*  statement -> block_stm
               | if_stm
               | while_stm
               | print_stm
               | expr_stmt                                                   */
{
    struct stm *stm;

    if (tok_matches(par, TOK_DO))
        stm = block(par);
    else if (tok_matches(par, TOK_IF))
        stm = if_cond(par);
    else if (tok_matches(par, TOK_WHILE))
        stm = while_cond(par);
    else if (tok_matches(par, TOK_PRINT))
        stm = print(par);
    else
        stm = expr_stmt(par);

    return stm;
}

static struct stm *
block(struct par_state *par)
/*  block_stm -> ^do^ declaration* "end"                                     */
{
    Vector_T statements;

    statements = Vector_new(sizeof(struct stm *));
    while (!tok_is(par, TOK_END) && !tok_is(par, TOK_EOF))
    {
        struct stm *stm = declaration(par);
        Vector_push(statements, &stm);
    }

    if (!tok_matches(par, TOK_END))
        err_at_tok(par->path, par->prev_tok,
            "\n    Missing `end` keyword after block.\n\n");

    return stm_new_block(statements);
}

static struct stm *
if_cond(struct par_state *par)
/*  if_stm -> ^if^ "(" expression ")" statement ( "else" statement )?        */
{
    struct stm *stm;

    if (tok_matches(par, TOK_PAREN_LEFT))
    {
        struct exp *cond;

        cond = expression(par);

        if (tok_matches(par, TOK_PAREN_RIGHT))
        {
            struct stm *then_block;
            struct stm *else_block;

            then_block = statement(par);

            if (tok_matches(par, TOK_ELSE))
                else_block = statement(par);
            else
                else_block = NULL;

            stm = stm_new_if(cond, then_block, else_block);
        }
        else
            err_at_tok(par->path, par->prev_tok,
                "\n    Expected a closing paren after the if condition.\n\n");
    }
    else
        err_at_tok(par->path, par->prev_tok,
            "\n    Expected an opening paren after `if` keyword.\n\n");

    return stm;
}

static struct stm *
while_cond(struct par_state *par)
/*  while_stm -> ^while^ "(" expression ")" statement                        */
{
    struct stm *stm;

    if (tok_matches(par, TOK_PAREN_LEFT))
    {
        struct exp *cond;

        cond = expression(par);

        if (tok_matches(par, TOK_PAREN_RIGHT))
        {
            struct stm *body;

            body = statement(par);
            stm = stm_new_while(cond, body);
        }
        else
            err_at_tok(par->path, par->prev_tok,
                "\n    Expected a closing paren after the while "
                "condition.\n\n");
    }
    else
        err_at_tok(par->path, par->prev_tok,
            "\n    Expected an opening paren after `while` keyword.\n\n");

    return stm;
}

static struct stm *
print(struct par_state *par)
/*  print_stm -> ^print^ expression ";"                                      */
{
    struct exp *exp;

    exp = expression(par);
    if (!tok_matches(par, TOK_SEMICOLON))
        err_at_tok(par->path, par->prev_tok,
            "\n    Missing semicolon after print statement.\n\n");

    return stm_new_print(exp, par->prev_tok);
}

static struct stm *
expr_stmt(struct par_state *par)
/*  expr_stmt -> expression ";"                                              */
{
    struct exp *exp;

    exp = expression(par);
    if (!tok_matches(par, TOK_SEMICOLON))
        err_at_tok(par->path, par->prev_tok,
            "\n    Missing semicolon after expression statement.\n\n");

    return stm_new_expr_stmt(exp, par->prev_tok);
}

static struct exp *
expression(struct par_state *par)
/*  expression -> assignment                                                 */
{
    return assignment(par);
}

static struct exp *
assignment(struct par_state *par)
/* XXX having assignment be an expression simplifies the grammar, but puts us
   in the situation of having an expression with a side effect. It would be
   suited better to being a statement                                        */
/*  assignment -> equality ( "=" assignment )*                               */
{
    struct exp *left = equality(par);

    if (tok_matches(par, TOK_EQUAL))
    {
        struct tok *eq = par->prev_tok;
        struct exp *exp = assignment(par);

        if (left->type == EXP_VAR)
        {
            struct tok *varname = left->data.name;
            free(left);
            return exp_new_assign(varname, exp);
        }
        else
            err_at_tok(par->path, eq,
                "\n    Invalid assignment target. "
                "Expected a variable name.\n\n");
    }

    return left;
}

static struct exp *
equality(struct par_state *par)
/*  equality -> ordering ( ( "!=" | "==" ) ordering )*                       */
{
    struct exp *left = ordering(par);

    while (tok_matches(par, TOK_BANG_EQUAL) ||
           tok_matches(par, TOK_EQUAL_EQUAL))
    {
        struct tok *op = par->prev_tok;
        struct exp *right = ordering(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
ordering(struct par_state *par)
/*  ordering -> addition ( ( ">" | ">=" | "<" | "<=" ) addition)*          */
{
    struct exp *left = addition(par);

    while (tok_matches(par, TOK_GREAT) || tok_matches(par, TOK_GREAT_EQUAL) ||
           tok_matches(par, TOK_LESS) || tok_matches(par, TOK_LESS_EQUAL))
    {
        struct tok *op = par->prev_tok;
        struct exp *right = addition(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
addition(struct par_state *par)
/*  addition -> multiplication ( ( "-" | "+" ) multiplication)*              */
{
    struct exp *left = multiplication(par);

    while (tok_matches(par, TOK_MINUS) || tok_matches(par, TOK_PLUS))
    {
        struct tok *op = par->prev_tok;
        struct exp *right = multiplication(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
multiplication(struct par_state *par)
/*  multiplication -> unary ( ( "/" | "*" ) unary )*                         */
{
    struct exp *left = unary(par);

    while (tok_matches(par, TOK_SLASH) || tok_matches(par, TOK_STAR))
    {
        struct tok *op = par->prev_tok;
        struct exp *right = unary(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
unary(struct par_state *par)
/*  unary -> ( ("!" | "-") unary )
           | primary                                                         */
{
    if (tok_matches(par, TOK_BANG) || tok_matches(par, TOK_MINUS))
    {
        struct tok *op = par->prev_tok;
        struct exp *exp = unary(par);
        return exp_new_unary(op, exp);
    }
    else
        return primary(par);
}

static struct exp *
primary(struct par_state *par)
/* primary -> IDENTIFIER
            | INTEGER | DOUBLE | STRING | "false" | "true" |
            | "(" expression ")"
            | XXX error productions                                          */
{
    if (tok_matches(par, TOK_IDENTIFIER))
        return exp_new_var(par->prev_tok);
    if (tok_matches(par, TOK_INTEGER) || tok_matches(par, TOK_DOUBLE) ||
        tok_matches(par, TOK_STRING) ||
        tok_matches(par, TOK_FALSE) || tok_matches(par, TOK_TRUE))
    {
        return exp_new_literal(par->prev_tok);
    }
    else if (tok_matches(par, TOK_PAREN_LEFT))
    {
        struct exp *exp = expression(par);
        if (!tok_matches(par, TOK_PAREN_RIGHT))
        {
            err_at_tok(par->path, par->this_tok,
                "\n    Expected a closing paren, instead got `%s`.\n\n",
                par->this_tok->lexeme);
            return NULL; /* Unreachable. */
        }
        else
            return exp;
    }
    else
    {
        err_at_tok(par->path, par->this_tok,
            "\n    Expected number, paren or keyword. Instead got `%s`.\n\n",
            par->this_tok->lexeme);
        return NULL; /* Unreachable. */
    }
}

static struct tok *
tok_next(struct par_state *par)
{
    par->prev_tok = par->this_tok;
    par->this_tok = lex_get_tok(&par->lex);

    return par->prev_tok;
}

static bool
tok_matches(struct par_state *par, enum tok_type type)
{
    if (par->this_tok->type == type)
    {
        tok_next(par);
        return true;
    }
    else
        return false;
}

inline static bool
tok_is(struct par_state *par, enum tok_type type)
{
    return par->this_tok->type == type;
}

inline static bool
tok_was(struct par_state *par, enum tok_type type)
{
    return par->prev_tok->type == type;
}

static struct stm *
stm_new_block(Vector_T block)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_BLOCK;
    stm->data.block = block;

    return stm;
}

static struct stm *
stm_new_if(struct exp *cond, struct stm *then_block, struct stm *else_block)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_IF;
    stm->data.if_cond.cond = cond;
    stm->data.if_cond.then_block = then_block;
    stm->data.if_cond.else_block = else_block;

    return stm;
}

struct stm*
stm_new_while(struct exp *cond, struct stm *body)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_WHILE;
    stm->data.while_cond.cond = cond;
    stm->data.while_cond.body = body;

    return stm;
}

static struct stm *
stm_new_var_decl(struct tok *name, struct exp *exp)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_VAR_DECL;
    stm->data.var_decl.name = name;
    stm->data.var_decl.exp = exp;

    return stm;
}

static struct stm *
stm_new_expr_stmt(struct exp *exp, struct tok *last)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_EXPR_STMT;
    stm->data.expr.exp = exp;
    stm->data.expr.last = last;

    return stm;
}

static struct stm *
stm_new_print(struct exp *exp, struct tok *last)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_PRINT;
    stm->data.print.exp = exp;
    stm->data.print.last = last;

    return stm;
}

static struct exp *
exp_new_assign(struct tok *name, struct exp *value)
{
    struct exp *exp = malloc(sizeof(struct stm));

    exp->type = EXP_ASSIGN;
    exp->data.assign.name = name;
    exp->data.assign.exp = value;

    return exp;
}

static struct exp *
exp_new_binary(struct tok *op, struct exp *left, struct exp *right)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_BINARY;
    exp->data.binary.op = op;
    exp->data.binary.left = left;
    exp->data.binary.right = right;

    return exp;
}

static struct exp *
exp_new_unary(struct tok *op, struct exp *operand)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_UNARY;
    exp->data.unary.op = op;
    exp->data.unary.exp = operand;

    return exp;
}

static struct exp *
exp_new_group(struct exp *group, struct tok *lparen, struct tok *rparen)
/* XXX group codepaths are neither used nor tested */
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_GROUP;
    exp->data.group.exp = group;
    exp->data.group.lparen = lparen;
    exp->data.group.rparen = rparen;

    return exp;
}

static struct exp *
exp_new_var(struct tok *tok)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_VAR;
    exp->data.name = tok;

    return exp;
}

static struct exp *
exp_new_literal(struct tok *tok)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_LITERAL;
    exp->data.literal = tok;

    return exp;
}

void
print_stm(struct stm *stm)
{
    switch (stm->type)
    {
        case STM_BLOCK:
        {
            Vector_T vector;
            int i, len;

            puts("[");
            vector = stm->data.block;
            len = Vector_length(vector);
            for (i = 0; i < len; ++i)
                print_stm(*(struct stm **)Vector_get(vector, i));
            puts("]");
        } break;

        case STM_IF:
        {
            printf("[ if ");
            print_exp(stm->data.if_cond.cond);
            putchar('\n');
            printf("then ");
            print_stm(stm->data.if_cond.then_block);

            if (stm->data.if_cond.else_block != NULL)
            {
                printf("else ");
                print_stm(stm->data.if_cond.else_block);
            }
            puts("]");
        } break;

        case STM_WHILE:
        {
            printf("[ while ");
            print_exp(stm->data.while_cond.cond);
            putchar('\n');
            print_stm(stm->data.while_cond.body);
            puts("]");
        } break;

        case STM_VAR_DECL:
        {
            printf("[ %s = ", stm->data.var_decl.name->lexeme);
            print_exp(stm->data.var_decl.exp);
            puts("]");
        } break;

        case STM_EXPR_STMT:
        {
            printf("[ expstm ");
            print_exp(stm->data.expr.exp);
            puts("]");
        } break;

        case STM_PRINT:
        {
            printf("[ print ");
            print_exp(stm->data.expr.exp);
            puts("]");
        } break;
    }
}

void
print_exp(struct exp *exp)
{
    switch (exp->type)
    {
        case EXP_ASSIGN:
        {
            printf("( assign %s ", exp->data.assign.name->lexeme);
            print_exp(exp->data.assign.exp);
            printf(") ");
        } break;

        case EXP_BINARY:
        {
            printf("( %s ", exp->data.binary.op->lexeme);
            print_exp(exp->data.binary.left);
            print_exp(exp->data.binary.right);
            printf(") ");
        } break;

        case EXP_UNARY:
        {
            printf("( %s ", exp->data.unary.op->lexeme);
            print_exp(exp->data.unary.exp);
            printf(") ");
        } break;

        case EXP_GROUP:
        {
            printf("( ");
            print_exp(exp->data.group.exp);
            printf(") ");
        } break;

        case EXP_VAR:
            printf("%s ", exp->data.name->lexeme);
        break;

        case EXP_LITERAL:
        {
            struct tok *tok = exp->data.literal;
            if (tok->type == TOK_STRING)
                printf("\"%s\" ", exp->data.literal->lexeme);
            else
                printf("%s ", exp->data.literal->lexeme);
        } break;
    }
}

