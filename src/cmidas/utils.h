#ifndef MD_utils_h
#define MD_utils_h

#include "parser.h"

struct stmlist
/* A doublylinked circular list, containing a sentinel node that marks both the
   start and the end of the list. It is circular for the sole reason that this
   way we only need one sentinel instead of one.

   It explicitly contains only statements. It is used to describe both a
   program and a code block, as both are a list of statements.

                       #stmnode                 #stmnode
              +---+    +----+-----+---+         +----+-----+---+
   stmlist--->|nil|<-->|prev|data|next|<- ... ->|prev|data|next|<-+
              +---+    +----+----+----+         +----+----+----+  |
                ^                                                 |
                +-------------------------------------------------+          */
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

