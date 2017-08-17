#ifndef MD_hash_h_
#define MD_hash_h_

#include "interpreter.h"

struct hash
/* An open adressing hash table. Collisions are solved with Robin Hood hashing.
   Catastrophic linear pileup is averted by setting a threshold on the number
   of linear probes an insertion operation can do before the table needs to be
   resized.

   The table allocates a number of `slack` rows, that do not factor into its
   decalred size. Meaning that is a table has a size of 32 and a slack of 5,
   the initial insertion operation could only target rows 0 to 31. But, for
   example, if it targets row 31 and finds it already occupied, it can continue
   probing rows 32 to 36. Coupled with the fact that the threshold on probing
   is precisely the number of slack rows means we can skip bounds checking when
   inserting.                                                                */
{
    struct entry** table;
    int size;
    int slack;
};

struct entry
{
    const char* key;
    struct val* val;
    int dist;
};

struct hash* hash_new(void);
void hash_insert(struct hash*, const char*, struct val*);
struct entry* hash_search(struct hash*, const char*);
void hash_print(struct hash*);

#endif

