#include <stdio.h>
#include <stdbool.h>

#include "parser.h"

int
main(int argc, const char *argv[])
{
    if (argc == 2)
    {
        Parser_T par;
        Vector_T ast;
        int i, len;

        par = Par_new();
        ast = Par_parse(par, argv[1]);
        if (ast == NULL)
            return 1;

        len = Vector_length(ast);
        for (i = 0; i < len; ++i)
            print_stm(*(struct stm **)Vector_get(ast, i), 0);
    }
    else
        puts("Usage: cmidas file");

    return 0;
}

