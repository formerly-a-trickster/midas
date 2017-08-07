#include "lexer.h"
#include <stdio.h>
#include <stdbool.h>

void lex_file(const char* file);

int main(int argc, const char* argv[])
{
    if (argc == 2)
        lex_file(argv[1]);
    else
        printf("Usage: cmidas [file]\n");

    return 0;
}

void
lex_file(const char* file)
{
    struct lex_state* lex = lex_new();

    lex_from_file(lex, file);
    while (1)
    {
        if (!lex_is_at_end(lex))
            printf("%c", lex_next_char(lex));
        else
            break;
    }
    fclose(lex->source);
}

