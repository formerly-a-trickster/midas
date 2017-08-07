#include "lexer.h"
#include "utils.h"
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
    FILE* source = fopen(file, "r");
    struct lex_state lex;
    struct token* t;

    lex_init(&lex);
    lex_feed(&lex, source);

    do
    {
        t = lex_get_token(&lex);
        printf("(%i %i)", t->type, t->lineno);
    }
    while (t->type != TOK_EOF);

    fclose(source);
}

