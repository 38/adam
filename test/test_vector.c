#include <vector.h>
#include <assert.h>

int main()
{
    vector_t *vec = vector_new(sizeof(int));
    assert(NULL != vec);
    int i;
    for(i = 0; i < 10000; i ++)
        vector_pushback(vec, &i);
    for(i = 0; i < 10000; i ++)
        assert(i == *(int*)vector_get(vec,i));
    return 0;
}
