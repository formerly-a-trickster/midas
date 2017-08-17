#include "error.h"
#include "utils.h"
#include "lexer.h"
#include "parser.h"

#include <stdbool.h>
#include <stdlib.h>

static struct stm* program(struct par_state*);
static struct stm* statement(struct par_state*);
static struct stm* var_decl(struct par_state*);
static struct stm* expr_stmt(struct par_state*);
static struct stm* print(struct par_state*);

static struct exp* expression(struct par_state*);
static struct exp* assignment(struct par_state*);
static struct exp* equality(struct par_state*);
static struct exp* ordering(struct par_state*);
static struct exp* addition(struct par_state*);
static struct exp* multiplication(struct par_state*);
static struct exp* unary(struct par_state*);
static struct exp* primary(struct par_state*);

static struct tok* tok_next(struct par_state*);
static bool tok_matches(struct par_state*, enum tok_type);

static struct stm* stm_new_block(struct stmlist*);
static struct stm* stm_new_var_decl(struct tok*, struct exp*);
static struct stm* stm_new_print(struct exp*, struct tok*);
static struct stm* stm_new_expr_stmt(struct exp*, struct tok*);

static struct exp* exp_new_assign(struct tok*, struct exp*);
static struct exp* exp_new_binary(struct tok*, struct exp*, struct exp*);
static struct exp* exp_new_unary(struct tok*, struct exp*);
static struct exp* exp_new_group(struct exp*, struct tok*, struct tok*);
static struct exp* exp_new_var(struct tok*);
static struct exp* exp_new_literal(struct tok*);

struct stm*
parse(struct par_state* par, const char* path)
{
    lex_init(&par->lex);
    par->path = path;
    par->prev_tok = NULL;
    par->this_tok = NULL;

    lex_feed(&par->lex, path);
    tok_next(par);

    return program(par);
}

static struct stm*
program(struct par_state* par)
/*  program -> statement* "EOF"                                              */
{
    struct stmlist* program = stmlist_new();
    /* XXX if we would have used !tok_matches(par, TOK_EOF), we would force the
       lexer to read beyond the EOF upon finally matching it */
    while(par->this_tok->type != TOK_EOF)
    {
        struct stm* stm = statement(par);
        stmlist_append(program, stm);
    }

    return stm_new_block(program);
}

static struct stm*
statement(struct par_state* par)
/*  statement -> var_decl   ";"
               | print_stm  ";"
               | expr_stmt  ";"                                              */
{
    struct stm* stm;

    if (tok_matches(par, TOK_VAR))
        stm = var_decl(par);
    else if (tok_matches(par, TOK_PRINT))
        stm = print(par);
    else
        stm = expr_stmt(par);

    if (!tok_matches(par, TOK_SEMICOLON))
        err_at_tok(par->path, par->prev_tok,
            "\n    Missing semicolon after statement.\n\n");
    else
        return stm;
}

static struct stm*
var_decl(struct par_state* par)
/*  var_decl -> ^var^ identifier "=" expression                              */
{
    if (tok_matches(par, TOK_IDENTIFIER))
    {
        struct tok* name = par->prev_tok;
        if (tok_matches(par, TOK_EQUAL))
        {
            struct exp* value = expression(par);
            return stm_new_var_decl(name, value);
        }
        else
            err_at_tok(par->path, par->prev_tok,
                "\n    An equal sign should follow the variable's name.\n\n");
    }
    else
        err_at_tok(par->path, par->prev_tok,
            "\n    A variable name should follow the `var` keyword.\n\n");
}

static struct stm*
print(struct par_state* par)
/*  print_stm -> ^print^ expression                                          */
{
    struct exp* exp = expression(par);

    return stm_new_print(exp, par->prev_tok);
}

static struct stm*
expr_stmt(struct par_state* par)
/*  expr_stmt -> expression                                                  */
{
    struct exp* exp = expression(par);

    return stm_new_expr_stmt(exp, par->prev_tok);
}

static struct exp*
expression(struct par_state* par)
/*  expression -> assignment                                                 */
{
    return assignment(par);
}

static struct exp*
assignment(struct par_state* par)
/* XXX having assignment be an expression simplifies the grammar, but puts us
   in the situation of having an expression with a side effect. It would be
   suited better to being a statement                                        */
