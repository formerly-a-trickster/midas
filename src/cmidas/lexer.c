#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct lex_state*
lex_new(void)
{
    struct lex_state* lex = malloc(sizeof(struct lex_state)); //XXX
    lex->source = NULL;
    lex->current = 0;
    lex->limit = 0;
    lex->lexeme = 0;
    lex->lineno = 0;

    return lex;
}

void
lex_from_file(struct lex_state* lex, const char* source)
{
    FILE* source_file = fopen(source, "r");

    if (source_file) {
        lex->source = source_file;
        lex_buffer_chars(lex);
    }
    else {
        printf(
            "Error in parser initialization:\nCould not open '%s'.\n",
            source
        );
        exit(1);
    }
}

void
lex_buffer_chars(struct lex_state* lex)
{
    size_t bytes_read;
    bytes_read = fread(
        &lex->buffer[lex->current],
        sizeof(char),
        HALF_BUFFER_SIZE * sizeof(char),
        lex->source
    );

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

char
lex_next_char(struct lex_state* lex)
{
    char next_char = lex->buffer[lex->current];
    lex->current = (lex->current + 1) % BUFFER_SIZE;
    if (lex->current % HALF_BUFFER_SIZE == 0)
    {
        lex_buffer_chars(lex);
        lex->limit = (lex->current + HALF_BUFFER_SIZE) % BUFFER_SIZE;
    }
    return next_char;
}

void
lex_roll_char(struct lex_state* lex)
{
    if (lex->current == lex->limit)
    {
        printf("Parser Error:\nRequested rollback beyond limit");
        exit(1);
    }
    lex->current = (lex->current - 1) % BUFFER_SIZE;
}

bool
lex_is_at_end(struct lex_state* lex)
{
    return lex->buffer[lex->current] == '\0';
}

