#ifndef _LOSSYCOUNTMINSKETCH_H
#define _LOSSYCOUNTMINSKETCH_H
#include "xis.h"

/* struct defining one row of our sketch */
typedef struct LossySketch
{
  unsigned _hash_func_index; /* which hash function */
  unsigned _b; /* number of counters or buckets for a hash function */
  uint32_t* identity; /* pointer to array of '_b' words */
  /* we can do this with because we are assigning fixed length of
   * memories for each word. It is efficient and easy to handle.
   * In our case, we assign 16Bytes, i.e. we limit our word 
   * length to 16 characters.
   */
  int* lossyCount; /* 1-D array of '_b' counts */
  /* Cache line size assumed to be 64Byte.
   * Each hash function has 4 * 4 Bytes of elements
   * in this data structure. Assuming 4 hash functions 
   * in each thread, there is no false sharing */
} LossySketch;


/* This function updates the local sketch based on input word */ 
void update_sketch( LossySketch* _sketch,
                    uint32_t element,
                    Xi ** xi_bucket,int index );


/* This function merges thread local sketches to 
 * create the final sketch for a node
 */
void local_merge_sketch(LossySketch* final, 
                        LossySketch*   LCMS,
                         const unsigned num_local_copies,
                         const unsigned num_hash_func,
                         const unsigned hash_func_index );

#endif /* _LOSSYCOUNTMINSKETCH_H */