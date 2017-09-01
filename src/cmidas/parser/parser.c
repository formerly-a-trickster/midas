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
  unsigned int  loop_depth;
  unsigned int  fun_depth;
};

static const char *read_file  (T par, const char *path);
static   Vector_T  program    (T par);
static struct stm *declaration(T par);
static struct stm *var_decl   (T par);
static struct stm *fun_decl   (T par);
static struct stm *statement  (T par);
static struct stm *block      (T par);
static struct stm *if_cond    (T par);
static struct stm *while_cond (T par);
static struct stm *for_cond   (T par);
static struct stm *break_stm  (T par);
static struct stm *return_stm (T par);
static struct stm *print      (T par);
static struct stm *exp_stm    (T par);

static struct exp *expression    (T par);
static struct exp *assignment    (T par);
static struct exp *logic_or      (T par);
static struct exp *logic_and     (T par);
static struct exp *equality      (T par);
static struct exp *ordering      (T par);
static struct exp *addition      (T par);
static struct exp *multiplication(T par);
static struct exp *unary         (T par);
static struct exp *call          (T par);
static struct exp *primary       (T par);

static struct tok *tok_next   (T par);
static       bool  tok_matches(T par, enum tok_t);
static       void  tok_consume(T par, enum tok_t, const char *);

static struct stm *stm_new_var_decl (const char *, struct exp *);
static struct stm *stm_new_fun_decl (const char *, Vector_T, struct stm *);
static struct stm *stm_new_block    (Vector_T);
static struct stm *stm_new_if       (struct exp *, struct stm *, struct stm *);
static struct stm *stm_new_while    (struct exp *, struct stm *);
static struct stm *stm_new_break    (void);
static struct stm *stm_new_return   (struct exp *);
static struct stm *stm_new_print    (struct exp *);
static struct stm *stm_new_exp_stm  (struct exp *);

static struct exp *exp_new_assign (const char *, struct exp *);
static struct exp *exp_new_binary (enum tok_t, struct exp *, struct exp *);
static struct exp *exp_new_unary  (enum tok_t, struct exp *);
static struct exp *exp_new_call   (struct exp *, Vector_T);
static struct exp *exp_new_ident  (const char *);
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
    par->loop_depth = 0;

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
/*
 * program -> statement* "EOF"
 */
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
            } break;

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
 *              | fun_decl ";"
 *              | statement
 */
{
    struct stm *stm;

    if (tok_matches(par, TOK_VAR))
        stm = var_decl(par);
    else if (tok_matches(par, TOK_FUN))
        stm = fun_decl(par);
    else
        stm = statement(par);

    return stm;
}

static struct stm *
var_decl(T par)
/*
 * var_decl -> ^var^ identifier "=" expression ";"
 */
{
    const char *name;
    struct exp *value;

    tok_consume(par, TOK_IDENTIFIER,
        "A variable name should follow the `var` keyword.");

    name = par->prev_tok->lexeme;

    tok_consume(par, TOK_EQUAL,
        "An equal sign should follow the variable's name.");

    value = expression(par);

    tok_consume(par, TOK_SEMICOLON,
        "Missing semicolon after variable declaration.");

    return stm_new_var_decl(name, value);
}

static struct stm *
fun_decl(T par)
/*
 * fun_decl -> ^fun^ identifier "(" formals? ")" block_stm
 *
 * formals -> identifier ( "," identifier )*
 */
{
    const char *name;
      Vector_T  formals;
    struct stm *body;

    tok_consume(par, TOK_IDENTIFIER,
            "A function name should follow the `fun` keyword.");

    name = par->prev_tok->lexeme;

    tok_consume(par, TOK_PAREN_LEFT,
            "Expected an opening paren after the function name");

    formals = Vector_new(sizeof(const char *));
    if (!tok_matches(par, TOK_PAREN_RIGHT))
    {
        do
        {
            tok_consume(par, TOK_IDENTIFIER,
                        "Function parameters should be a comma-separated list"
                        "of identifiers.");
            Vector_push(formals, &par->prev_tok->lexeme);
        } while (tok_matches(par, TOK_COMMA));

        tok_consume(par, TOK_PAREN_RIGHT,
                "Expected a closing paren after function arguments");
    }

    tok_consume(par, TOK_DO,
            "Expected a do...end block after function parameter list.");

    ++par->fun_depth;
    body = block(par);
    --par->fun_depth;

    return stm_new_fun_decl(name, formals, body);
}

