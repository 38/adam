#include <cesk/cesk_diff.h>
/**
 * @brief the node of hash table used for diff reduction
 **/
typedef struct _cesk_diff_reduction_node_t {
	cesk_diff_item_t* current_item;
	struct _cesk_diff_merge_node_t* next;
} _cesk_diff_merge_node_t;
/**
 * @brief the hash table for diff reduction, key is the address value is the item
 **/
static _cesk_diff_reduction_node_t* _cesk_diff_reduction_table[CESK_DIFF_REDUCTION_TABLE_SIZE];
/**
 * @brief if the flag equals the current tick, that means the slot is in use 
 **/
static uint32_t _cesk_diff_reduction_flag[CESK_DIFF_REDUCTION_TABLE_SIZE];
/**
 * @brief the useable memory reserved for the table
 **/
static _cesk_diff_reduction_node_t* _cesk_diff_reduction_pool;
/**
 * @brief the allocation pointer, the next free element
 **/
static uint32_t _cesk_diff_reduction_alloc_ptr;
/**
 * @brief the capacity of the reduction node pool
 **/
static uint32_t _cesk_diff_reduction_pool_capacity;
/** 
 * @brief the clock tick
 **/
static uint32_t _cesk_diff_reduction_tick;

int cesk_diff_init()
{
	_cesk_diff_reduction_tick = 0;
	_cesk_diff_reduction_pool_capacity = CESK_DIFF_REDUCTION_POOL_SIZE;
	_cesk_diff_reduction_pool = (_cesk_diff_merge_node_t*)malloc(sizeof(_cesk_diff_merge_node_t*) * CESK_DIFF_REDUCTION_POOL_SIZE);
	if(NULL == _cesk_diff_reduction_pool) 
	{
		LOG_ERROR("can not allocate memory for reduction pool");
		return -1;
	}
	_cesk_diff_reduction_alloc_ptr = 0;
}
int cesk_diff_fianlize()
{
	if(NULL != _cesk_diff_reduction_pool) free(_cesk_diff_reduction_pool);
	return 0;
}
/**
 * @brief allocate new memory for the node 
 * @return the pointer to the new node, NULL if an error occurs
 **/
static inline _cesk_diff_reduction_node_t* _cesk_diff_reduction_alloc()
{
	if(_cesk_diff_reduction_alloc_ptr == _cesk_diff_reduction_pool_capacity)
	{
		uint32_t newsize = _cesk_diff_reduction_pool_capacity * 2;
		LOG_DEBUG("the reduction node pool is full, resize the pool from %u to %u",
				  _cesk_diff_reduction_pool_capacity,
				  newsize);
		_cesk_diff_reduction_node_t* newpool = (_cesk_diff_reduction_node_t*)malloc(newsize);
		if(NULL == newpool)
		{
			LOG_ERROR("can not increase the size of the reduction node pool");
			return NULL;
		}
	}
	return _cesk_diff_reduction_pool + (_cesk_diff_reduction_alloc_ptr ++);
}
/**
 * @brief clear the table prepare for the next merge
 **/
static inline void _cesk_diff_reduction_table_clear()
{
	_cesk_diff_reduction_tick ++;
	_cesk_diff_reduction_alloc_ptr = 0;
}
/**
 * @brief the hash function of the address
 * @param type the addressing type, fixed or relocated 
 * @param addr the address that we want to hash
 * @return the hashcode of this address
 **/
static inline hashval_t _cesk_diff_addr_hashcode(uint8_t type, const cesk_diff_addr_t* addr)
{
	uintptr_t val;
	if(CESK_DIFF_FIXED == type)
		val = (addr->value.id) * (addr->value.id);
	else
		val = (uintptr_t)addr->value.ptr;
	return (val * MH_MULTIPLY) % CESK_DIFF_REDUCTION_TABLE_SIZE;
}
/**
 * @brief insert a node to the table, if there's already one, merge them
 * @return the result of the operation, < 0 for error 
 **/
static inline int _cesk_diff_recution_table_insert(_cesk_diff_reduction_node_t* item)
{
	//TODO
}
cesk_diff_item_t* cesk_diff_item_new()
{
	cesk_diff_item_t* item = (cesk_diff_item_t*)malloc(sizeof(cesk_diff_item_t));
	if(NULL == item)
	{
		LOG_ERROR("can not allocate memory for the new diff item struct");
		return NULL;
	}
	memset(item, 0 ,sizeof(cesk_diff_item_t));
	return item;
}
void cesk_diff_item_free(cesk_diff_item_t* item)
{
	if(NULL == item) return;
	free(item);
}
cesk_diff_t* cesk_diff_new()
{
	cesk_diff_t* ret = (cesk_diff_t*)malloc(sizeof(cesk_diff_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocaate memory for the new diff struct");
		return NULL;
	}
	memset(ret, 0, sizeof(cesk_diff_t));
	return ret;
}
void cesk_diff_free(cesk_diff_t* diff)
{
	if(NULL == diff) return;
	cesk_diff_item_t* ptr;
	for(ptr = diff->head; NULL != ptr;)
	{
		cesk_diff_item_t* node = ptr;
		ptr = ptr->next;
		free(node);
	}
}
int cesk_diff_push_back(cesk_diff_t* diff, cesk_diff_item_t* item)
{
	if(NULL == diff || NULL == item)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	item->next = NULL;
	item->prev = diff->tail;
	diff->tail = item;
	if(NULL == diff->head) diff->head = item;
	return 0;
}
int cesk_diff_push_front(cesk_diff_t* diff, cesk_diff_item_t* item)
{
	if(NULL == diff || NULL == item)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	item->next = diff->head;
	item->prev = NULL;
	diff->head = item;
	if(NULL == diff->tail) diff->tail = item;
	return 0;
}


int cesk_diff_reduce(cesk_diff_t* diff)
{
	if(NULL == diff)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(1 == diff->reduced)
	{
		LOG_NOTICE("the diff is reduced");
		return 0;
	}

	//TODO use a hash table
	return 0;
}
