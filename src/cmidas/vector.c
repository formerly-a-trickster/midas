#include <string.h>
#include <stdlib.h>
#include "vector.h"

#define T Vector_T

struct T
{
    int capacity;
    int size;
    int next;
    char *data;
};

static void Vector_grow(T vector);

T
Vector_new(int size)
{
    T vector;

    vector = malloc(sizeof(struct Vector_T));
    vector->capacity = 2;
    vector->size = size;
    vector->next = 0;
    vector->data = calloc(2, size);

    return vector;
}

int
Vector_push(T vector, void *data)
{
    if (vector->next >= vector->capacity)
        Vector_grow(vector);

    memcpy(vector->data + vector->next * vector->size, data, vector->size);
    ++vector->next;

    return vector->next - 1;
}

void *
Vector_pop(T vector)
{
    --vector->next;
    return vector->data + vector->next * vector->size;
}

void *
Vector_get(T vector, int index)
{
    return vector->data + index * vector->size;
}

int
Vector_length(T vector)
{
    return vector->next;
}

static void
Vector_grow(T vector)
{
    char *data;

    data = realloc(vector->data, vector->size * vector->capacity * 2);
    vector->data = data;
    vector->capacity *= 2;
}

#undef T

