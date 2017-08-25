#include "parser.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdbool.h>

int
main(int argc, const char *argv[])
{
    if (argc == 2)
    {
        Parser_T par;
        Vector_T ast;
        Interpreter_T intpr;

        par = Par_new();
        ast = Par_parse(par, argv[1]);
        if (ast == NULL)
            return 1;

        intpr = Intpr_new();
        Intpr_run(intpr, argv[1], ast);
    }
    else
        puts("Usage: cmidas file");

    return 0;
}

