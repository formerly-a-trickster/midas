#ifndef MD_lexer_h_
#define MD_lexer_h_

#include <stdio.h>
#include <stdbool.h>

#define HALF_BUFFER_SIZE 50
#define BUFFER_SIZE (HALF_BUFFER_SIZE * 2)

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

struct lex_state
{
    const char *buffer;
    const char *index;
    const char *start;
           int  lineno;
           int  colno;
          bool  had_error;
          char  error_msg[256];
};

struct tok
{
    const char *lexeme;
    enum tok_type type;
    int length;
    int lineno;
    int colno;
};

void        lex_init   (struct lex_state *);
int         lex_feed   (struct lex_state *, const char *);
struct tok *lex_get_tok(struct lex_state *);
void        print_tok  (struct tok *);

#endif
