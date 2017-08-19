#ifndef ALG_vector_h_
#define ALG_vector_h_

#define T Vector_T

typedef struct T *T;

extern    T  Vector_new (int size);
extern  int  Vector_push(T vector, void *data);
extern void *Vector_pop (T vector);
extern void *Vector_get (T vector, int index);
extern  int  Vector_length(T vector);

#undef T

#endif

