/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 **/
#include <stdio.h>

#include <cesk/cesk_diff.h>
#include <vector.h>
/**
 * @brief data structure used as the node in the hash table for diff pacakge reduction
 * @detail what the diff reduction actually does is merge all operations in the same address
 *        into one adddress, so we use a hash table to build the map from address to the 
 *        reduced operation.
 **/
typedef struct _cesk_diff_hash_node_t {
	uint32_t                token;                   /*!< the allocation token */
	cesk_diff_frame_addr_t  address;                 /*!< the affected address */
	cesk_diff_item_t*       reduced;                 /*!< the reduced diff item in this address */
	struct _cesk_diff_hash_node_t* hash_next;        /*!< the next element in the slot chain */
	struct _cesk_diff_hash_node_t* list_next;        /*!< the next element in the node list */
} _cesk_diff_hash_node_t;

/** @brief the reduction hash table **/
static _cesk_diff_hash_node_t* _cesk_diff_hash[CESK_DIFF_HASH_SIZE];
/** @brief allocation pointer **/
static uint32_t _cesk_diff_alloc_ptr;
/** @brief allocation token **/
static uint32_t _cesk_diff_alloc_token;
/** @brief memory pool **/
static vector_t* _cesk_diff_mem_pool;
/** @brief head of the node list */
static _cesk_diff_hash_node_t* _cesk_diff_node_list;

int cesk_diff_init()
{
	_cesk_diff_alloc_ptr = 0;
	_cesk_diff_alloc_token = 0;
	_cesk_diff_mem_pool = vector_new(sizeof(_cesk_diff_hash_node_t));
	if(NULL == _cesk_diff_mem_pool)
	{
		LOG_ERROR("can not intialize memory allocator for diff package");
		return -1;
	}
	memset(_cesk_diff_hash, 0, sizeof(_cesk_diff_hash));
	return 0;
}
void cesk_diff_finalize()
{
	vector_free(_cesk_diff_mem_pool);
}
//TODO hash table functions
/**
 * @brief clear the hash table 
 * @return nothing
 **/
static void _cesk_diff_hash_clear()
{
	_cesk_diff_alloc_ptr = 0;
	_cesk_diff_alloc_token ++;
	_cesk_diff_node_list = NULL;
}
/**
 * @brief allocate a hash table node
 * @return pointer to the new node
 **/
static inline _cesk_diff_hash_node_t* _cesk_diff_hash_node_alloc()
{
	if(vector_size(_cesk_diff_mem_pool) < _cesk_diff_alloc_ptr)
	{
		/* if there's no used memory for the new node, just try to allocate a new one */
		if(vector_dry_pushback(_cesk_diff_mem_pool) < 0)
		{
			LOG_ERROR("can not allocate a new node");
			return NULL;
		}
	}
	_cesk_diff_hash_node_t* ret = (_cesk_diff_hash_node_t*)vector_get(_cesk_diff_mem_pool, _cesk_diff_alloc_ptr++);
	ret->token = _cesk_diff_alloc_token;
	ret->reduced = NULL;
	ret->hash_next = NULL;
	ret->list_next = _cesk_diff_node_list;
	_cesk_diff_node_list = ret;
	return ret;
}
/**
 * @brief the hash code for frame address
 * @param addr the frame address
 * @return the hashcode
 **/
static inline hashval_t _cesk_diff_hashcode(cesk_diff_frame_addr_t addr)
{
	if(CESK_DIFF_FRAME_STORE == addr.type)
		return addr.value * MH_MULTIPLY;
	else
		return (addr.value * addr.value + 107 * addr.value + 1023) * MH_MULTIPLY;
}
/**
 * @brief find hash node by address, if node does not in the table, insert a new node 
 * @param addr
 * @return the pointer to the node in given address, NULL when error 
 **/
