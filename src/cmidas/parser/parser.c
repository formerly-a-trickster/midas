#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "vector.h"

#define T Parser_T

#define tok_is(PAR, TYPE)  ((PAR)->this_tok->type == (TYPE))

#define tok_was(PAR, TYPE) ((PAR)->prev_tok->type == (TYPE))

struct T
{
       Lexer_T  lex;
    const char *path;
    struct tok *prev_tok;
    struct tok *this_tok;
       jmp_buf  handle_err;
          bool  had_err;
          char  err_msg[256];
};

static const char *read_file  (T par, const char *path);
static   Vector_T  program    (T par);
static struct stm *declaration(T par);
static struct stm *statement  (T par);
static struct stm *block      (T par);
static struct stm *if_cond    (T par);
static struct stm *while_cond (T par);
static struct stm *var_decl   (T par);
static struct stm *exp_stm    (T par);
static struct stm *print      (T par);

static struct exp *expression    (T par);
static struct exp *assignment    (T par);
static struct exp *equality      (T par);
static struct exp *ordering      (T par);
static struct exp *addition      (T par);
static struct exp *multiplication(T par);
static struct exp *unary         (T par);
static struct exp *primary       (T par);

static struct tok *tok_next   (T par);
static        bool tok_matches(T par, enum tok_type);
static        void tok_consume(T par, enum tok_type, const char *);

static struct stm *stm_new_block    (Vector_T);
static struct stm *stm_new_if       (struct exp *, struct stm *, struct stm *);
static struct stm *stm_new_while    (struct exp *, struct stm *);
static struct stm *stm_new_var_decl (struct tok *, struct exp *);
static struct stm *stm_new_print    (struct exp *, struct tok *);
static struct stm *stm_new_exp_stm  (struct exp *, struct tok *);

static struct exp *exp_new_assign (struct tok *, struct exp *);
static struct exp *exp_new_binary (struct tok *, struct exp *, struct exp *);
static struct exp *exp_new_unary  (struct tok *, struct exp *);
static struct exp *exp_new_group  (struct exp *, struct tok *, struct tok *);
static struct exp *exp_new_var    (struct tok *);
static struct exp *exp_new_literal(struct tok *);

T
Par_new(void)
{
    T par;

    par = malloc(sizeof(struct T));

    par->lex = Lex_new();
    par->path = NULL;
    par->prev_tok = NULL;
    par->this_tok = NULL;
    par->had_err = false;

    return par;
}

Vector_T
Par_parse(T par, const char *path)
{
    const char *buffer;

    par->path = path;

    buffer = read_file(par, path);
    if (buffer == NULL)
    {
        printf("%s\n", par->err_msg);
        return NULL;
    }
    Lex_feed(par->lex, buffer);

    tok_next(par);

    return program(par);
}

static const char *
read_file(T par, const char *path)
{
    FILE *source;
    int file_size, bytes_read;
    char* buffer;

    source = fopen(path, "rb");
    if (source == NULL)
    {
        sprintf(par->err_msg, "File: %s\n"
                "Failed to read file. It could not be opened.",
                path);
        par->had_err = true;
        return NULL;
    }

    /* XXX the standard does not guarantee that this will work */
    fseek(source, 0L, SEEK_END);
    file_size = ftell(source);
    rewind(source);

    buffer = malloc(file_size + 1);
    if (buffer == NULL)
    {
        sprintf(par->err_msg, "File: %s\n"
                "Failed to read file. Not enough memory.",
                path);
        par->had_err = true;
        return NULL;
    }

    bytes_read = fread(buffer, sizeof(char), file_size, source);
    if (bytes_read < file_size)
    {
        sprintf(par->err_msg, "File: %s\n"
                "Failed to read file. Reading stopped midway.",
                path);
        par->had_err = true;
        return NULL;
    }


    return buffer;
}

static Vector_T
program(T par)
/* program -> statement* "EOF" */
{
    Vector_T program;

    program = Vector_new(sizeof(struct stm *));
    /*
     * XXX if we would have used !tok_matches(par, TOK_EOF), we would force the
     * lexer to read beyond the EOF upon finally matching it
     */
    switch(setjmp(par->handle_err))
    {
        case 0:
            while(!tok_is(par, TOK_EOF))
            {
                struct stm *stm = declaration(par);
                Vector_push(program, &stm);
            }
            break;

        case 1:
            printf("Parsing failed.\n%s\n", par->err_msg);

        case 2:
            exit(1);
            break;
    }

    return program;
}

