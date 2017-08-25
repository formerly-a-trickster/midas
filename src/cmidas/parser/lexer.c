#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "error.h"
#include "lexer.h"

struct keyword keywords[] =
{
    { "do"   , 3, TOK_DO      },
    { "else" , 5, TOK_ELSE    },
    { "end"  , 4, TOK_END     },
    { "false", 6, TOK_FALSE   },
    { "for"  , 4, TOK_FOR     },
    { "if"   , 3, TOK_IF      },
    { "print", 6, TOK_PRINT   },
    { "true" , 5, TOK_TRUE    },
    { "var"  , 4, TOK_VAR     },
    { "while", 6, TOK_WHILE   },
    { NULL   , 0, ERR_UNKNOWN }
};

static struct tok *tok_new   (struct lex_state *, enum tok_type);
static struct tok *identifier(struct lex_state *);
static struct tok *number    (struct lex_state *);
static struct tok *string    (struct lex_state *);

static char char_next   (struct lex_state *);
static char lookahead   (struct lex_state *);
static bool char_matches(struct lex_state *, const char);

static void skip_space(struct lex_state *);
static void skip_line (struct lex_state *);

static inline bool is_at_end   (struct lex_state *);
static inline bool is_alpha    (char);
static inline bool is_numeric  (char);
static inline bool is_alpha_num(char);

void
lex_init(struct lex_state *lex)
{
    lex->lineno = 1;
    lex->colno  = 0;
}

int
lex_feed(struct lex_state *lex, const char *path)
{
    FILE *source;
    int file_size, bytes_read;
    char* buffer;

    source = fopen(path, "rb");
    if (source == NULL)
    {
        sprintf(lex->error_msg, "Failed to read `%s`. "
                                "Could not open file.", path);
        lex->had_error = true;
        return 1;
    }

    /* XXX the standard does not guarantee that this will work */
    fseek(source, 0L, SEEK_END);
    file_size = ftell(source);
    rewind(source);

    buffer = malloc(file_size + 1);
    if (buffer == NULL)
    {
        sprintf(lex->error_msg, "Failed to read `%s`. "
                                "Not enough memory.", path);
        lex->had_error = true;
        return 2;
    }

    bytes_read = fread(buffer, sizeof(char), file_size, source);
    if (bytes_read < file_size)
    {
        sprintf(lex->error_msg, "Failed to read `%s`. "
                                "Reading stopped midway.", path);
        lex->had_error = true;
        return 3;
    }

    buffer[file_size] = '\0';
    fclose(source);

    lex->buffer = buffer;
    lex->index  = buffer;
    lex->start  = buffer;
}

struct tok *
lex_get_tok(struct lex_state *lex)
{
    skip_space(lex);
    lex->start = lex->index;
    const char c = char_next(lex);

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
            return lex_get_tok(lex);

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
            return tok_new(lex, TOK_SLASH);

        case '*':
            return tok_new(lex, TOK_STAR);

        case '\0':
            return tok_new(lex, TOK_EOF);

        default:
            return tok_new(lex, ERR_UNKNOWN);
    }
}

void
print_tok(struct tok *tok)
{
    printf("<\"%s\", type %i, len %i, line %i, col %i>\n",
        tok->lexeme, tok->type, tok->length, tok->lineno, tok->colno);
}

static struct tok *
tok_new(struct lex_state *lex, enum tok_type type)
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
identifier(struct lex_state *lex)
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
number(struct lex_state *lex)
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
string(struct lex_state *lex)
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
char_next(struct lex_state *lex)
{
    char next;

    next = *lex->index;
    ++lex->index;
    ++lex->colno;

    return next;
}

static char
lookahead(struct lex_state *lex)
{
    return *lex->index;
}

static bool
char_matches(struct lex_state *lex, const char c)
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
skip_space(struct lex_state *lex)
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
skip_line(struct lex_state *lex)
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

static inline bool
is_at_end(struct lex_state *lex)
{
    return *lex->index == '\0';
}

static inline bool
is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static inline bool
is_numeric(char c)
{
    return c >= '0' && c <= '9';
}

static inline bool
is_alpha_num(char c)
{
    return is_alpha(c) || is_numeric(c);
}

