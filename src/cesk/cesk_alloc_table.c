#include <cesk/cesk_alloc_table.h>
/**
 * @brief node type of  allocation table
 **/
typedef struct _cesk_alloc_node_t{
	cesk_store_t* store;
	uint32_t key;
	uint32_t val;
	struct _cesk_alloc_node_t* next;
} cesk_alloc_node_t;
struct _cesk_alloc_table_t{
	cesk_alloc_node_t* slot[CESK_ALLOC_TABLE_NSLOTS];
};
cesk_alloc_table_t* cesk_alloc_table_new()
{
	cesk_alloc_table_t* ret = (cesk_alloc_table_t*)malloc(sizeof(cesk_alloc_table_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for allocation table");
		return NULL;
	}
	memset(ret, 0, sizeof(cesk_alloc_table_t));
	return ret;
}
void cesk_alloc_table_free(cesk_alloc_table_t* mem)
{
	if(NULL == mem) return;
	int i;
	for(i = 0; i < CESK_ALLOC_TABLE_NSLOTS; i ++)
	{
		if(mem->slot[i])
		{
			cesk_alloc_node_t* ptr;
			for(ptr = mem->slot[i]; NULL != ptr;)
			{
				cesk_alloc_node_t* node = ptr;
				ptr = ptr->next;
				free(node);
			}
		}
	}
}
/**
 * @brief the hashcode for key <store, address> pair
 **/
static inline hashval_t _cesk_alloc_table_hashcode(cesk_store_t* store, uint32_t addr)
{
	return (((uintptr_t)store) * MH_MULTIPLY + (addr * addr + addr * MH_MULTIPLY)) % CESK_ALLOC_TABLE_NSLOTS;
}
/**
 * @notice We assume the function is always called properly that means, never try to insert a
 *         duplicated element in to the allocation table
 **/
int cesk_alloc_table_insert(
		cesk_alloc_table_t* table,
		cesk_store_t* store, 
		uint32_t key_addr, 
		uint32_t val_addr)
{
	if(NULL == table || NULL == store)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if((CESK_STORE_ADDR_IS_OBJ(key_addr) && CESK_STORE_ADDR_IS_RELOC(val_addr)))
	{
		LOG_DEBUG("inserting a record mapping object address @0x%x to relocated address @0x%x", 
				   key_addr,
				   val_addr);
	}
	else if(CESK_STORE_ADDR_IS_RELOC(key_addr) && CESK_STORE_ADDR_IS_OBJ(val_addr))
	{
		LOG_DEBUG("inserting a record mapping relocated address @0x%x to object address @0x%x",
				  key_addr,
				  val_addr);
	}
	else 
	{
		LOG_ERROR("invalid allocation record");
		return -1;
	}
	/* we do not check duplication */
	hashval_t h = _cesk_alloc_table_hashcode(store, key_addr);
	cesk_alloc_node_t* ptr = (cesk_alloc_node_t*)malloc(sizeof(cesk_alloc_node_t));
	ptr->key = key_addr;
	ptr->val = val_addr;
	ptr->store = store;
	ptr->next = table->slot[h];
	table->slot[h] = ptr;
	return 0;
}

uint32_t cesk_alloc_table_query(
		cesk_alloc_table_t* table,
		cesk_store_t* store,
		uint32_t addr)
{
	hashval_t h = _cesk_alloc_table_hashcode(store, addr);
	cesk_alloc_node_t* ptr;
	for(ptr = table->slot[h]; NULL != ptr; ptr = ptr->next)
		if(ptr->store == store && ptr->key == addr)
			return ptr->val;
	return CESK_STORE_ADDR_NULL;
}
