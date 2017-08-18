#ifndef MD_list_h_
#define MD_list_h_

struct list
{
    struct node* nil;
    int length;
};

struct node
{
    struct node* next;
    struct node* prev;
    void* data;
};

struct list* list_new(void);
void list_prepend(struct list*, void*);
void list_append(struct list*, void*);

#endif

