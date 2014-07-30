#include <cesk/cesk_alloctab.h>
/**
 * @brief node type of  allocation table
 **/
typedef struct _cesk_alloc_node_t{
	uint32_t token;  
	uint32_t key;
	uint32_t val;
	struct _cesk_alloc_node_t* next;
} cesk_alloc_node_t;
struct _cesk_alloctab_t{
	cesk_alloc_node_t* slot[CESK_ALLOC_TABLE_NSLOTS];
	uint32_t chain_size[CESK_ALLOC_TABLE_NSLOTS];
	uint32_t next_token;
	uint32_t max_chain_size;
	uint32_t using;
};
cesk_alloctab_t* cesk_alloctab_new(cesk_alloctab_t* old)
{
	if(NULL == old || old->max_chain_size > CESK_ALLOC_TABLE_MAX_CHAIN_SIZE)
	{
		cesk_alloctab_t* ret = (cesk_alloctab_t*)malloc(sizeof(cesk_alloctab_t));
		if(NULL == ret)
		{
			LOG_ERROR("can not allocate memory for allocation table");
			return NULL;
		}
		memset(ret, 0, sizeof(cesk_alloctab_t));
		ret->using = 1;
		return ret;
	}
	else
	{
		old->using ++;
		return old;
	}
}
void cesk_alloctab_free(cesk_alloctab_t* mem)
{
	if(NULL == mem) return;
	mem->using --;
	if(0 == mem->using)
	{
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
		free(mem);
	}
}
/**
 * @brief the hashcode for key <store, address> pair
 **/
static inline hashval_t _cesk_alloctab_hashcode(uint32_t token, uint32_t addr)
{
	return ((token) * MH_MULTIPLY + (addr * addr + addr * MH_MULTIPLY)) % CESK_ALLOC_TABLE_NSLOTS;
}
/**
 * @note We assume the function is always called properly that means, never try to insert a
 *         duplicated element in to the allocation table
 **/
int cesk_alloctab_insert(cesk_alloctab_t* table, const cesk_store_t* store, uint32_t key_addr,  uint32_t val_addr)
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
	hashval_t h = _cesk_alloctab_hashcode(store->alloc_token, key_addr);
	cesk_alloc_node_t* ptr = (cesk_alloc_node_t*)malloc(sizeof(cesk_alloc_node_t));
	ptr->key = key_addr;
	ptr->val = val_addr;
	ptr->token = store->alloc_token;
	ptr->next = table->slot[h];
	table->slot[h] = ptr;
	table->chain_size[h] ++;
	if(table->chain_size[h] > table->max_chain_size) 
		table->max_chain_size = table->chain_size[h];
	return 0;
}

uint32_t cesk_alloctab_query(const cesk_alloctab_t* table, const cesk_store_t* store, uint32_t addr)
{
	if(NULL == table) return CESK_STORE_ADDR_NULL;
	hashval_t h = _cesk_alloctab_hashcode(store->alloc_token, addr);
	cesk_alloc_node_t* ptr;
	for(ptr = table->slot[h]; NULL != ptr; ptr = ptr->next)
		if(ptr->token == store->alloc_token && ptr->key == addr)
			return ptr->val;
	return CESK_STORE_ADDR_NULL;
}
int cesk_alloctab_map_addr(cesk_alloctab_t* table, cesk_store_t* store, uint32_t rel_addr, uint32_t obj_addr)
{
	if(!CESK_STORE_ADDR_IS_RELOC(rel_addr)) 
	{
		LOG_ERROR("invalid relocated address @0x%x", rel_addr);
		return -1;
	}
	if(!CESK_STORE_ADDR_IS_OBJ(obj_addr))
	{
		LOG_ERROR("invalid object address @0x%x", obj_addr);
		return -1;
	}
	if(cesk_alloctab_insert(table, store, rel_addr, obj_addr) < 0)
	{
		LOG_ERROR("can not insert record mapping relocated address @0x%x to"
				  "object address @0x%x", 
				  rel_addr,
				  obj_addr);
		return -1;
	}
	if(cesk_alloctab_insert(table, store, obj_addr, rel_addr) < 0)
	{
		LOG_ERROR("can not insert record mapping object address @0x%x to"
				  "object address @0x%x",
				  obj_addr,
				  rel_addr);
		return -1;
	}
	return 0;
}
uint32_t cesk_alloctab_get_token(cesk_alloctab_t* table)
{
	return table->next_token ++;
}
