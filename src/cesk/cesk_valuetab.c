#include <cesk/cesk_valuetab.h>
cesk_valuetab_t *cesk_valuetab_new()
{
	cesk_valuetab_t* ret = vector_new(sizeof(cesk_valuetab_item_t));
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
uint32_t cesk_valuetab_append(
		cesk_valuetab_t* table, 
		cesk_value_t* value, const dalvik_instruction_t* inst,
		uint32_t parent_addr,
		uint32_t field_offset)
{
	if(NULL == table || NULL == value)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	cesk_valuetab_item_t buf = {
		.value = value,
		.instruction = inst,
		.parent_addr = parent_addr,
		.field_offset = field_offset
	}; 
	uint32_t ret = vector_size(table);
	if(vector_pushback(table, &buf) < 0)
	{
		LOG_ERROR("can not append the value to the end of the table");
		return -1;
	}
	return ret;
}
uint32_t cesk_valuetab_install_reloc(cesk_valuetab_t* table, cesk_store_t* store, uint32_t addr)
{
	if(NULL == table || NULL == store || !CESK_STORE_ADDR_IS_RELOC(addr))
	{
		LOG_ERROR("invalid arguments");
		return CESK_STORE_ADDR_NULL;
	}
	/* get value using address */
	uint32_t value_idx = CESK_STORE_ADDR_RELOC_IDX(addr);
	if(value_idx >= vector_size(table))
	{
		LOG_ERROR("there's no entry #%d in the relocation table", value_idx);
		return CESK_STORE_ADDR_NULL;
	}
	cesk_valuetab_item_t* item = vector_get(table, value_idx);
	if(NULL == item)
	{
		LOG_ERROR("can not aquire the pointer to value table item #%d", value_idx);
		return CESK_STORE_ADDR_NULL;
	}
	/* try to allocate a OA for the value */
	uint32_t obj_addr = cesk_store_allocate_oa(store, item->instruction, item->parent_addr, item->field_offset);
	if(CESK_STORE_ADDR_NULL == obj_addr)
	{
		LOG_ERROR("can not allocate OA for the relocated object #%d", value_idx);
		return CESK_STORE_ADDR_NULL;
	}
	/* check if there's a object in the object */
	if(cesk_alloctab_query(store->alloc_tab, store, obj_addr) != CESK_STORE_ADDR_NULL)
	{
		LOG_ERROR("can not place value in store");
		return CESK_STORE_ADDR_NULL;
	}
	if(cesk_alloctab_insert(store->alloc_tab, store, addr, obj_addr) < 0)
	{
		LOG_ERROR("can not map relocated address @0x%x to object address @0x%x in allocation table", addr, obj_addr);
		return CESK_STORE_ADDR_NULL;
	}
	if(cesk_alloctab_insert(store->alloc_tab, store, obj_addr, addr) < 0)
	{
		LOG_ERROR("can not map object address 0x%x to relocated address @0x%x in allocation table", obj_addr, addr);
		return CESK_STORE_ADDR_NULL;
	}
	if(cesk_store_attach_oa(store, obj_addr, item->value) < 0)
	{
		LOG_ERROR("can not assign value to OA @0x%x", obj_addr);
		return CESK_STORE_ADDR_NULL;
	}
	return obj_addr;
}