static inline _cesk_diff_hash_node_t* _cesk_diff_hash_find(cesk_diff_frame_addr_t addr)
{
	hashval_t h = _cesk_diff_hashcode(addr)%CESK_DIFF_HASH_SIZE;
	_cesk_diff_hash_node_t* ptr;
	/* try to find the node in the hash */
	for(ptr = _cesk_diff_hash[h];
		NULL != ptr && _cesk_diff_alloc_token == ptr->token;
		ptr = ptr->hash_next) 
		if(ptr->address.type == addr.type && ptr->address.value == addr.value)
			return ptr;
	/* we didn't find any existing node here */
	if(NULL != _cesk_diff_hash[h] && _cesk_diff_alloc_token != ptr->token)
		_cesk_diff_hash[h] = NULL;
	ptr = _cesk_diff_hash_node_alloc();
	if(NULL == ptr)
	{
		LOG_ERROR("can not allocate memory for new reduction hash table node");
		return NULL;
	}
	ptr->address.value = addr.value;
	ptr->address.type  = addr.type;
	ptr->hash_next = _cesk_diff_hash[h];
	_cesk_diff_hash[h] = ptr;
	return ptr;
}
cesk_diff_item_t* cesk_diff_item_new()
{
	cesk_diff_item_t* ret = (cesk_diff_item_t*)malloc(sizeof(cesk_diff_item_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate diff item");
		return NULL;
	}
	memset(ret, 0, sizeof(cesk_diff_item_t));
	return ret;
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
		LOG_ERROR("can not allocate new diff package");
		return NULL;
	}

	ret->head = NULL;
	ret->tail = NULL;
	/* the diff have never been reduced */
	ret->red = 0;
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
		if(node->type == CESK_DIFF_ITEM_TYPE_SET)
			cesk_value_decref(node->data.value);
		cesk_diff_item_free(node);
	}
	free(diff);
}

int cesk_diff_push_front(cesk_diff_t* diff, cesk_diff_item_t* item)
{
	if(NULL == diff || NULL == item)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(diff->red)
	{
		LOG_ERROR("can not modify a reduced diff package");
		return -1;
	}
	item->next = diff->head;
	diff->head = item;
	if(NULL == diff->tail) 
		diff->tail = diff->head;  /* if this is the first element in the list */
	return 0;
}

int cesk_diff_push_back(cesk_diff_t* diff, cesk_diff_item_t* item)
{
	if(NULL == diff || NULL == item)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	if(diff->red)
	{
		LOG_ERROR("can not modofiy a reduced diff package");
		return -1;
	}
	item->next = NULL;
	if(NULL != diff->tail)diff->tail->next = item;
	diff->tail = item;
	if(NULL == diff->head) /* if this is the first element in the list */
		diff->head = diff->tail;
	return 0;
}
/**
 * @brief merge allocation with a diff item
 * @param first the first diff item (ealier one)
 * @param second the second diff item (later one)
 * @return the result of reduction, <0 means error.
 **/
static inline int _cesk_diff_item_merge_allocation(cesk_diff_item_t* first, cesk_diff_item_t* second)
{
	switch(second->type)
	{
		case CESK_DIFF_ITEM_TYPE_ALLOCSET:
			/* allocation + allocation-set ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCREUSE:
			/* allocation + allocation-reuse ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			/* allocation + allocation ==> impossible */
			LOG_ERROR("can not allocate same address in same store twice");
			return -1;
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			/* allocation + deallocation ==> nop */
			first->type = CESK_DIFF_ITEM_TYPE_NOP;
			return 0;
		case CESK_DIFF_ITEM_TYPE_REUSE:
			/* allocation + reuse ==> allocation & reuse */
			first->type = CESK_DIFF_ITEM_TYPE_ALLOCREUSE;
			first->data.reuse = second->data.reuse;
			return 0;
		case CESK_DIFF_ITEM_TYPE_SET:
			/* allocation + set ==> allocatoin-set */
			first->type = CESK_DIFF_ITEM_TYPE_ALLOCSET;
			first->data.value = second->data.value;
			return -1;
		case CESK_DIFF_ITEM_TYPE_NOP:
			/* allocation + nop = allocation */
			return 0;
		default:
			LOG_ERROR("invalid diff item type");
			return -1;
	}
}
/**
 * @brief merge deallocation with a diff item
 * @param first the first diff item (ealier one)
 * @param second the second diff item (later one)
 * @return the result of reduction, <0 means error.
 **/