static struct stm *
declaration(T par)
/*
 * declaration -> var_decl ";"
 *              | statement
 */
{
    struct stm *stm;

    if (tok_matches(par, TOK_VAR))
        stm = var_decl(par);
    else
        stm = statement(par);

    return stm;
}

static struct stm *
var_decl(T par)
/* var_decl -> ^var^ identifier "=" expression ";" */
{
    struct tok *name;
    struct exp *value;

    tok_consume(par, TOK_IDENTIFIER,
        "A variable name should follow the `var` keyword.");

    name = par->prev_tok;

    tok_consume(par, TOK_EQUAL,
        "An equal sign should follow the variable's name.");

    value = expression(par);

    tok_consume(par, TOK_SEMICOLON,
        "Missing semicolon after variable declaration.");

    return stm_new_var_decl(name, value);
}

static struct stm *
statement(T par)
/*
 * statement -> block_stm
 *            | if_stm
 *            | while_stm
 *            | print_stm
 *            | exp_stm
 */
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
        stm = exp_stm(par);

    return stm;
}

static struct stm *
block(T par)
/* block_stm -> ^do^ declaration* "end" */
{
    Vector_T statements;

    statements = Vector_new(sizeof(struct stm *));
    while (!tok_is(par, TOK_END) && !tok_is(par, TOK_EOF))
    {
        struct stm *stm;

        stm = declaration(par);
        Vector_push(statements, &stm);
    }

    tok_consume(par, TOK_END, "Missing `end` keyword after block.");

    return stm_new_block(statements);
}

static struct stm *
if_cond(T par)
/* if_stm -> ^if^ "(" expression ")" statement ( "else" statement )? */
{
    struct stm *then_block, *else_block;
    struct exp *cond;

    tok_consume(par, TOK_PAREN_LEFT,
        "Expected an opening paren after `if` keyword.");

    cond = expression(par);

    tok_consume(par, TOK_PAREN_RIGHT,
        "Expected a closing paren after the if condition.");

    then_block = statement(par);

    if (tok_matches(par, TOK_ELSE))
        else_block = statement(par);
    else
        else_block = NULL;

    return stm_new_if(cond, then_block, else_block);
}

static struct stm *
while_cond(T par)
/* while_stm -> ^while^ "(" expression ")" statement */
{
    struct exp *cond;
    struct stm *body;

    tok_consume(par, TOK_PAREN_LEFT,
        "Expected an opening paren after `while` keyword.");

    cond = expression(par);

    tok_consume(par, TOK_PAREN_RIGHT,
        "Expected a closing paren after the while condition.");

    body = statement(par);

    return stm_new_while(cond, body);
}

static struct stm *
print(T par)
/* print_stm -> ^print^ expression ";" */
{
    struct exp *exp;

    exp = expression(par);

    tok_consume(par, TOK_SEMICOLON,
        "Missing semicolon after print statement.");

    return stm_new_print(exp, par->prev_tok);
}

static struct stm *
exp_stm(T par)
/* exp_stm -> expression ";" */
{
    struct exp *exp;

    exp = expression(par);

    tok_consume(par, TOK_SEMICOLON,
        "Missing semicolon after expression statement.");

    return stm_new_exp_stm(exp, par->prev_tok);
}

static struct exp *
expression(T par)
/*  expression -> assignment */
{
    return assignment(par);
}

static struct exp *
assignment(T par)
/*
 * XXX having assignment be an expression simplifies the grammar, but puts us
 * in the situation of having an expression with a side effect. It would be
 * suited better to being a statement.
 *
 * assignment -> equality ( "=" assignment )*
 */
{
    struct exp *left;

    left = equality(par);
    if (tok_matches(par, TOK_EQUAL))
    {
        struct exp *exp;

        exp = assignment(par);
        if (left->type == EXP_VAR)
        {
            struct tok *varname;

            varname = left->data.name;
            left = exp_new_assign(varname, exp);
        }
        else
        {
            sprintf(par->err_msg,
                    "Invalid assignment target. Expected a variable name.");
            par->had_err = true;
            longjmp(par->handle_err, 1);
        }
    }

    return left;
}

