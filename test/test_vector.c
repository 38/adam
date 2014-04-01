#include <vector.h>
#include <assert.h>
#include <adam.h>
int main()
{
	adam_init();
	vector_t *vec = vector_new(sizeof(int));
	assert(NULL != vec);
	int i;
	for(i = 0; i < 10000; i ++)
		vector_pushback(vec, &i);
	for(i = 0; i < 10000; i ++)
		assert(i == *(int*)vector_get(vec,i));
	adam_finalize();
	return 0;
}