static inline int _cesk_diff_item_merge_deallocation(cesk_diff_item_t* first, cesk_diff_item_t* second)
{
	switch(second->type)
	{
		case CESK_DIFF_ITEM_TYPE_ALLOCREUSE:
			/* deallocation + allocation-reuse ==> reuse */
			first->type = CESK_DIFF_ITEM_TYPE_REUSE;
			first->data.reuse = second->data.reuse;
			return 0;
		case CESK_DIFF_ITEM_TYPE_ALLOCSET:
			/* deallocation + allocation-set ==> set */
			first->type = CESK_DIFF_ITEM_TYPE_SET;
			first->data.value = second->data.value;
			return 0;
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			/* deallocation + allocation ==> nop */
			first->type = CESK_DIFF_ITEM_TYPE_NOP;
			return 0;
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			/* deallocation + deallocation ==> impossible */
		case CESK_DIFF_ITEM_TYPE_REUSE:
			/* deallocation + reuse ==> impossible */
		case CESK_DIFF_ITEM_TYPE_SET:
			/* deallocation + set ==> impossible */
			LOG_ERROR("can not merge, becuase the object is already dead");
			return -1;
		case CESK_DIFF_ITEM_TYPE_NOP:
			return 0;
		default:
			LOG_ERROR("invalid diff item type");
			return -1;
	}
}
/**
 * @brief merge allocation-reuse with a diff item
 * @param first the first diff item (ealier one)
 * @param second the second diff item (later one)
 * @return the result of reduction, <0 means error.
 **/
static inline int _cesk_diff_item_merge_allocreuse(cesk_diff_item_t* first, cesk_diff_item_t* second)
{
	switch(second->type)
	{
		case CESK_DIFF_ITEM_TYPE_ALLOCSET:
			/* allocation-set + allocation-reuse ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCREUSE:
			/* allocation-reuse + allocation-reuse ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			/* allocation-reuse + allocation ==> impossible */
			LOG_ERROR("can not allocate same address in same store twice");
			return -1;
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			/* allocation-reuse + deallocation ==> impossible*/
			//TODO verify this case
			LOG_ERROR("because the same address is only allocated by the same instruction, so this is impossible");
			return -1;
		case CESK_DIFF_ITEM_TYPE_REUSE:
			/* allocation-reuse + reuse ==> allocation & reuse */
			first->type = CESK_DIFF_ITEM_TYPE_ALLOCREUSE;
			first->data.reuse = second->data.reuse;
			return 0;
		case CESK_DIFF_ITEM_TYPE_SET:
			/* allocation-reuse + set ==> impossible */
			LOG_ERROR("can not combine allocation and set, because type do not match");
			return -1;
		case CESK_DIFF_ITEM_TYPE_NOP:
			/* allocation + nop = allocation */
			return 0;
		default:
			LOG_ERROR("invalid diff item type");
			return -1;
	}
}
/**
 * @brief merge reuse with a diff item
 * @param first the first diff item (ealier one)
 * @param second the second diff item (later one)
 * @return the result of reduction, <0 means error.
 **/
static inline int _cesk_diff_item_merge_reuse(cesk_diff_item_t* first, cesk_diff_item_t* second)
{
	switch(second->type)
	{
		case CESK_DIFF_ITEM_TYPE_ALLOCSET:
			/* reuse + allocation-set ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			/* reuse + allocation ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCREUSE:
			/* reuse + allocation-reuse => impossible */
			LOG_ERROR("can not merge allocation before reuse, because it's not possible");
			return -1;
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			/* reuse + deallocation ==> impossible */
			LOG_ERROR("deallocation must followed by allocation");
			return -1;
		case CESK_DIFF_ITEM_TYPE_REUSE:
			/* reuse + reuse ==> reuse */
			first->type = CESK_DIFF_ITEM_TYPE_REUSE;
			first->data.reuse = second->data.reuse;
			return 0;
		case CESK_DIFF_ITEM_TYPE_SET:
			/* reuse + set == impossible */
			LOG_ERROR("can not merge reuse and set, type do not match");
			return -1;
		case CESK_DIFF_ITEM_TYPE_NOP:
			/* reuse + nop ==> reuse */
			return 0;
		default:
			LOG_ERROR("invalid diff item type");
			return -1;
			
	}
}
/**
 * @brief merge set with a diff item
 * @param first the first diff item (ealier one)
 * @param second the second diff item (later one)
 * @return the result of reduction, <0 means error.
 **/
