#ifndef lexer_h
#define lexer_h
#include <stdio.h>
#include <stdbool.h>

#define HALF_BUFFER_SIZE 64
#define BUFFER_SIZE (HALF_BUFFER_SIZE * 2)

enum tok_type
{
    TOK_LEFT_PAREN,
    TOK_RIGHT_PAREN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,

    TOK_NUMBER
};

struct token
{
    enum tok_type type;
    const char* lexeme;
    int length;
    int lineno;
};

void lex(FILE* source);

#endif
