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
    FILE* source = fopen(file, "r");
    lex(source);
    fclose(source);
}

