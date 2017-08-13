#include "utils.h"

#include <stdlib.h>

struct stmlist*
stmlist_new(void)
{
    struct stmnode* nil = malloc(sizeof(struct stmnode));
    nil->next = nil;
    nil->prev = nil;

    struct stmlist* list = malloc(sizeof(struct stmlist));
    list->nil = nil;
    list->length = 0;

    return list;
}

void
stmlist_prepend(struct stmlist* list, struct stm* data)
{
    struct stmnode* node = malloc(sizeof(struct stmnode));
    node->data = data;

    node->next = list->nil->next;
    list->nil->next->prev = node;
    list->nil->next = node;
    node->prev = list->nil;

    ++list->length;
}

void
stmlist_append(struct stmlist* list, struct stm* data)
{
    struct stmnode* node = malloc(sizeof(struct stmnode));
    node->data = data;

    node->prev = list->nil->prev;
    list->nil->prev->next = node;
    list->nil->prev = node;
    node->next = list->nil;

    ++list->length;
}

