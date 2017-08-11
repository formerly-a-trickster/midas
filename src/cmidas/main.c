#include "parser.h"
#include "interpreter.h"

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
ast_file(const char* path)
{
    struct par_state par;
    struct stm* program_ast;

    par_init(&par);
    program_ast = par_read(&par, path);

    print_stm(program_ast);
}

