#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct lex_state
{
    FILE* source;             // Source file currently read.
    char buffer[BUFFER_SIZE]; // Double buffered input from source.
    int current;              // Current cell of the buffer.
    int limit;                // The index beyond which we can't roll back.
    int lexeme;               // Start of the currently read lexeme.
    int lineno;               // Currnet line of the source file.
};

void lex(FILE* source);
static struct lex_state* new_lex(void);

static void buffer_chars(struct lex_state*);
static char next_char(struct lex_state*);
static void roll_char(struct lex_state*);
static bool is_at_end(struct lex_state*);

void
lex(FILE* source)
{
    struct lex_state* lex = new_lex();
    lex->source = source;
    buffer_chars(lex);

    while (!is_at_end(lex))
        printf("%c", next_char(lex));
}

static struct lex_state*
new_lex(void)
{
    // XXX unchecked malloc
    struct lex_state* lex = malloc(sizeof(struct lex_state));
    lex->source = NULL;
    lex->current = 0;
    lex->limit = 0;
    lex->lexeme = 0;
    lex->lineno = 0;

    return lex;
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
roll_char(struct lex_state* lex)
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

