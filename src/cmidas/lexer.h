#ifndef MD_lexer_h_
#define MD_lexer_h_

#include <stdio.h>
#include <stdbool.h>

#define HALF_BUFFER_SIZE 50
#define BUFFER_SIZE (HALF_BUFFER_SIZE * 2)

enum tok_type
{
    TOK_BANG,
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
    TOK_INTEGER,
    TOK_DOUBLE,
    TOK_STRING,

    TOK_VAR,
    TOK_TRUE,
    TOK_FALSE,
    TOK_PRINT,

    TOK_EOF,

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
    const char *path;        /* Path to file being currently parsed.         */
    FILE *source;            /* Source file currently read.                  */
    char buffer[BUFFER_SIZE];/* Double buffered input from source.           */
    // XXX all of these could be unsigned short ints
    int index;               /* Location in the buffer; next char to be read.*/
    int chars_left;          /* How many chars are left to be read.          */
    int tok_start;           /* Where our current token string starts.       */
    int lineno;              /* Currnet line of the source file.             */
    int colno;               /* Current column of the source file            */
};

struct tok
{
    enum tok_type type;
    const char *lexeme;
    int length;
    int lineno;
    int colno;
};

void        lex_init   (struct lex_state *);
void        lex_feed   (struct lex_state *, const char *);
struct tok *lex_get_tok(struct lex_state *);
void        print_tok  (struct tok *);

#endif
