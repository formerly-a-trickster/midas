#include "parser.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdbool.h>

void ast_file(const char* file);

int main(int argc, const char* argv[])
{
    if (argc == 2)
    {
        struct intpr intpr;
        interpret(&intpr, argv[1]);
    }
    else
        puts("Usage: cmidas file");

    return 0;
}

