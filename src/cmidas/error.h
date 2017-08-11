#ifndef MD_error_h
#define MD_error_h

#include "lexer.h"

#define TXT_BOLD  "\x1b[1m"
#define TXT_RED  "\x1b[31m"
#define TXT_CLEAR "\x1b[0m"

void err_at_tok(const char*, struct tok*, const char*, ...);
void show_err_head(const char*);
void show_err_code(const char*, int, int, int);

#endif

