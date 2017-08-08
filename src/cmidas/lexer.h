#ifndef MD_lexer_h
#define MD_lexer_h
#include <stdio.h>
#include <stdbool.h>

#define HALF_BUFFER_SIZE 50
#define BUFFER_SIZE (HALF_BUFFER_SIZE * 2)

enum tok_type
{
    TOK_COMMA       =  0,
    TOK_EQUAL       =  1,
    TOK_GREAT       =  2,
    TOK_LESS        =  3,
    TOK_MINUS       =  4,
    TOK_PAREN_LEFT  =  5,
    TOK_PAREN_RIGHT =  6,
    TOK_PERCENT     =  7,
    TOK_PLUS        =  8,
    TOK_SEMICOLON   =  9,
    TOK_SLASH       = 10,
    TOK_STAR        = 11,

    TOK_GREAT_EQUAL = 12,
    TOK_LESS_EQUAL  = 13,
    TOK_BANG_EQUAL  = 14,
    TOK_EQUAL_EQUAL = 15,
    TOK_PLUS_PLUS   = 16,

    TOK_IDENTIFIER  = 17,
    TOK_NUMBER      = 18,
    TOK_STRING      = 19,

    TOK_EOF         = 20
};

struct lex_state
{
    FILE* source;            /* Source file currently read.                  */
    char buffer[BUFFER_SIZE];/* Double buffered input from source.           */
    int index;               /* Location in the buffer; next char to be read.*/
    int chars_left;          /* How many chars are left to be read.          */
    int tok_start;           /* Where our current token string starts.       */
    int lineno;              /* Currnet line of the source file.             */
};

struct token
{
    enum tok_type type;
    const char* lexeme;
    int length;
    int lineno;
};

void lex_init(struct lex_state*);
void lex_feed(struct lex_state*, FILE* source);
struct token* lex_get_token(struct lex_state*);
void print_tok(struct token*);

#endif
