#ifndef MD_lexer_h_
#define MD_lexer_h_

#include <stdio.h>
#include <stdbool.h>

#define T Lex_T

typedef struct T *T;

enum tok_type
{
    TOK_BANG,

    TOK_EOF, TOK_COMMA, TOK_EQUAL, TOK_GREAT, TOK_LESS, TOK_MINUS,
    TOK_PAREN_LEFT, TOK_PAREN_RIGHT, TOK_PERCENT, TOK_PLUS, TOK_SEMICOLON,
    TOK_SLASH, TOK_STAR,

    TOK_GREAT_EQUAL, TOK_LESS_EQUAL, TOK_BANG_EQUAL, TOK_EQUAL_EQUAL,
    TOK_PLUS_PLUS,

    TOK_DOUBLE, TOK_INTEGER, TOK_STRING,

    TOK_DO, TOK_ELSE, TOK_END, TOK_FALSE, TOK_FOR, TOK_IDENTIFIER, TOK_IF,
    TOK_PRINT, TOK_TRUE, TOK_VAR, TOK_WHILE,

    ERR_UNKNOWN
};

struct keyword
{
    const char *name;
    int length;
    enum tok_type type;
};

struct tok
{
    const char *lexeme;
    enum tok_type type;
    int length;
    int lineno;
    int colno;
};

         T  Lex_new (void);
      void  Lex_feed(T lex, const char *buffer);
struct tok *Lex_tok (T lex);

void        print_tok(struct tok *);

#undef T

#endif

