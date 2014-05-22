#include <cesk/cesk_reloc.h>
cesk_reloc_table_t *cesk_reloc_table_new()
{
	cesk_reloc_table_t* ret = vector_new(sizeof(cesk_reloc_item_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not create a new vector as the value table");
		return NULL;
	}
	return ret;
}
void cesk_reloc_table_free(cesk_reloc_table_t* mem)
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
uint32_t cesk_reloc_table_append(
		cesk_reloc_table_t* table, 
		const dalvik_instruction_t* inst,
		uint32_t parent_addr,
		uint32_t field_offset)
{
	if(NULL == table)
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}
	cesk_reloc_item_t buf = {
		.instruction = inst,
		.parent_addr = parent_addr,
		.field_offset = field_offset
	}; 
	uint32_t ret = vector_size(table);
	if(0 != (ret & CESK_STORE_ADDR_RELOC_PREFIX))
	{
		LOG_ERROR("to many relocated objects, try to adjust "
				  "CESK_STORE_ADDR_RELOC_PREFIX(currently 0x%lx)", 
				  CESK_STORE_ADDR_RELOC_PREFIX);
		return CESK_STORE_ADDR_NULL;
	}
	if(vector_pushback(table, &buf) < 0)
	{
		LOG_ERROR("can not append the value to the end of the table");
		return CESK_STORE_ADDR_NULL;
	}
	ret = CESK_STORE_ADDR_RELOC_PREFIX | ret;
	if(CESK_STORE_ADDR_IS_RELOC(ret)) 
		return ret;
	else
	{
		LOG_ERROR("to many relocated objects, try to adjust "
				  "CESK_STORE_ADDR_RELOC_PREFIX(currently 0x%lx)", 
				  CESK_STORE_ADDR_RELOC_PREFIX);
		return CESK_STORE_ADDR_NULL;
	}
}
uint32_t cesk_reloc_allocate(
		cesk_reloc_table_t* value_tab, 
		cesk_store_t* store, 
		const dalvik_instruction_t* inst,
		uint32_t parent,
		uint32_t field)
{
	if(NULL == store || NULL == value_tab)
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}
	if(NULL == store->alloc_tab)
	{
		LOG_ERROR("can not allocate relocation address"
				  "because no allocation table is attached"
				  "to this store");
		return CESK_STORE_ADDR_NULL;
	}
	/* first try to allocate an object address for this */
	uint32_t obj_addr = cesk_store_allocate(store, inst, parent, field);
	if(CESK_STORE_ADDR_NULL == obj_addr)
	{
		LOG_ERROR("can not allocate an object address for this value, aborting");
		return CESK_STORE_ADDR_NULL;
	}
	/* try to find this address in the allocation table, if it's already there means this
	 * address has been allocated previously, in other words, it's a resued address */
	uint32_t rel_addr = cesk_alloctab_query(store->alloc_tab, store, obj_addr);
	if(rel_addr != CESK_STORE_ADDR_NULL)
	{
		/* found an relocated address in allocation record? */
		LOG_DEBUG("found reused relocation address @0x%x", rel_addr);
		return rel_addr;
	}
	/* otherwise, this address is new to allocation table, so create a new relocation address for it */
	rel_addr = cesk_reloc_table_append(value_tab, inst, parent, field);
	if(CESK_STORE_ADDR_NULL == rel_addr)
	{
		LOG_ERROR("can not create an new relocated address for this object");
		return CESK_STORE_ADDR_NULL;
	}
	/* finally, map the relocated address to the object address in this store */
	if(cesk_alloctab_map_addr(store->alloc_tab, store, rel_addr, obj_addr) < 0)
	{
		LOG_ERROR("can not map address @0x%x --> @0x%x", rel_addr, obj_addr);
		return CESK_STORE_ADDR_NULL;
	}
	LOG_DEBUG("relocated address is initialized, address map: @0x%x --> @0x%x", rel_addr, obj_addr);
	/* here we done */
	return rel_addr;
}
uint32_t cesk_reloc_addr_init(const cesk_reloc_table_t* table, cesk_store_t* store, uint32_t addr, cesk_value_t* init_val)
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
	cesk_reloc_item_t* item = vector_get(table, value_idx);
	if(NULL == item)
	{
		LOG_ERROR("can not aquire the pointer to value table item #%d", value_idx);
		return CESK_STORE_ADDR_NULL;
	}
	if(NULL == init_val)
	{
		LOG_ERROR("can not place an value with uninitialized value to store (item #%d)", value_idx);
		return CESK_STORE_ADDR_NULL;
	}
	/* try to allocate a OA for the value */
	uint32_t obj_addr = cesk_store_allocate(store, item->instruction, item->parent_addr, item->field_offset);
	if(CESK_STORE_ADDR_NULL == obj_addr)
	{
		LOG_ERROR("can not allocate OA for the relocated object #%d", value_idx);
		return CESK_STORE_ADDR_NULL;
	}
	/* check if there's a object already there */
	if(cesk_alloctab_query(store->alloc_tab, store, obj_addr) != CESK_STORE_ADDR_NULL)
	{
		LOG_ERROR("can not place value in store");
		return CESK_STORE_ADDR_NULL;
	}
	/* if it's new, insert it to allocation table */
	if(cesk_alloctab_map_addr(store->alloc_tab, store, addr, obj_addr) < 0)
	{
		LOG_ERROR("can not map address @0x%x --> @0x%x", addr, obj_addr);
		return CESK_STORE_ADDR_NULL;
	}
	if(cesk_store_attach(store, obj_addr, init_val) < 0)
	{
		LOG_ERROR("can not assign value to OA @0x%x", obj_addr);
		return CESK_STORE_ADDR_NULL;
	}
	/* do not forget release the writable pointer */
	cesk_store_release_rw(store, obj_addr);
	LOG_DEBUG("relocated address is initialized, address map: @0x%x --> @0x%x", addr, obj_addr);
	return obj_addr;
}
