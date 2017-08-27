#ifndef ALG_vector_h_
#define ALG_vector_h_

/*
 * Stores contents of the same size in a contiguous block of memory, like a
 * dynamic array. `Vector_push` takes a pointer to the value that you want to
 * copy into the array. `Vector_get` and `Vector_pop` return a pointer into the
 * vector. Dereference this to get the stored value.
 *
 * The pointer returned by `Vector_pop` is valid until a new value is added to
 * the vector. The same is true from poiners returned from `Vector_get` after
 * enough succesive `Vector_pop`s.
 */

#define T Vector_T

typedef struct T *T;

extern    T  Vector_new (int size);
extern  int  Vector_push(T vector, void *data);
extern void *Vector_pop (T vector);
extern void *Vector_get (T vector, int index);
extern  int  Vector_length(T vector);

#undef T

#endif

