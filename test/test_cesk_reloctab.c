#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	cesk_reloctab_t* table = cesk_reloctab_new();
	assert(NULL != table);
	//insert 1000000 empty sets 
	int i = 0;
	for(i = 0; i < 1000000; i ++)
	{
		cesk_value_t* value = cesk_value_empty_set();
		assert(NULL != value);
		assert(i == cesk_reloctab_insert(table, value));
	}
	for(i = 0; i < 1000000; i ++)
	{
		cesk_value_t* value = cesk_reloctab_get(table, i);
		assert(NULL != value);
		assert(CESK_TYPE_SET == value->type);
		assert(0 == cesk_set_size(value->pointer.set));
	}
	cesk_reloctab_free(table);
	adam_finalize();
	return 0;
}