/*  assignment -> equality ( "=" assignment )*                               */
{
    struct exp* left = equality(par);

    if (tok_matches(par, TOK_EQUAL))
    {
        struct tok* eq = par->prev_tok;
        struct exp* exp = assignment(par);

        if (left->type == EXP_VAR)
        {
            struct tok* varname = left->data.name;
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

static struct exp*
equality(struct par_state* par)
/*  equality -> ordering ( ( "!=" | "==" ) ordering )*                       */
{
    struct exp* left = ordering(par);

    while (tok_matches(par, TOK_BANG_EQUAL) ||
           tok_matches(par, TOK_EQUAL_EQUAL))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = ordering(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
ordering(struct par_state* par)
/*  ordering -> addition ( ( ">" | ">=" | "<" | "<=" ) addition)*          */
{
    struct exp* left = addition(par);

    while (tok_matches(par, TOK_GREAT) || tok_matches(par, TOK_GREAT_EQUAL) ||
           tok_matches(par, TOK_LESS) || tok_matches(par, TOK_LESS_EQUAL))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = addition(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
addition(struct par_state* par)
/*  addition -> multiplication ( ( "-" | "+" ) multiplication)*              */
{
    struct exp* left = multiplication(par);

    while (tok_matches(par, TOK_MINUS) || tok_matches(par, TOK_PLUS))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = multiplication(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
multiplication(struct par_state* par)
/*  multiplication -> unary ( ( "/" | "*" ) unary )*                         */
{
    struct exp* left = unary(par);

    while (tok_matches(par, TOK_SLASH) || tok_matches(par, TOK_STAR))
    {
        struct tok* op = par->prev_tok;
        struct exp* right = unary(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp*
unary(struct par_state* par)
/*  unary -> ( ("!" | "-") unary )
           | primary                                                         */
{
    if (tok_matches(par, TOK_BANG) || tok_matches(par, TOK_MINUS))
    {
        struct tok* op = par->prev_tok;
        struct exp* exp = unary(par);
        return exp_new_unary(op, exp);
    }
    else
        return primary(par);
}

static struct exp*
primary(struct par_state* par)
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
        struct exp* exp = expression(par);
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

static struct tok*
tok_next(struct par_state* par)
{
    par->prev_tok = par->this_tok;
    par->this_tok = lex_get_tok(&par->lex);

    return par->prev_tok;
}

static bool
tok_matches(struct par_state* par, enum tok_type type)
{
    if (par->this_tok->type == type)
    {
        tok_next(par);
        return true;
    }
    else
        return false;
}

static struct stm*
stm_new_block(struct stmlist* block)
{
    struct stm* stm = malloc(sizeof(struct stm));

    stm->type = STM_BLOCK;
    stm->data.block = block;

    return stm;
}

static struct stm*
stm_new_var_decl(struct tok* name, struct exp* exp)
{
    struct stm* stm = malloc(sizeof(struct stm));

    stm->type = STM_VAR_DECL;
    stm->data.var_decl.name = name;
    stm->data.var_decl.exp = exp;

    return stm;
}

static struct stm*
stm_new_expr_stmt(struct exp* exp, struct tok* last)
{
    struct stm* stm = malloc(sizeof(struct stm));

    stm->type = STM_EXPR_STMT;
    stm->data.expr.exp = exp;
    stm->data.expr.last = last;

    return stm;
}

static struct stm*
stm_new_print(struct exp* exp, struct tok* last)
{
    struct stm* stm = malloc(sizeof(struct stm));

    stm->type = STM_PRINT;
    stm->data.print.exp = exp;
    stm->data.print.last = last;

    return stm;
}

static struct exp*
exp_new_assign(struct tok* name, struct exp* value)
{
    struct exp* exp = malloc(sizeof(struct stm));

    exp->type = EXP_ASSIGN;
    exp->data.assign.name = name;
    exp->data.assign.exp = value;

    return exp;
}

static struct exp*
exp_new_binary(struct tok* op, struct exp* left, struct exp* right)
{
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_BINARY;
    exp->data.binary.op = op;
    exp->data.binary.left = left;
    exp->data.binary.right = right;

    return exp;
}

static struct exp*
exp_new_unary(struct tok* op, struct exp* operand)
{
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_UNARY;
    exp->data.unary.op = op;
    exp->data.unary.exp = operand;

    return exp;
}

static struct exp*
exp_new_group(struct exp* group, struct tok* lparen, struct tok* rparen)
/* XXX group codepaths are neither used nor tested */
{
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_GROUP;
    exp->data.group.exp = group;
    exp->data.group.lparen = lparen;
    exp->data.group.rparen = rparen;

    return exp;
}

static struct exp*
exp_new_var(struct tok* tok)
{
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_VAR;
    exp->data.name = tok;

    return exp;
}

static struct exp*
exp_new_literal(struct tok* tok)
{
    struct exp* exp = malloc(sizeof(struct exp));

    exp->type = EXP_LITERAL;
    exp->data.literal = tok;

    return exp;
}

void
print_stm(struct stm* stm)
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
                print_stm(node->data);
            }
            break;

        case STM_VAR_DECL:
            printf("[ %s = ", stm->data.var_decl.name->lexeme);
            print_exp(stm->data.var_decl.exp);
            printf("]\n");
            break;

        case STM_EXPR_STMT:
            printf("[ expstm ");
            print_exp(stm->data.expr.exp);
            printf("]\n");
            break;

        case STM_PRINT:
            printf("[ print ");
            print_exp(stm->data.expr.exp);
            printf("]\n");
            break;
    }
}

void
print_exp(struct exp* exp)
{
    switch (exp->type)
    {
        case EXP_ASSIGN:
            printf("( assign %s ", exp->data.assign.name->lexeme);
            print_exp(exp->data.assign.exp);
            printf(") ");
            break;

        case EXP_BINARY:
            printf("( %s ", exp->data.binary.op->lexeme);
            print_exp(exp->data.binary.left);
            print_exp(exp->data.binary.right);
            printf(") ");
            break;

        case EXP_UNARY:
            printf("( %s ", exp->data.unary.op->lexeme);
            print_exp(exp->data.unary.exp);
            printf(") ");
            break;

        case EXP_GROUP:
            printf("( ");
            print_exp(exp->data.group.exp);
            printf(") ");
            break;

        case EXP_VAR:
            printf("%s ", exp->data.name->lexeme);
            break;

        case EXP_LITERAL:
            ;
            struct tok* tok = exp->data.literal;
            if (tok->type == TOK_STRING)
                printf("\"%s\" ", exp->data.literal->lexeme);
            else
                printf("%s ", exp->data.literal->lexeme);
            break;
    }
}

