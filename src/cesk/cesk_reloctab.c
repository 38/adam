#include <cesk/cesk_reloctab.h>
cesk_reloctab_t* cesk_reloctab_new()
{
	cesk_reloctab_t* ret = (cesk_reloctab_t*)malloc(sizeof(cesk_reloctab_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the relocation table");
		return NULL;
	}
	ret->_capacity  = CESK_RELOCTAB_INIT_SIZE;
	ret->_nxtidx    = 0;
	ret->table = (cesk_value_t**)malloc(ret->_capacity * sizeof(cesk_value_t*));
	return ret;
}
void cesk_reloctab_free(cesk_reloctab_t* table)
{
	if(NULL == table) return;
	int i;
	for(i = 0; i < table->_nxtidx; i ++)
		cesk_value_decref(table->table[i]);
	free(table);
}

int cesk_reloctab_insert(cesk_reloctab_t* table, cesk_value_t* value)
{
	if(NULL == table || NULL == value)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(table->_capacity == table->_nxtidx)
	{
		uint32_t new_size = table->_capacity * 2;
		LOG_DEBUG("the relocation table is full, resize the table from %u to %u", table->_capacity, new_size - 1);
		cesk_value_t** new_val = (cesk_value_t**)  malloc(sizeof(cesk_value_t*) * new_size);
		if(NULL == new_val)
		{
			LOG_ERROR("can not increase the capacity of the relocation table");
			return -1;
		}
		table->_capacity = new_size;
		table->table = new_val;
	}
	int ret = table->_nxtidx ++;
	table->table[ret] = value;
	cesk_value_incref(value);
	return ret;
}
