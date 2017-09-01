#ifndef MD_LEXER
#define MD_LEXER

#define T Lexer_T

typedef struct T *T;

enum tok_t
{
    TOK_EOF,

    TOK_BANG, TOK_COMMA, TOK_EQUAL, TOK_GREAT, TOK_LESS, TOK_MINUS,
    TOK_PAREN_LEFT, TOK_PAREN_RIGHT, TOK_PERCENT, TOK_PLUS, TOK_SEMICOLON,
    TOK_SLASH, TOK_STAR,

    TOK_BANG_EQUAL, TOK_EQUAL_EQUAL, TOK_GREAT_EQUAL, TOK_LESS_EQUAL,
    TOK_PLUS_PLUS, TOK_SLASH_SLASH,

    TOK_DOUBLE, TOK_FALSE, TOK_INTEGER, TOK_NIL, TOK_STRING, TOK_TRUE,

    TOK_AND, TOK_BREAK, TOK_DO, TOK_ELSE, TOK_END, TOK_FOR, TOK_FUN,
    TOK_IDENTIFIER, TOK_IF, TOK_OR, TOK_PRINT, TOK_RETURN, TOK_VAR, TOK_WHILE
};

struct tok
{
    const char *lexeme;
    enum tok_t  type;
           int  length;
           int  lineno;
           int  colno;
};

         T  Lex_new    (void);
      void  Lex_feed   (T lex, const char *buffer);
struct tok *Lex_get_tok(T lex);
      void  Lex_get_err(T lex);

      void  print_tok(struct tok *);

#undef T

#endif /* MD_LEXER */