static struct exp *
equality(T par)
/* equality -> ordering ( ( "!=" | "==" ) ordering )* */
{
    struct exp *left;

    left = ordering(par);
    while (tok_matches(par, TOK_BANG_EQUAL) ||
           tok_matches(par, TOK_EQUAL_EQUAL))
    {
        struct tok *op;
        struct exp *right;

        op = par->prev_tok;
        right = ordering(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
ordering(T par)
/* ordering -> addition ( ( ">" | ">=" | "<" | "<=" ) addition)* */
{
    struct exp *left;

    left = addition(par);
    while (tok_matches(par, TOK_GREAT) || tok_matches(par, TOK_GREAT_EQUAL) ||
           tok_matches(par, TOK_LESS) || tok_matches(par, TOK_LESS_EQUAL))
    {
        struct tok *op;
        struct exp *right;

        op = par->prev_tok;
        right = addition(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
addition(T par)
/* addition -> multiplication ( ( "-" | "+" ) multiplication)* */
{
    struct exp *left;

    left = multiplication(par);
    while (tok_matches(par, TOK_MINUS) || tok_matches(par, TOK_PLUS))
    {
        struct tok *op;
        struct exp *right;

        op = par->prev_tok;
        right = multiplication(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
multiplication(T par)
/* multiplication -> unary ( ( "/" | "//" | "*" | "%" ) unary )* */
{
    struct exp *left;

    left = unary(par);
    while
    (
        tok_matches(par, TOK_SLASH) ||
        tok_matches(par, TOK_SLASH_SLASH) ||
        tok_matches(par, TOK_STAR) ||
        tok_matches(par, TOK_PERCENT)
    )
    {
        struct tok *op;
        struct exp *right;

        op = par->prev_tok;
        right = unary(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
unary(T par)
/*
 * unary -> ( ("!" | "-") unary )
 *        | primary
 */
{
    if (tok_matches(par, TOK_BANG) || tok_matches(par, TOK_MINUS))
    {
        struct tok *op;
        struct exp *exp;

        op = par->prev_tok;
        exp = unary(par);
        return exp_new_unary(op, exp);
    }
    else
        return primary(par);
}

static struct exp *
primary(T par)
/*
 * primary -> IDENTIFIER
 *          | INTEGER | DOUBLE | STRING | "false" | "true" |
 *          | "(" expression ")"
 *          | XXX error productions
 */
{
    struct exp *exp;

    if (tok_matches(par, TOK_IDENTIFIER))
        exp = exp_new_var(par->prev_tok);
    else if
    (
        tok_matches(par, TOK_INTEGER) ||
        tok_matches(par, TOK_DOUBLE) ||
        tok_matches(par, TOK_STRING) ||
        tok_matches(par, TOK_FALSE) ||
        tok_matches(par, TOK_TRUE)
    )
        exp = exp_new_literal(par->prev_tok);
    else if (tok_matches(par, TOK_PAREN_LEFT))
    {
        exp = expression(par);

        tok_consume(par, TOK_PAREN_RIGHT, "Expected a closing paren.");
    }
    else
    {
        sprintf(par->err_msg,
                "Expected number, paren or keyword. Instead got `%s`.",
                par->this_tok->lexeme);
        par->had_err = true;
        longjmp(par->handle_err, 1);
    }

    return exp;
}

static struct tok *
tok_next(T par)
{
    struct tok *tok;

    tok = Lex_get_tok(par->lex);
    if (tok == NULL)
    {
        printf("File: %s\n", par->path);
        Lex_get_err(par->lex);
        par->had_err = true;
        longjmp(par->handle_err, 2);
    }

    par->prev_tok = par->this_tok;
    par->this_tok = tok;

    return par->prev_tok;
}

static bool
tok_matches(T par, enum tok_type type)
{
    if (par->this_tok->type == type)
    {
        tok_next(par);
        return true;
    }
    else
        return false;
}

static void
tok_consume(T par, enum tok_type type, const char *message)
{
    if (par->this_tok->type == type)
        tok_next(par);
    else
    {
        strcpy(par->err_msg, message);
        par->had_err = true;
        longjmp(par->handle_err, 1);
    }
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
stm_new_exp_stm(struct exp *exp, struct tok *last)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_EXP_STM;
    stm->data.exp.exp = exp;
    stm->data.exp.last = last;

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

        case STM_EXP_STM:
        {
            printf("[ expstm ");
            print_exp(stm->data.exp.exp);
            puts("]");
        } break;

        case STM_PRINT:
        {
            printf("[ print ");
            print_exp(stm->data.exp.exp);
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

#undef T
#undef tok_is
#undef tok_was

