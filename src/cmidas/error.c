#include "error.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

void
err_at_tok(const char* path, struct tok* tok, const char* format, ...)
{
    show_err_head(path);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    show_err_code(path, tok->lineno, tok->colno, tok->length);
    /* XXX there should be a more elegant shutdown solution than just bailing
       out on the first error */
    exit(1);
}

void
show_err_head(const char* path)
{
    int path_len = strlen(path);
    for (int i = 0; i < 80 - path_len; ++i)
        putchar('-');
    printf(TXT_BOLD "%s\n" TXT_CLEAR, path);
}

void
show_err_code(const char* path, int lineno, int colno, int length)
{
    FILE* file = fopen(path, "r");
    char line[512];
    int i = 1, start = lineno - 3, end = lineno + 3;


    while (fgets(line, sizeof line, file) != NULL)
    {
        if (i == lineno)
        {
            printf(TXT_BOLD "%4i|", i);
            for (int i = 0; i < colno - 1; ++i)
                putchar(line[i]);

            printf(TXT_RED);
            for (int i = colno - 1; i < colno + length - 1; ++i)
                putchar(line[i]);

            printf(TXT_CLEAR TXT_BOLD);
            for (int i = colno + length - 1; line[i] != '\0'; ++i)
                putchar(line[i]);

            printf (TXT_CLEAR);
        }
        else if (i > start && i < end)
            printf("%4i|%s", i, line);

        i++;
    }

    fclose(file);
}

