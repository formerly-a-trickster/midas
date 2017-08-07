#ifndef MD_lexer_h
#define MD_lexer_h
#include <stdio.h>
#include <stdbool.h>

#define HALF_BUFFER_SIZE 64
#define BUFFER_SIZE (HALF_BUFFER_SIZE * 2)

struct lex_state
{
    FILE* source;             // Source file currently read.
    char buffer[BUFFER_SIZE]; // Double buffered input from source.
    int current;              // Current cell of the buffer.
    int limit;                // The index beyond which we can't roll back.
    int lexeme;               // Start of the currently read lexeme.
    int lineno;               // Currnet line of the source file.
};

enum tok_type
{
    TOK_LEFT_PAREN,  // 0
    TOK_RIGHT_PAREN, // 1
    TOK_PLUS,        // 2
    TOK_MINUS,       // 3
    TOK_STAR,        // 4
    TOK_SLASH,       // 5
    TOK_PERCENT,     // 6

    TOK_NUMBER,      // 7

    TOK_EOF          // 8
};

struct token
{
    enum tok_type type;
    char* lexeme;
    int length;
    int lineno;
};

void lex_init(struct lex_state*);
void lex_feed(struct lex_state*, FILE* source);
struct token* lex_get_token(struct lex_state*);

#endif
