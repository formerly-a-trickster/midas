#include "list.h"

#include <stdlib.h>

struct list*
list_new(void)
{
    struct node* nil = malloc(sizeof(struct node));
    nil->next = nil;
    nil->prev = nil;

    struct list* list = malloc(sizeof(struct list));
    list->nil = nil;
    list->length = 0;

    return list;
}

void
list_prepend(struct list* list, void* data)
{
    struct node* node = malloc(sizeof(struct node));
    node->data = data;

    node->next = list->nil->next;
    list->nil->next->prev = node;
    list->nil->next = node;
    node->prev = list->nil;

    ++list->length;
}

void
list_append(struct list* list, void* data)
{
    struct node* node = malloc(sizeof(struct node));
    node->data = data;

    node->prev = list->nil->prev;
    list->nil->prev->next = node;
    list->nil->prev = node;
    node->next = list->nil;

    ++list->length;
}