static inline int _cesk_diff_item_merge_set(cesk_diff_item_t* first, cesk_diff_item_t* second)
{
	switch(second->type)
	{
		case CESK_DIFF_ITEM_TYPE_ALLOCSET:
			/* set + allocation-set ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			/* set + allocation ==> impossible */
		case CESK_DIFF_ITEM_TYPE_ALLOCREUSE:
			/* set + allocation-reuse ==> impossible */
			LOG_ERROR("can not allocate after set");
			return -1;
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			/* set + deallocate ==> impossible */
			LOG_ERROR("can not merge deallocation before set");
			return -1;
		case CESK_DIFF_ITEM_TYPE_REUSE:
			/* set + reuse ==> impossible */
			LOG_ERROR("type mismatch");
			return -1;
		case CESK_DIFF_ITEM_TYPE_SET:
			/* set + set ==> set */
			first->type = CESK_DIFF_ITEM_TYPE_SET;
			cesk_value_decref(first->data.value);
			first->data.value = second->data.value;
			return 0;
		case CESK_DIFF_ITEM_TYPE_NOP:
			/* set + nop ==> set */
			return 0;
		default:
			LOG_ERROR("invalid diff item type");
			return -1;
	}
}
/**
 * @brief merge allocation-set with a diff item
 * @param first the first diff item (ealier one)
 * @param second the second diff item (later one)
 * @return the result of reduction, <0 means error.
 **/
static inline int _cesk_diff_item_merge_allocset(cesk_diff_item_t* first, cesk_diff_item_t* second)
{
	switch(second->type)
	{
		case CESK_DIFF_ITEM_TYPE_ALLOCSET:
		case CESK_DIFF_ITEM_TYPE_ALLOCREUSE:
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			/* allocate-set + alloc* ==> impossible */
			LOG_ERROR("can not allocate twice");
			return -1;
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			/* allocate-set + deallocate ==>impossible */
			LOG_ERROR("can not merge allocation-set an deallocate, not possible");
			return -1;
		case CESK_DIFF_ITEM_TYPE_REUSE:
			/* allocate-set + reuse ==> impossible */
			LOG_ERROR("type mismatch");
			return -1;
		case CESK_DIFF_ITEM_TYPE_SET:
			/* allocation-set + set ==> allocation-set */
			first->type = CESK_DIFF_ITEM_TYPE_ALLOCSET;
			cesk_value_decref(first->data.value);
			first->data.value = second->data.value;
			return 0;
		case CESK_DIFF_ITEM_TYPE_NOP:
			/* allocation-set + nop ==> allocation-set */
			return 0;
		default:
			LOG_ERROR("invalid diff item type");
			return -1;
	}
}
/**
 * @brief reduce two diff item into one, and store the result in the first item
 * @param first the first diff item (ealier one)
 * @param second the second diff item (later one)
 * @return the result of reduction, <0 means error
 * @note   this function do not free the second diff item, so if the function returns
 *         a success value, caller should free the second diff item
 **/
