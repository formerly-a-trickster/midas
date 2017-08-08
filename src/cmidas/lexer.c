#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static struct token* token_new(struct lex_state*, enum tok_type);
static struct token* identifier(struct lex_state*);
static struct token* number(struct lex_state*);
static struct token* string(struct lex_state*);

static void buffer_chars(struct lex_state*);
static char next_char(struct lex_state*);
static char lookahead(struct lex_state*);
static bool next_matches(struct lex_state*, const char);

static void skip_whitespace(struct lex_state*);
static void skip_line(struct lex_state*);

static inline bool is_at_end(struct lex_state*);
static inline bool is_alpha(char);
static inline bool is_numeric(char);
static inline bool is_alpha_num(char);

void
lex_init(struct lex_state* lex)
{
    lex->source = NULL;
    lex->index = 0;
    lex->chars_left = 0;
    lex->tok_start = 0;
    lex->lineno = 0;
}

void
lex_feed(struct lex_state* lex, FILE* source)
{
    lex->source = source;
    buffer_chars(lex);
}

struct token*
lex_get_token(struct lex_state* lex)
{
    skip_whitespace(lex);
    lex->tok_start = lex->index;
    const char c = next_char(lex);

    if (is_alpha(c))
        return identifier(lex);

    if (is_numeric(c))
        return number(lex);

    switch (c)
    {
        case ',':
            return token_new(lex, TOK_COMMA);
        case '=':
            if (next_matches(lex, '='))
                return token_new(lex, TOK_EQUAL_EQUAL);
            else
                return token_new(lex, TOK_EQUAL);
        case '>':
            if (next_matches(lex, '='))
                return token_new(lex, TOK_GREAT_EQUAL);
            else
                return token_new(lex, TOK_GREAT);
        case '#':
            skip_line(lex);
            /* XXX Find better solution */
            return lex_get_token(lex);
        case '<':
            if (next_matches(lex, '='))
                return token_new(lex, TOK_LESS_EQUAL);
            else
                return token_new(lex, TOK_LESS);
        case '-':
            return token_new(lex, TOK_MINUS);
        case '(':
            return token_new(lex, TOK_PAREN_LEFT);
        case ')':
            return token_new(lex, TOK_PAREN_RIGHT);
        case '%':
            return token_new(lex, TOK_PERCENT);
        case '+':
            if (next_matches(lex, '+'))
                return token_new(lex, TOK_PLUS_PLUS);
            else
                return token_new(lex, TOK_PLUS);
        case '"':
            return string(lex);
        case ';':
            return token_new(lex, TOK_SEMICOLON);
        case '/':
            return token_new(lex, TOK_SLASH);
        case '*':
            return token_new(lex, TOK_STAR);
        case '\0':
            return token_new(lex, TOK_EOF);
        default:
            printf("Parser Error:\n"
                   "Encountered unknown symbol '%c' while parsing.\n", c);
            exit(1);
    }
}

void
print_tok(struct token* tok)
{
    if (tok->type == TOK_STRING)
        printf("(\"%s\") ", tok->lexeme);
    else
        printf("(%s) ", tok->lexeme);
}

static struct token*
token_new(struct lex_state* lex, enum tok_type type)
{
    struct token* tok = malloc(sizeof(struct token));
    const int start = lex->tok_start;
    const int end = lex->index;
    char* lexeme;

    tok->type = type;
    tok->lineno = lex->lineno;

    if (start < end)
    {
        const size_t size = end - start;

        lexeme = malloc((size + 1) * sizeof(char));
        memcpy(lexeme, &lex->buffer[start], size);
        lexeme[size] = '\0';
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
        tok->length = first_half + end;
    }

    tok->lexeme = lexeme;

    return tok;
}

static struct token*
identifier(struct lex_state* lex)
{
    /* printf("Scanning for identifier\n"); */
    while (is_alpha_num(lookahead(lex)))
        next_char(lex);

    return token_new(lex, TOK_IDENTIFIER);
}

static struct token*
number(struct lex_state* lex)
{
    while (is_numeric(lookahead(lex)))
        next_char(lex);

    return token_new(lex, TOK_NUMBER);
}

static struct token*
string(struct lex_state* lex)
{
    lex->tok_start = lex->index;
    while (lookahead(lex) != '"')
        next_char(lex);

    struct token* tok = token_new(lex, TOK_STRING);
    next_char(lex);

    return tok;
}

static void
buffer_chars(struct lex_state* lex)
{
    const size_t bytes_read = fread(
        &lex->buffer[lex->index],
        sizeof(char),
        HALF_BUFFER_SIZE * sizeof(char),
        lex->source
    );

    lex->chars_left += HALF_BUFFER_SIZE;

    /* XXX code like this should be abstracted out */
    if (bytes_read != HALF_BUFFER_SIZE * sizeof(char))
    {
        if (feof(lex->source))
            lex->buffer[(lex->index + bytes_read) % BUFFER_SIZE] = '\0';
        else if (ferror(lex->source))
        {
            puts("Parser Error:\nCould not buffer chars.\n");
            exit(1);
        }
    }
}

static char
next_char(struct lex_state* lex)
{
    const char next_char = lex->buffer[lex->index];
    /* printf("char '%c'\n", next_char); */
    /* XXX Nothing stops chars_lex from going negative and reading the same
       buffer ad infinitum.                                                  */
    lex->index = (lex->index + 1) % BUFFER_SIZE;
    --lex->chars_left;
    if (lex->chars_left == 0)
        buffer_chars(lex);
    return next_char;
}

static char
lookahead(struct lex_state* lex)
{
    return lex->buffer[lex->index];
}

static bool
next_matches(struct lex_state* lex, const char c)
{
    if (is_at_end(lex))
        return false;
    else if (lex->buffer[lex->index] == c)
    {
        next_char(lex);
        return true;
    }
    else
        return false;
}

static void
skip_whitespace(struct lex_state* lex)
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
                next_char(lex);
                break;

            case '\n':
                ++lex->lineno;
                next_char(lex);
                break;

            default:
                return;
        }
    }
}

static void
skip_line(struct lex_state* lex)
{
    char c;
    for (;;)
    {
        c = lookahead(lex);
        switch (c)
        {
            case '\n':
                ++lex->lineno;
                return;

            case '\0':
                return;

            default:
                next_char(lex);
                break;
        }
    }
}

static inline bool
is_at_end(struct lex_state* lex)
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

