#include <vector.h>
#include <stdlib.h>
#include <string.h>
vector_t* vector_new(size_t elem_size)
{
    vector_t* ret = NULL;
    if(0 == elem_size) goto ERR;
    ret = (vector_t*)malloc(sizeof(vector_t));
    if(NULL == ret) goto ERR;
    ret->capacity = VECTOR_INIT_CAP;
    ret->size = 0;
    ret->elem_size = elem_size;
    ret->data = (char*)malloc(elem_size * ret->capacity);
    if(NULL == ret->data) goto ERR;
    return ret;
ERR:
    if(NULL != ret)
    {
        if(NULL != ret->data) free(ret->data);
        free(ret);
    }
    return NULL;
}
void vector_free(vector_t* vec)
{
    if(NULL != vec)
    {
        if(NULL != vec->data) free(vec->data);
        free(vec);
    }
}
static inline int _vector_resize(vector_t* vec)  /* double the capacity of a vector */
{
    if(NULL == vec) return -1;
    if(NULL == vec->data) return -1;
    size_t new_capacity = vec->capacity * 2;
    void* data = realloc(vec->data, new_capacity * vec->elem_size);
    if(NULL == data) return -1;
    vec->data = data;
    vec->capacity = new_capacity;
    return 0;
}
int vector_pushback(vector_t* vec, void* data)
{
    if(NULL == vec) return -1;
    if(NULL == data) return -1;
    if(vec->size == vec->capacity)
    {
        int rc = _vector_resize(vec);
        if(rc < 0) return rc;
    }
    void* addr = vec->data + vec->size * vec->elem_size;
    memcpy(addr, data, vec->elem_size);
    return 0;
}