static struct stm *
statement(T par)
/*
 * statement -> block_stm
 *            | if_stm
 *            | while_stm
 *            | for_stm
 *            | break_stm
 *            | return_stm
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
    else if (tok_matches(par, TOK_FOR))
        stm = for_cond(par);
    else if (tok_matches(par, TOK_BREAK))
        stm = break_stm(par);
    else if (tok_matches(par, TOK_RETURN))
        stm = return_stm(par);
    else if (tok_matches(par, TOK_PRINT))
        stm = print(par);
    else
        stm = exp_stm(par);

    return stm;
}

static struct stm *
block(T par)
/*
 * block_stm -> ^do^ declaration* "end"
 */
{
    Vector_T stmts;

    stmts = Vector_new(sizeof(struct stm *));
    while (!tok_is(par, TOK_END) && !tok_is(par, TOK_EOF))
    {
        struct stm *stm;

        stm = declaration(par);
        Vector_push(stmts, &stm);
    }

    tok_consume(par, TOK_END, "Missing `end` keyword after block.");

    return stm_new_block(stmts);
}

static struct stm *
if_cond(T par)
/*
 * if_stm -> ^if^ "(" expression ")" statement ( "else" statement )?
 */
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
/*
 * while_stm -> ^while^ "(" expression ")" statement
 */
{
    struct exp *cond;
    struct stm *body;

    tok_consume(par, TOK_PAREN_LEFT,
        "Expected an opening paren after `while` keyword.");

    cond = expression(par);

    tok_consume(par, TOK_PAREN_RIGHT,
        "Expected a closing paren after the while condition.");

    ++par->loop_depth;
    body = statement(par);
    --par->loop_depth;

    return stm_new_while(cond, body);
}

static struct stm*
for_cond(T par)
/*
 * for_stm -> ^for^ "("
 *                      ( var_decl | expr_stm | ";" )
 *                      expression? ";"
 *                      assignment?
 *                  ")" statement
 *
 *  NB: var_decl and expr_stm already contain a semicolon
 */
{
    struct stm *init, *body, *loop;
    struct exp *cond, *incr;

    tok_consume(par, TOK_PAREN_LEFT,
            "Expected an opening paren after the `for` keyword.");

    if (tok_matches(par, TOK_VAR))
        init = var_decl(par);
    else if (tok_matches(par, TOK_SEMICOLON))
        init = NULL;
    else
        init = exp_stm(par);

    if (!tok_matches(par, TOK_SEMICOLON))
    {
        cond = expression(par);
        tok_consume(par, TOK_SEMICOLON,
                "Expected a semicolon after the for condition.");
    }
    else
        cond = NULL;

    if (!tok_matches(par, TOK_PAREN_RIGHT))
    {
        incr = assignment(par);
        tok_consume(par, TOK_PAREN_RIGHT,
                "Expected a closing paren after the for construct.");
    }
    else
        incr = NULL;

    ++par->loop_depth;
    body = statement(par);
    --par->loop_depth;

    if (incr != NULL)
    {
        struct stm *incr_stm;

        incr_stm = stm_new_exp_stm(incr);
        if (body->type == STM_BLOCK)
            Vector_push(body->data.block, &incr_stm);
        else
        {
            Vector_T stmts;

            stmts = Vector_new(sizeof(struct stm *));
            Vector_push(stmts, &body);
            Vector_push(stmts, &incr_stm);
            body = stm_new_block(stmts);
        }
    }

    if (cond == NULL)
    {
        /* XXX we should be able to just slap together a token */
        struct tok *true_tok;

        true_tok = malloc(sizeof(struct tok));
        true_tok->lexeme = "true";
        true_tok->length = 5;
        true_tok->type   = TOK_TRUE;
        true_tok->lineno = 0;
        true_tok->colno  = 0;

        cond = exp_new_literal(true_tok);
    }

    loop = stm_new_while(cond, body);

    if (init != NULL)
    {
        Vector_T stmts;

        stmts = Vector_new(sizeof(struct stm *));
        Vector_push(stmts, &init);
        Vector_push(stmts, &loop);
        loop = stm_new_block(stmts);
    }

    return loop;
}

