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

static struct tok *tok_new(struct lex_state *, enum tok_type);
static struct tok *identifier(struct lex_state *);
static struct tok *number(struct lex_state *);
static struct tok *string(struct lex_state *);

static void buffer_chars(struct lex_state *);
static char char_next(struct lex_state *);
static char lookahead(struct lex_state *);
static bool char_matches(struct lex_state *, const char);

static void skip_whitespace(struct lex_state *);
static void skip_line(struct lex_state *);

static inline bool is_at_end(struct lex_state *);
static inline bool is_alpha(char);
static inline bool is_numeric(char);
static inline bool is_alpha_num(char);

void
lex_init(struct lex_state *lex)
{
    lex->source = NULL;
    lex->index = 0;
    lex->chars_left = 0;
    lex->tok_start = 0;
    lex->lineno = 1;
    lex->colno = 1;
}

void
lex_feed(struct lex_state *lex, const char *path)
/* XXX ideally, the lexer would not have to keep track of actual files and
  would just be fed characters from an overarching structure.                */
/* XXX this just segfaults on attempting to open a file that's not there     */
{
    FILE *source = fopen(path, "r");
    lex->path = path;
    lex->source = source;
    buffer_chars(lex);
}

struct tok *
lex_get_tok(struct lex_state *lex)
{
    skip_whitespace(lex);
    lex->tok_start = lex->index;
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

static struct tok *
tok_new(struct lex_state *lex, enum tok_type type)
{
    struct tok *tok = malloc(sizeof(struct tok));
    const int start = lex->tok_start;
    const int end = lex->index;
    char *lexeme;

    tok->type = type;
    tok->lineno = lex->lineno;

    if (start < end)
    /*   0 1 2 3 4 5 6 7 8 9     This token has a length of 4. We allocate 5
        |v|a|r| |s|i|z|e| |=|... char-widths for it. Besides the char, it needs
                 ^       ^       to be null terminated. This null character
           start |       | end   resides at index 4.                         */
    {
        const size_t size = end - start + 1;

        lexeme = malloc((size) * sizeof(char));
        strncpy(lexeme, &lex->buffer[start], size);
        lexeme[size - 1] = '\0';
        tok->length = size;
    }
    else
    /* While reading identifier characters, the index went beyond BUFFER_SIZE,
       wrapping back to position 0 and triggering a buffer refill.

       |z|e|_|=|_|  . . .  |_|s|i|
            ^                 ^
            | ended here      | started here   length of 4                   */
    {
        const size_t first_half = BUFFER_SIZE - start;

        lexeme = malloc((first_half + end + 1) * sizeof(char));
        memcpy(lexeme, &lex->buffer[start], first_half);
        if (end > 0)
            memcpy(lexeme + first_half, &lex->buffer[0], end);
        lexeme[first_half + end] = '\0';
        tok->length = first_half + end + 1;
    }

    tok->lexeme = lexeme;
    tok->colno = lex->colno - tok->length;

    switch (tok->type)
    {
        case ERR_UNKNOWN:
            err_at_tok(lex->path, tok,
                "\n    Encountered unknown glyph `%s` while lexing.\n\n",
                tok->lexeme);
            break;

        default:
            break;
    }

    /* XXX it would be helpful to add a debug switch that can show internal
       lexing state */
    return tok;
}

static struct tok *
identifier(struct lex_state *lex)
{
    while (is_alpha_num(lookahead(lex)))
        char_next(lex);

    struct tok *tok = tok_new(lex, TOK_IDENTIFIER);
    for (struct keyword *keyw = keywords; keyw->name != NULL; ++keyw)
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
    lex->tok_start = lex->index;
    while (lookahead(lex) != '"')
        char_next(lex);

    struct tok *tok = tok_new(lex, TOK_STRING);
    char_next(lex);

    return tok;
}

static void
buffer_chars(struct lex_state *lex)
{
    const size_t bytes_read = fread(
        &lex->buffer[lex->index],
        sizeof(char),
        HALF_BUFFER_SIZE * sizeof(char),
        lex->source);

    lex->chars_left += HALF_BUFFER_SIZE;

    /* XXX code like this should be abstracted out */
    if (bytes_read != HALF_BUFFER_SIZE * sizeof(char))
    {
        if (feof(lex->source))
            lex->buffer[(lex->index + bytes_read) % BUFFER_SIZE] = '\0';
        else if (ferror(lex->source))
        {
            // XXX this should be an IO error
            puts("Parser Error:\nCould not buffer chars.\n");
            exit(1);
        }
    }
}

static char
char_next(struct lex_state *lex)
{
    const char char_next = lex->buffer[lex->index];
    /* XXX Nothing stops chars_lex from going negative and reading the same
       buffer ad infinitum.                                                  */
    lex->index = (lex->index + 1) % BUFFER_SIZE;
    --lex->chars_left;
    ++lex->colno;
    if (lex->chars_left == 0)
        buffer_chars(lex);
    return char_next;
}

static char
lookahead(struct lex_state *lex)
{
    return lex->buffer[lex->index];
}

static bool
char_matches(struct lex_state *lex, const char c)
{
    if (is_at_end(lex))
        return false;
    else if (lex->buffer[lex->index] == c)
    {
        char_next(lex);
        return true;
    }
    else
        return false;
}

static void
skip_whitespace(struct lex_state *lex)
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
    return lex->buffer[lex->index] == '\0';
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

