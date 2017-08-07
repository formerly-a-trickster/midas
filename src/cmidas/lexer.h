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

struct lex_state
{
    FILE* source;             // Source file currently read.
    char buffer[BUFFER_SIZE];   // Double buffered input from source.
    int current;              // Current cell of the buffer.
    int limit;                // The index beyond which we can't roll back.
    int lexeme;               // Start of the currently read lexeme.
    int lineno;               // Currnet line of the source file.
};

struct lex_state* lex_new(void);
void lex_from_file(struct lex_state*, const char*);
void lex_buffer_chars(struct lex_state*);

char lex_next_char(struct lex_state*);
void lex_roll_char(struct lex_state*);
bool lex_is_at_end(struct lex_state*);

#endif