static struct stm *
break_stm(T par)
/*
 * break_stm -> ^break^ ";"
 */
{
    if (par->loop_depth)
        tok_consume(par, TOK_SEMICOLON,
                "Missing semicolone after break statement.");
    else
    {
        sprintf(par->err_msg,
                "Encoutered break statement outside of a for or while loop");
        par->had_err = true;
        longjmp(par->handle_err, 1);
    }

    return stm_new_break();
}

static struct stm *
return_stm(T par)
/*
 * return_stm -> ^return^ primary? ";"
 */
{
    if (par->fun_depth)
    {
        if (!tok_matches(par, TOK_SEMICOLON))
        {
            struct exp *ret_exp;

            ret_exp = primary(par);

            tok_consume(par, TOK_SEMICOLON,
                    "Missing semicolon after return statement");

            return stm_new_return(ret_exp);
        }
        else
        {
            struct tok *nil;

            /* XXX using a literal token is less than ideal */
            nil = malloc(sizeof(struct tok));
            nil->lexeme = "nil";
            nil->type = TOK_NIL;
            nil->length = 4;
            nil->lineno = 0;
            nil->colno = 0;

            return stm_new_return(exp_new_literal(nil));
        }
    }
    else
    {
        sprintf(par->err_msg,
                "Encountered return statement outside of a function "
                "declaration.");
        par->had_err = true;
        longjmp(par->handle_err, 1);
    }
}

static struct stm *
print(T par)
/*
 * print_stm -> ^print^ expression ";"
 */
{
    struct exp *exp;

    exp = expression(par);

    tok_consume(par, TOK_SEMICOLON,
        "Missing semicolon after print statement.");

    return stm_new_print(exp);
}

static struct stm *
exp_stm(T par)
/*
 * exp_stm -> expression ";"
 */
{
    struct exp *exp;

    exp = expression(par);

    tok_consume(par, TOK_SEMICOLON,
        "Missing semicolon after expression statement.");

    return stm_new_exp_stm(exp);
}

static struct exp *
expression(T par)
/*
 * expression -> assignment
 */
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
 * assignment -> logic_or ( "=" assignment )
 */
{
    struct exp *left;

    left = logic_or(par);
    if (tok_matches(par, TOK_EQUAL))
    {
        struct exp *right;

        right = assignment(par);

        if (left->type == EXP_IDENT)
            return exp_new_assign(left->data.name, right);
        else
        {
            sprintf(par->err_msg, "Cannot assign to this target.");
            par->had_err = true;
            longjmp(par->handle_err, 1);
        }
    }

    return left;
}

