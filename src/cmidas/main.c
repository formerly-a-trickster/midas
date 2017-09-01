#include <stdio.h>
#include <stdbool.h>

#include "parser.h"
#include "vector.h"
#include "interpreter.h"

int
main(int argc, const char *argv[])
{
    if (argc == 2)
    {
        Parser_T par;
        Vector_T ast;
        Interpr_T intpr;
        int len, i;

        par = Par_new();
        ast = Par_parse(par, argv[1]);
        if (ast == NULL)
            return 1;

        len = Vector_length(ast);
        for (i = 0; i < len; ++i)
            print_stm(*(struct stm **)Vector_get(ast, i), 0);

        intpr = Interpr_new();
        Interpr_run(intpr, ast);
    }
    else
        puts("Usage: cmidas file");

    return 0;
}

