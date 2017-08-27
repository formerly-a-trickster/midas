#ifndef MD_hash_h_
#define MD_hash_h_

/*
 * An open adressing hash table. Collisions are solved with Robin Hood hashing.
 * Catastrophic linear pileup is averted by setting a threshold on the number
 * of linear probes an insertion operation can do before the table needs to be
 * resized.
 *
 * The table allocates a number of slack rows, that do not factor into its
 * declared size. For example, if a table has a size of 32 and a slack of 5,
 * the initial insertion operation could only target rows 0 to 31. But, if it
 * targets row 31 and finds it is already occupied, it can continue probing
 * rows 32 to 36. Coupled with the fact that the threshold on probing is
 * precisely the number of slack rows, this means we can skip bounds checking
 * when inserting.
 */

#define T Hash_T

typedef struct T *T;

extern    T  Hash_new (void);
extern void *Hash_set (T hash, const char *key, void *val);
extern void *Hash_get (T hash, const char *key);
extern void  Hash_free(T hash);

#undef T

#endif