static struct exp *
logic_or(T par)
/*
 * or -> and ( "or" and )*
 */
{
    struct exp *left;

    left = logic_and(par);
    while (tok_matches(par, TOK_OR))
    {
        enum tok_t  op;
        struct exp *right;

        op = par->prev_tok->type;
        right = logic_and(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
logic_and(T par)
/*
 * and -> equality ( "or" equality )*
 */
{
    struct exp *left;

    left = equality(par);
    while (tok_matches(par, TOK_AND))
    {
        enum tok_t  op;
        struct exp *right;

        op = par->prev_tok->type;
        right = equality(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
equality(T par)
/*
 * equality -> ordering ( ( "!=" | "==" ) ordering )*
 */
{
    struct exp *left;

    left = ordering(par);
    while (tok_matches(par, TOK_BANG_EQUAL) ||
           tok_matches(par, TOK_EQUAL_EQUAL))
    {
        enum tok_t  op;
        struct exp *right;

        op = par->prev_tok->type;
        right = ordering(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
ordering(T par)
/*
 * ordering -> addition ( ( ">" | ">=" | "<" | "<=" ) addition)*
 */
{
    struct exp *left;

    left = addition(par);
    while (tok_matches(par, TOK_GREAT) || tok_matches(par, TOK_GREAT_EQUAL) ||
           tok_matches(par, TOK_LESS) || tok_matches(par, TOK_LESS_EQUAL))
    {
        enum tok_t  op;
        struct exp *right;

        op = par->prev_tok->type;
        right = addition(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
addition(T par)
/*
 * addition -> multiplication ( ( "-" | "+" ) multiplication)*
 */
{
    struct exp *left;

    left = multiplication(par);
    while (tok_matches(par, TOK_MINUS) || tok_matches(par, TOK_PLUS))
    {
        enum tok_t  op;
        struct exp *right;

        op = par->prev_tok->type;
        right = multiplication(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
multiplication(T par)
/*
 * multiplication -> unary ( ( "/" | "//" | "*" | "%" ) unary )*
 */
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
        enum tok_t  op;
        struct exp *right;

        op = par->prev_tok->type;
        right = unary(par);
        left = exp_new_binary(op, left, right);
    }

    return left;
}

static struct exp *
unary(T par)
/*
 * unary -> ( ("!" | "-") unary )
 *        | call
 */
{
    if (tok_matches(par, TOK_BANG) || tok_matches(par, TOK_MINUS))
    {
        enum tok_t  op;
        struct exp *exp;

        op = par->prev_tok->type;
        exp = unary(par);
        return exp_new_unary(op, exp);
    }
    else
        return call(par);
}

static struct exp *
/*
 * call -> primary ( "(" parameters? ")" )*
 *
 * parameters -> expression ( "," expression )*
 */
call(T par)
{
    struct exp *callee;

    callee = primary(par);
    while (tok_matches(par, TOK_PAREN_LEFT))
    {
        Vector_T params;

        params = Vector_new(sizeof(struct exp *));
        if (!tok_matches(par, TOK_PAREN_RIGHT))
        {
            do
            {
                struct exp *exp;

                exp = expression(par);
                Vector_push(params, &exp);
            } while (tok_matches(par, TOK_COMMA));
        }

        tok_consume(par, TOK_PAREN_RIGHT,
                "Expected a closing paren after arguments.");

        callee = exp_new_call(callee, params);
    }

    return callee;
}

static struct exp *
primary(T par)
/*
 * primary -> IDENTIFIER
 *          | INTEGER | DOUBLE | STRING | NIL | "false" | "true" |
 *          | "(" expression ")"
 *          | XXX error productions
 */
{
    struct exp *exp;

    if (tok_matches(par, TOK_IDENTIFIER))
        exp = exp_new_ident(par->prev_tok->lexeme);
    else if
    (
        tok_matches(par, TOK_INTEGER) ||
        tok_matches(par, TOK_DOUBLE ) ||
        tok_matches(par, TOK_STRING ) ||
        tok_matches(par, TOK_NIL    ) ||
        tok_matches(par, TOK_FALSE  ) ||
        tok_matches(par, TOK_TRUE   )
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
    printf("(%s)\n", tok->lexeme);

    par->prev_tok = par->this_tok;
    par->this_tok = tok;

    return par->prev_tok;
}

static bool
tok_matches(T par, enum tok_t type)
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
tok_consume(T par, enum tok_t type, const char *message)
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
stm_new_var_decl(const char *name, struct exp *exp)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_VAR_DECL;
    stm->data.var_decl.name = name;
    stm->data.var_decl.exp = exp;

    return stm;
}

static struct stm *
stm_new_fun_decl(const char *name, Vector_T formals, struct stm *body)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_FUN_DECL;
    stm->data.fun_decl.name = name;
    stm->data.fun_decl.formals = formals;
    stm->data.fun_decl.body = body;

    return stm;
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

static struct stm*
stm_new_while(struct exp *cond, struct stm *body)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_WHILE;
    stm->data.while_cond.cond = cond;
    stm->data.while_cond.body = body;

    return stm;
}

static struct stm *
stm_new_break(void)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_BREAK;

    return stm;
}

static struct stm *
stm_new_return(struct exp *ret_exp)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_RETURN;
    stm->data.ret_exp = ret_exp;

    return stm;
}

static struct stm *
stm_new_print(struct exp *exp)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_PRINT;
    stm->data.print = exp;

    return stm;
}

static struct stm *
stm_new_exp_stm(struct exp *exp)
{
    struct stm *stm = malloc(sizeof(struct stm));

    stm->type = STM_EXP_STM;
    stm->data.exp_stm = exp;

    return stm;
}

static struct exp *
exp_new_assign(const char *name, struct exp *value)
{
    struct exp *exp = malloc(sizeof(struct stm));

    exp->type = EXP_ASSIGN;
    exp->data.assign.name = name;
    exp->data.assign.exp = value;

    return exp;
}

static struct exp *
exp_new_binary(enum tok_t op, struct exp *left, struct exp *right)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_BINARY;
    exp->data.binary.op = op;
    exp->data.binary.left = left;
    exp->data.binary.right = right;

    return exp;
}

static struct exp *
exp_new_unary(enum tok_t op, struct exp *operand)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_UNARY;
    exp->data.unary.op = op;
    exp->data.unary.exp = operand;

    return exp;
}

static struct exp *
exp_new_call(struct exp *callee, Vector_T params)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_CALL;
    exp->data.call.callee = callee;
    exp->data.call.params = params;

    return exp;
}

static struct exp *
exp_new_ident(const char *name)
{
    struct exp *exp = malloc(sizeof(struct exp));

    exp->type = EXP_IDENT;
    exp->data.name = name;

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

static void
pspaces(int indent)
{
    int i;

    for (i = 0; i < indent; ++i)
        putchar(' ');
}

void
print_stm(struct stm *stm, int indent)
{
    pspaces(indent);
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
                print_stm(*(struct stm **)Vector_get(vector, i), indent + 4);

            pspaces(indent);
            puts("]");
        } break;

        case STM_IF:
        {
            printf("[ if ");
            print_exp(stm->data.if_cond.cond);
            putchar('\n');

            pspaces(indent + 4);
            puts("[ then");
            print_stm(stm->data.if_cond.then_block, indent + 8);
            pspaces(indent + 4);
            puts("]");

            if (stm->data.if_cond.else_block != NULL)
            {
                pspaces(indent);
                puts("else");
                pspaces(indent);
                puts("[");
                print_stm(stm->data.if_cond.else_block, indent + 4);
            }

            pspaces(indent);
            puts("]");
        } break;

        case STM_WHILE:
        {
            printf("[ while ");
            print_exp(stm->data.while_cond.cond);
            putchar('\n');
            print_stm(stm->data.while_cond.body, indent + 4);

            pspaces(indent);
            puts("]");
        } break;

        case STM_BREAK:
            puts("[ break ]");
        break;

        case STM_RETURN:
            printf("[ return ");
            print_exp(stm->data.ret_exp);
            puts("]");
        break;

        case STM_VAR_DECL:
        {
            printf("[ %s = ", stm->data.var_decl.name);
            print_exp(stm->data.var_decl.exp);
            puts("]");
        } break;

        case STM_FUN_DECL:
        {
            int len, i;

            printf("[ %s = <fun", stm->data.fun_decl.name);
            len = Vector_length(stm->data.fun_decl.formals);
            for (i = 0; i < len; ++i)
            {
                printf
                (
                    " %s",
                    *(const char **)Vector_get(stm->data.fun_decl.formals, i)
                );
            }
            puts("> ]");
        } break;

        case STM_PRINT:
        {
            printf("[ print ");
            print_exp(stm->data.print);
            puts("]");
        } break;

        case STM_EXP_STM:
        {
            printf("[ expstm ");
            print_exp(stm->data.exp_stm);
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
            printf("( assign %s ", exp->data.assign.name);
            print_exp(exp->data.assign.exp);
            printf(") ");
        } break;

        case EXP_BINARY:
        {
            /* XXX op is just a token code */
            printf("( %i ", exp->data.binary.op);
            print_exp(exp->data.binary.left);
            print_exp(exp->data.binary.right);
            printf(") ");
        } break;

        case EXP_UNARY:
        {
            /* XXX op is just a token code */
            printf("( %i ", exp->data.unary.op);
            print_exp(exp->data.unary.exp);
            printf(") ");
        } break;

        case EXP_CALL:
        {
            int len, i;

            printf("(call ");
            print_exp(exp->data.call.callee);
            printf("with ");

            len = Vector_length(exp->data.call.params);
            for (i = 0; i < len; ++i)
                print_exp(*(struct exp**)Vector_get(exp->data.call.params, i));

            printf(") ");
        } break;

        case EXP_IDENT:
            printf("%s ", exp->data.name);
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