static inline int _cesk_diff_item_reduce(cesk_diff_item_t* first, cesk_diff_item_t* second)
{
	/* we do not check the validity of the pointers */
	LOG_DEBUG("merge two diff item");
	LOG_DEBUG("first = %s", cesk_diff_item_to_string(first, NULL));
	LOG_DEBUG("second = %s", cesk_diff_item_to_string(second, NULL));
	/* verify two items affects the same address */
	if(first->frame_addr.type != second->frame_addr.type ||
	   first->frame_addr.value != second->frame_addr.value)
	{
		LOG_ERROR("can not reduce two diff items affets different addresses to one");
		return -1;
	}
	switch(first->type)
	{
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			return _cesk_diff_item_merge_allocation(first, second);
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			return _cesk_diff_item_merge_deallocation(first, second);
		case CESK_DIFF_ITEM_TYPE_ALLOCREUSE:
			return _cesk_diff_item_merge_allocreuse(first, second);
		case CESK_DIFF_ITEM_TYPE_NOP:
			/* just copy the second item to the first */
			first->type = second->type;
			first->data.value = second->data.value;  /* because the reuse flag is smaller than pointer, so we only perform a 
			                                            pointer assignment instead of do a bit assignment */
			return 0;
		case CESK_DIFF_ITEM_TYPE_REUSE:
			return _cesk_diff_item_merge_reuse(first, second);
		case CESK_DIFF_ITEM_TYPE_SET:
			return _cesk_diff_item_merge_set(first, second);
		case CESK_DIFF_ITEM_TYPE_ALLOCSET:
			return _cesk_diff_item_merge_allocset(first, second);
		default:
			LOG_ERROR("invalid diff item type");
			return -1;
	}
}
int cesk_diff_reduce(cesk_diff_t* diff)
{
	if(NULL == diff)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(diff->red) return 0;
	cesk_diff_item_t* ptr;
	for(ptr = diff->head; NULL != ptr;)
	{
		cesk_diff_item_t* this = ptr;
		ptr = ptr->next;
		/* do merge */
		_cesk_diff_hash_node_t* node = _cesk_diff_hash_find(this->frame_addr);
		if(NULL == node)
		{
			LOG_ERROR("failed to aquire the node for address <%u,%u>", this->frame_addr.type, this->frame_addr.value);
			return -1;
		}
		else
		{
			if(NULL == node->reduced)
			{
				/* if this node is newly created, just assign current diff item to the reduced one */
				node->reduced = this;
			}
			else
			{
				if(_cesk_diff_item_reduce(node->reduced, this) < 0)
				{
					LOG_ERROR("can not reduce the diff item");
					return -1;
				}
				cesk_diff_item_free(this);
			}
		}
	}
	/* rebuild the reduced diff */
	diff->head = NULL;
	diff->tail = NULL;
	diff->red  = 1;
	_cesk_diff_hash_node_t* np;
	for(np = _cesk_diff_node_list; 
		np != NULL;
		np = np->list_next)
	{
		cesk_diff_item_t* this = np->reduced;
		this->next = diff->head;
		diff->head = this;
		if(NULL == diff->tail)
			diff->tail = this;
	}
	/* clear the temp hash table */
	_cesk_diff_hash_clear();
	return 0;
}
#define __PR(fmt, args...) buf += sprintf(buf, fmt, ##args)
/**
 * @brief output human readable address to buffer
 **/
static char* _cesk_diff_write_address(const cesk_diff_frame_addr_t* addr, char* buf)
{
	switch(addr->type)
	{
		case CESK_DIFF_FRAME_STORE:
			__PR("(store @0x%x)", addr->value);
			break;
		case CESK_DIFF_FRAME_REGISTER:
			__PR("(register %u)", addr->value);
			break;
		default:
			__PR("(invalid-address-type)");
	}
	return buf;
}
const char* cesk_diff_item_to_string(const cesk_diff_item_t* item, char* buf)
{
	static char __default_buf__[128];
	if(NULL == buf)
		buf = __default_buf__;
	char* ret = buf;
	switch(item->type)
	{
		case CESK_DIFF_ITEM_TYPE_NOP:
			__PR("(nop)");
			break;
		case CESK_DIFF_ITEM_TYPE_DEALLOCATE:
			__PR("(dellocate ");
			goto __PA;
		case CESK_DIFF_ITEM_TYPE_ALLOCATE:
			__PR("(allocate ");
__PA:
			buf = _cesk_diff_write_address(&item->frame_addr, buf);
			buf[0] = ')';
			buf[1] = 0;
			buf ++;
			break;
		case CESK_DIFF_ITEM_TYPE_REUSE:
			__PR("(reuse ");
			buf = _cesk_diff_write_address(&item->frame_addr, buf);
			__PR("%u)", item->data.reuse);
			break;
		case CESK_DIFF_ITEM_TYPE_SET:
			__PR("(set ");
			buf = _cesk_diff_write_address(&item->frame_addr, buf);
			__PR("%p)", item->data.value);
			break;
		default:
			__PR("(invalid-diff-item-type ");
			buf = _cesk_diff_write_address(&item->frame_addr, buf);
			__PR("%p)", item->data.value);
	}
	return ret;
}
