#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdbool.h>

void ast_file(const char* file);

int main(int argc, const char* argv[])
{
    if (argc == 2)
        ast_file(argv[1]);
    else
        puts("Usage: cmidas file");

    return 0;
}

void
ast_file(const char* file)
{
    FILE* source = fopen(file, "r");
    struct par_state par;
    struct expr* ast;

    par_init(&par);
    ast = par_read(&par, source);

    ast_print(ast);
}

