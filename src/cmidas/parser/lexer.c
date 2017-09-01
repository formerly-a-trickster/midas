#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lexer.h"

#define T Lexer_T

#define is_at_end(LEX) ((*(LEX)->index) == '\0')

#define is_alpha(C) (((C) >= 'a' && (C) <= 'z') || \
                     ((C) >= 'A' && (C) <= 'Z') || \
                      (C) == '_')

#define is_numeric(C) ((C) >= '0' && (C) <= '9')

#define is_alpha_num(C) (is_alpha(C) || is_numeric(C))

struct T
{
    const char *buffer;
    const char *index;
    const char *start;
           int  lineno;
           int  colno;
          bool  had_error;
          char  error_msg[256];
};

struct keyword
{
    const char *name;
    int length;
    enum tok_t type;
} keywords[] =
{
    { "and"  , 4, TOK_AND   },
    { "break", 6, TOK_BREAK },
    { "do"   , 3, TOK_DO    },
    { "else" , 5, TOK_ELSE  },
    { "end"  , 4, TOK_END   },
    { "false", 6, TOK_FALSE },
    { "for"  , 4, TOK_FOR   },
    { "fun"  , 4, TOK_FUN   },
    { "if"   , 3, TOK_IF    },
    { "or"   , 3, TOK_OR    },
    { "print", 6, TOK_PRINT },
    { "true" , 5, TOK_TRUE  },
    { "var"  , 4, TOK_VAR   },
    { "while", 6, TOK_WHILE },
    { NULL   , 0, TOK_EOF   }
};

static struct tok *tok_new   (T lex, enum tok_t type);
static struct tok *identifier(T lex);
static struct tok *number    (T lex);
static struct tok *string    (T lex);

static char char_next   (T lex);
static char lookahead   (T lex);
static bool char_matches(T lex, const char c);

static void skip_space(T lex);
static void skip_line (T lex);

T
Lex_new(void)
{
    T lex;

    lex = malloc(sizeof(struct T));
    lex->lineno = 1;
    lex->colno  = 0;

    return lex;
}

void
Lex_feed(T lex, const char *buffer)
{
    lex->buffer = buffer;
    lex->index  = buffer;
    lex->start  = buffer;
}

struct tok *
Lex_get_tok(T lex)
{
    char c;

    skip_space(lex);
    lex->start = lex->index;
    c = char_next(lex);
    if (is_alpha(c))
        return identifier(lex);

    if (is_numeric(c))
        return number(lex);

    switch (c)
    {
        case '!':
            if (char_matches(lex, '='))
                return tok_new(lex, TOK_BANG_EQUAL);
            else
                return tok_new(lex, TOK_BANG);

        case ',':
            return tok_new(lex, TOK_COMMA);

        case '=':
            if (char_matches(lex, '='))
                return tok_new(lex, TOK_EQUAL_EQUAL);
            else
                return tok_new(lex, TOK_EQUAL);

        case '>':
            if (char_matches(lex, '='))
                return tok_new(lex, TOK_GREAT_EQUAL);
            else
                return tok_new(lex, TOK_GREAT);

        case '#':
            skip_line(lex);
            /* XXX Eliminate recursion from lexer */
            return Lex_get_tok(lex);

        case '<':
            if (char_matches(lex, '='))
                return tok_new(lex, TOK_LESS_EQUAL);
            else
                return tok_new(lex, TOK_LESS);

        case '-':
            return tok_new(lex, TOK_MINUS);

        case '(':
            return tok_new(lex, TOK_PAREN_LEFT);

        case ')':
            return tok_new(lex, TOK_PAREN_RIGHT);

        case '%':
            return tok_new(lex, TOK_PERCENT);

        case '+':
            if (char_matches(lex, '+'))
                return tok_new(lex, TOK_PLUS_PLUS);
            else
                return tok_new(lex, TOK_PLUS);

        case '"':
            return string(lex);

        case ';':
            return tok_new(lex, TOK_SEMICOLON);

        case '/':
            if (char_matches(lex, '/'))
                return tok_new(lex, TOK_SLASH_SLASH);
            else
                return tok_new(lex, TOK_SLASH);

        case '*':
            return tok_new(lex, TOK_STAR);

        case '\0':
            return tok_new(lex, TOK_EOF);

        default:
            sprintf
            (
                lex->error_msg,
                "Line: %i\n"
                "Col : %i\n"
                "Encoutered unknown glyph `%c`.",
                lex->lineno, lex->colno, c
            );
            lex->had_error = true;
            return NULL;
    }
}

