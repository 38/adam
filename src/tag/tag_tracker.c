#include <tag/tag_tracker.h>
/**
 * @brief the abstract machine state
 **/
typedef struct {
	uint32_t closure; /*!< the closure index */
	uint32_t block; /*!< the block index */
	uint32_t instruction; /*!< the instruction index */
} _tag_tracker_avm_stat;
/**
 * @brief the node of the tag tracker
 **/
typedef struct _tag_tracker_node_t _tag_tracker_node_t;

struct _tag_tracker_node_t{
	uint32_t tag; /*!< the tag index */
	_tag_tracker_avm_stat dest;  /*!< the destination vm stat (aka the key) */
	_tag_tracker_avm_stat sour; /*!< the source vm stat (aka the value) */
	_tag_tracker_node_t* next;  /*!< the 'next' pointer in the hash table */
};

static _tag_tracker_node_t* _tag_tracker_table[TAG_TRACKER_HASH_SIZE];

int tag_tracker_init()
{
	return 0;
}
void tag_tracker_finalize()
{
	uint32_t i;
	for(i = 0; i < TAG_TRACKER_HASH_SIZE; i ++)
	{
		_tag_tracker_node_t *ptr;
		for(ptr = _tag_tracker_table[i]; NULL != ptr;)
		{
			_tag_tracker_node_t* cur = ptr;
			ptr = ptr->next;
			free(cur);
		}
	}
}

static inline hashval_t _tag_tracker_node_hash(uint32_t tag, uint32_t closure, uint32_t block, uint32_t inst)
{
	return (tag * MH_MULTIPLY + (closure * closure + 1007 * closure + 23) * MH_MULTIPLY) ^
		   ((block * block + block * MH_MULTIPLY) * MH_MULTIPLY * MH_MULTIPLY - inst * (~MH_MULTIPLY));
}
static inline _tag_tracker_node_t* _tag_tracker_node_new(uint32_t tag, _tag_tracker_avm_stat dest, _tag_tracker_avm_stat sour)
{
	_tag_tracker_node_t* ret = (_tag_tracker_node_t*)malloc(sizeof(_tag_tracker_node_t));
	if(NULL == ret) 
	{
		LOG_ERROR("can not allocate memory for new tag tracker node");
		return NULL;
	}
	ret->tag = tag;
	ret->dest = dest;
	ret->sour = sour;
	return ret;
}
