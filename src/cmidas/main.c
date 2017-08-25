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
        struct intpr *intpr;
        Vector_T ast;

        par = Par_new();
        ast = parse(par, argv[1]);
        if (ast == NULL)
            return 1;

        intpr = intpr_new();
        interpret(intpr, argv[1], ast);
    }
    else
        puts("Usage: cmidas file");

    return 0;
}

