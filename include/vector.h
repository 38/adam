#ifndef __VECTOR_H__
#define __VECTOR_H__
/* A simple vector, which can dynamic resize at run time */
#include <stdint.h>
#include <stdlib.h>
#include <constants.h>

typedef struct {
    size_t  capacity;       /* Current Capacity of the vector, used for resize */
    size_t  size;           /* Current used size of vector */
    size_t  elem_size;      /* Indicates the size of element */
    char*   data;           /* The data section */
} vector_t;

vector_t* vector_new(size_t elem_size);  /* Create a new vector */
void      vector_free(vector_t* vec);      /* free the vector after use */
int       vector_pushback(vector_t* vec, void* data);  /* push an element at the end of the vector */

/* Get the element idx in the vector */
static inline void* vector_get(vector_t* vec, int idx) 
{
#ifndef CHECK_EVERYTHING   /* For performance reason, we DO NOT check the validity of vector. */
    if(NULL == vec) return NULL; 
    if(idx >= vec->size) return NULL;
#endif
    return (void*)(vec->data + vec->elem_size * idx);
}
static inline size_t vector_size(vector_t* vec)
{
#ifdef CHECK_EVERYTHING
    if(NULL == vec) return 0;
#endif
    return vec->size;
}
#endif /* __VECTOR_H__*/
