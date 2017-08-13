#ifndef MD_utils_h_
#define MD_utils_h_

#include "parser.h"

struct stmlist
{
    struct stmnode* nil;
    int length;
};

struct stmnode
{
    struct stmnode* next;
    struct stmnode* prev;
    struct stm* data;
};

struct stmlist* stmlist_new(void);
void stmlist_prepend(struct stmlist*, struct stm*);
void stmlist_append(struct stmlist*, struct stm*);

#endif

