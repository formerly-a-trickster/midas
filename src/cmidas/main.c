#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdbool.h>

int
main(int argc, const char *argv[])
{
    if (argc == 2)
    {
        struct intpr *intpr = intpr_new();
        interpret(intpr, argv[1]);
    }
    else
        puts("Usage: cmidas file");

    return 0;
}

