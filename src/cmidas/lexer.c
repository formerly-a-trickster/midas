#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static struct token* token_new(enum tok_type, const char*, int, int);

static void buffer_chars(struct lex_state*);
static char next_char(struct lex_state*);
static void rollback(struct lex_state*);
static bool is_at_end(struct lex_state*);
static void skip_whitespace(struct lex_state*);

void
lex_init(struct lex_state* lex)
{
    // XXX unchecked malloc
    lex->source = NULL;
    lex->current = 0;
    lex->limit = 0;
    lex->lexeme = 0;
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
    char c = next_char(lex);
    switch (c)
    {
        case '(': return token_new(TOK_LEFT_PAREN , NULL, 2, lex->lineno);
        case ')': return token_new(TOK_RIGHT_PAREN, NULL, 2, lex->lineno);
        case '+': return token_new(TOK_PLUS       , NULL, 2, lex->lineno);
        case '-': return token_new(TOK_MINUS      , NULL, 2, lex->lineno);
        case '*': return token_new(TOK_STAR       , NULL, 2, lex->lineno);
        case '/': return token_new(TOK_SLASH      , NULL, 2, lex->lineno);
        case '%': return token_new(TOK_PERCENT    , NULL, 2, lex->lineno);
        case '\0':return token_new(TOK_EOF        , NULL, 2, lex->lineno);
        default:
            printf("Encountered unknown symbol '%c' while parsing.\n", c);
            exit(1);
    }
}

static struct token* // XXX lexeme is unhandled
token_new(enum tok_type type, const char* lexeme, int length, int lineno)
{
    struct token* tok = malloc(sizeof(struct token));

    tok->type = type;
    tok->length = length;
    tok->lineno = lineno;

    return tok;
}

static void
buffer_chars(struct lex_state* lex)
{
    size_t bytes_read;
    bytes_read = fread(
        &lex->buffer[lex->current],
        sizeof(char),
        HALF_BUFFER_SIZE * sizeof(char),
        lex->source
    );

    // XXX code like this should be abstracted out
    if (bytes_read != HALF_BUFFER_SIZE * sizeof(char))
    {
        if (feof(lex->source))
            lex->buffer[(lex->current + bytes_read) % BUFFER_SIZE] = '\0';
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
    char next_char = lex->buffer[lex->current];
    lex->current = (lex->current + 1) % BUFFER_SIZE;
    if (lex->current % HALF_BUFFER_SIZE == 0)
    {
        buffer_chars(lex);
        lex->limit = (lex->current + HALF_BUFFER_SIZE) % BUFFER_SIZE;
    }
    return next_char;
}

static void
rollback(struct lex_state* lex)
{
    if (lex->current == lex->limit)
    {
        printf("Parser Error:\nRequested rollback beyond limit");
        exit(1);
    }
    lex->current = (lex->current - 1) % BUFFER_SIZE;
}

static bool
is_at_end(struct lex_state* lex)
{
    return lex->buffer[lex->current] == '\0';
}

static void
skip_whitespace(struct lex_state* lex)
{
    for (;;)
    {
        char c = next_char(lex);
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                break;

            case '\n':
                ++lex->lineno;
                break;

            default:
                rollback(lex);
                return;
        }
    }
}

