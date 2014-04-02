#include <cesk/cesk_valuetab.h>
cesk_valuetab_t *cesk_valuetab_new()
{
	cesk_valuetab_t* ret = vector_new(sizeof(cesk_value_t*));
	if(NULL == ret)
	{
		LOG_ERROR("can not create a new vector as the value table");
		return NULL;
	}
	return ret;
}
void cesk_valuetab_free(cesk_valuetab_t* mem)
{
	if(NULL == mem) return;
	int i;
	for(i = 0; i < vector_size(mem); i ++)
	{
		cesk_value_t* val = vector_get(mem, i);
		cesk_value_decref(val);
	}
	vector_free(mem);
	return;
}
uint32_t cesk_valuetab_append(cesk_valuetab_t* table, cesk_value_t* value)
{
	if(NULL == table || NULL == value)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	uint32_t ret = vector_size(table);
	if(vector_pushback(table, &value) < 0)
	{
		LOG_ERROR("can not append the value to the end of the table");
		return -1;
	}
	return ret;
}
uint32_t cesk_valuetab_install_reloc(cesk_valuetab_t* table, cesk_store_t* store, uint32_t addr)
{
	// TODO
	return -1;
}
