#ifndef MD_lexer_h
#define MD_lexer_h
#include <stdio.h>
#include <stdbool.h>

#define HALF_BUFFER_SIZE 20
#define BUFFER_SIZE (HALF_BUFFER_SIZE * 2)

enum tok_type
{
    TOK_COMMA,
    TOK_EQUAL,
    TOK_GREAT,
    TOK_LESS,
    TOK_MINUS,
    TOK_PAREN_LEFT,
    TOK_PAREN_RIGHT,
    TOK_PERCENT,
    TOK_PLUS,
    TOK_SEMICOLON,
    TOK_SLASH,
    TOK_STAR,

    TOK_GREAT_EQUAL,
    TOK_LESS_EQUAL,
    TOK_BANG_EQUAL,
    TOK_EQUAL_EQUAL,
    TOK_PLUS_PLUS,

    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,

    TOK_EOF
};

struct lex_state
{
    FILE* source;             // Source file currently read.
    char buffer[BUFFER_SIZE]; // Double buffered input from source.
    int index;                // Location in the buffer; next char to be read.
    int chars_left;           // How many chars are left to be read.
    int tok_start;            // Where our current token string starts/
    int lineno;               // Currnet line of the source file.
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

#endif