void
Lex_get_err(T lex)
{
    printf("%s\n", lex->error_msg);
}

void
print_tok(struct tok *tok)
{
    printf("<\"%s\", type %i, len %i, line %i, col %i>\n",
        tok->lexeme, tok->type, tok->length, tok->lineno, tok->colno);
}

static struct tok *
tok_new(T lex, enum tok_t type)
{
    struct tok *tok;
    char *lexeme;
    int length;

    length = lex->index - lex->start;

    tok = malloc(sizeof(struct tok));
    lexeme = malloc(length + 1);
    strncpy(lexeme, lex->start, length);
    lexeme[length] = '\0';

    tok->lexeme = lexeme;
    tok->length = length + 1;
    tok->type   = type;
    tok->lineno = lex->lineno;
    tok->colno  = lex->colno - length;

    return tok;
}

static struct tok *
identifier(T lex)
{
    struct tok *tok;
    struct keyword *keyw;

    while (is_alpha_num(lookahead(lex)))
        char_next(lex);

    tok = tok_new(lex, TOK_IDENTIFIER);
    for (keyw = keywords; keyw->name != NULL; ++keyw)
    {
        if (tok->length == keyw->length &&
            strcmp(tok->lexeme, keyw->name) == 0)
        {
            tok->type = keyw->type;
            break;
        }
    }

    return tok;
}

static struct tok *
number(T lex)
{
    bool is_double = false;

    for (;;)
    {
        if (is_numeric(lookahead(lex)))
            char_next(lex);
        else if (lookahead(lex) == '.')
        {
            if (is_double == false)
            {
                is_double = true;
                char_next(lex);
            }
            else
                break;
        }
        else
            break;
    }

    if (is_double)
        return tok_new(lex, TOK_DOUBLE);
    else
        return tok_new(lex, TOK_INTEGER);
}

static struct tok *
string(T lex)
{
    struct tok *tok;

    lex->start = lex->index;
    while (lookahead(lex) != '"')
        char_next(lex);

    tok = tok_new(lex, TOK_STRING);
    char_next(lex);

    return tok;
}

static char
char_next(T lex)
{
    char next;

    next = *lex->index;
    ++lex->index;
    ++lex->colno;

    return next;
}

static char
lookahead(T lex)
{
    return *lex->index;
}

static bool
char_matches(T lex, const char c)
{
    if (is_at_end(lex))
        return false;
    else if (*lex->index == c)
    {
        char_next(lex);
        return true;
    }
    else
        return false;
}

static void
skip_space(T lex)
{
    char c;
    for (;;)
    {
        c = lookahead(lex);
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                char_next(lex);
                break;

            case '\n':
                ++lex->lineno;
                lex->colno = 0;
                char_next(lex);
                break;

            default:
                return;
        }
    }
}

static void
skip_line(T lex)
{
    char c;
    for (;;)
    {
        c = lookahead(lex);
        switch (c)
        {
            case '\n':
                ++lex->lineno;
                lex->colno = 0;
                char_next(lex);
                return;

            case '\0':
                return;

            default:
                char_next(lex);
                break;
        }
    }
}


#undef T
#undef is_at_end
#undef is_alpha
#undef is_numeric
#undef is_alpha_num

