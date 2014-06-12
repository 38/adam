#include <cesk/cesk_reloc.h>
typedef struct _cesk_reloc_reverse_hash_node_t _cesk_reloc_reverse_hash_node_t;
/**
 * @brief node in global relocation hash table key: {table, inst, offset} --> value: {addr}
 **/
struct _cesk_reloc_reverse_hash_node_t{
	uint32_t inst;                      /*!< the instruction index */
	uint32_t offset;                    /*!< the field offset */
	const cesk_reloc_table_t* table;    /*!< the relocation table */
	uint32_t addr;                      /*!< relocated address */
	_cesk_reloc_reverse_hash_node_t* next; /*!< next node */
};
/**
 * @brief the hash table 
 **/
static _cesk_reloc_reverse_hash_node_t* _hash[CESK_RELOC_HASH_SIZE];

int cesk_reloc_init()
{
	memset(_hash, 0, sizeof(_hash));
	return 0;
}
void cesk_reloc_finalize()
{
	int i;
	for(i = 0; i < CESK_RELOC_HASH_SIZE; i ++)
	{
		_cesk_reloc_reverse_hash_node_t* ptr;
		for(ptr = _hash[i]; NULL != ptr; )
		{
			_cesk_reloc_reverse_hash_node_t* node = ptr;
			ptr = ptr->next;
			free(node);
		}
	}
}
/**
 * @brief the hash code for the hash table 
 * @param table the relocation table
 * @param inst the instruction index
 * @param offset the field offset
 * @return the hashcode for this input
 **/
static inline hashval_t _cesk_reloc_reverse_hash(const cesk_reloc_table_t* table, uint32_t inst, uint32_t offset)
{
	return ((((uintptr_t)table)%2147483647) * MH_MULTIPLY * MH_MULTIPLY) + inst * inst * MH_MULTIPLY + offset * MH_MULTIPLY * MH_MULTIPLY;
}
/**
 * @brief look for the table
 * @param table the relocation table
 * @param inst the instruction index
 * @return the node for this record, NULL means not found
 **/
static inline _cesk_reloc_reverse_hash_node_t* _cesk_reloc_reverse_hash_find(const cesk_reloc_table_t* table, uint32_t inst, uint32_t offset)
{
	hashval_t h = _cesk_reloc_reverse_hash(table, inst, offset) % CESK_RELOC_HASH_SIZE;
	_cesk_reloc_reverse_hash_node_t* ptr;
	for(ptr = _hash[h]; NULL != ptr; ptr = ptr->next)
		if(ptr->inst == inst && ptr->offset == offset && ptr->table == table) 
			return ptr;
	return NULL;
}
/**
 * @brief insert a record to the table
 * @param table the relocation table
 * @param inst the instruction index
 * @param offset the field offset
 * @param addr the relocated address assigned to this allocation
 * @return the newly created hash node, NULL indicates failure of insertion
 **/
static inline _cesk_reloc_reverse_hash_node_t* _cesk_reloc_reverse_hash_insert(const cesk_reloc_table_t* table, uint32_t inst, uint32_t offset, uint32_t addr)
{
	hashval_t h = _cesk_reloc_reverse_hash(table, inst, offset) % CESK_RELOC_HASH_SIZE;
	_cesk_reloc_reverse_hash_node_t* ptr = (_cesk_reloc_reverse_hash_node_t*)malloc(sizeof(_cesk_reloc_reverse_hash_node_t));
	if(NULL == ptr) return NULL;
	ptr->inst = inst;
	ptr->offset = offset;
	ptr->table = table;
	ptr->addr = addr;
	ptr->next = _hash[h];
	_hash[h] = ptr;
	return ptr;
}
/**
 * @biref delete a node from the hash table
 * @param table the relocation table
 * @param inst the instruction index
 * @param offset the field offset
 * @return nothing
 **/
static inline void _cesk_reloc_reverse_hash_delete(const cesk_reloc_table_t* table, uint32_t inst, uint32_t offset)
{
	hashval_t h = _cesk_reloc_reverse_hash(table, inst, offset) % CESK_RELOC_HASH_SIZE;
	_cesk_reloc_reverse_hash_node_t *prev,*ptr,*next;
	prev = NULL;
	for(ptr = _hash[h]; NULL != ptr; ptr = ptr->next)
	{
		if(ptr->inst == inst && ptr->offset == offset && ptr->table == table)
			break;
		prev = ptr;
	}
	if(NULL != ptr)
	{
		next = ptr->next;
		if(NULL != prev) prev->next = next;
		else _hash[h] = next;
		free(ptr);
	}
}
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
	size_t sz = vector_size(mem);
	for(i = 0; i < sz; i ++)
	{
		cesk_reloc_item_t* item = (cesk_reloc_item_t*)vector_get(mem, i);
		if(NULL != item)
			_cesk_reloc_reverse_hash_delete(mem, dalvik_instruction_get_index(item->instruction), item->field_offset);
	}
	vector_free(mem);
	return;
}
uint32_t cesk_reloc_table_append(
		cesk_reloc_table_t* table, 
		const dalvik_instruction_t* inst,
		uint32_t field_offset)
{
	if(NULL == table)
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}
	cesk_reloc_item_t buf = {
		.instruction = inst,
		.field_offset = field_offset,
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
		uint32_t field,
		int dry_run)
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
	uint32_t obj_addr = cesk_store_allocate(store, inst, field);
	if(CESK_STORE_ADDR_NULL == obj_addr)
	{
		LOG_ERROR("can not allocate an object address for this value, aborting");
		return CESK_STORE_ADDR_NULL;
	}
	uint32_t rel_addr;
	_cesk_reloc_reverse_hash_node_t* node = _cesk_reloc_reverse_hash_find(value_tab, dalvik_instruction_get_index(inst), field);
	if(NULL != node)
	{
		/* try to find this address in the allocation table, if it's already there means this
		 * address has been allocated previously, in other words, it's a resued address */
		rel_addr = cesk_alloctab_query(store->alloc_tab, store, obj_addr);
		if(rel_addr != CESK_STORE_ADDR_NULL)
		{
			/* found an relocated address in allocation record? */
			LOG_DEBUG("found reused relocation address @0x%x", rel_addr);
			return rel_addr;
		}
		else
			rel_addr = node->addr;
	}
	else
	{
		/* otherwise, this address is new to allocation table, so create a new relocation address for it */
		rel_addr = cesk_reloc_table_append(value_tab, inst, field);
		if(CESK_STORE_ADDR_NULL == rel_addr)
		{
			LOG_ERROR("can not create an new relocated address for this object");
			return CESK_STORE_ADDR_NULL;
		}
		if(_cesk_reloc_reverse_hash_insert(value_tab, dalvik_instruction_get_index(inst), field, rel_addr) == NULL)
		{
			LOG_ERROR("can not insert new record to hash table");
			return CESK_STORE_ADDR_NULL;
		}
	}
	/* finally, map the relocated address to the object address in this store */
	if(0 == dry_run && cesk_alloctab_map_addr(store->alloc_tab, store, rel_addr, obj_addr) < 0)
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
	uint32_t obj_addr = cesk_store_allocate(store, item->instruction, item->field_offset);
	if(CESK_STORE_ADDR_NULL == obj_addr)
	{
		LOG_ERROR("can not allocate OA for the relocated object #%d", value_idx);
		return CESK_STORE_ADDR_NULL;
	}
	/* check if there's a object already there */
	if(cesk_alloctab_query(store->alloc_tab, store, obj_addr) != CESK_STORE_ADDR_NULL)
	{
		LOG_ERROR("can not place value in store, because there's already a value there");
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
