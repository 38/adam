#include <tag/tag_tracker.h>
/**
 * @brief a dummy index
 **/
#define _TAG_TRACKER_IDX_DUMMY 0xfffffffful;
/**
 * @brief the abstract machine state
 **/
typedef struct {
	uint32_t closure; /*!< the closure index */
	uint32_t block; /*!< the block index */
	uint32_t instruction; /*!< the instruction index */
	uint32_t reg;  /*!< the register invovles */
} _tag_tracker_avm_stat;
/**
 * @brief the node of the tag tracker
 **/
typedef struct _tag_tracker_node_t _tag_tracker_node_t;
/**
 * @brief the source of edge
 **/
typedef struct _tag_tracker_source_t _tag_tracker_source_t;

struct _tag_tracker_node_t{
	uint32_t tag; /*!< the tag index */
	_tag_tracker_avm_stat dest;  /*!< the destination vm stat (aka the key) */
	_tag_tracker_source_t* sour;  /*!< the source list */
	_tag_tracker_node_t* next;  /*!< the 'next' pointer in the hash table */
};

struct _tag_tracker_source_t{
	_tag_tracker_avm_stat stat;    /*!< the source vm state */
	_tag_tracker_source_t* next;   /*!< linked list */
};
/**
 * @breif internal hash table 
 **/
static _tag_tracker_node_t* _tag_tracker_hashtab[TAG_TRACKER_HASH_SIZE];

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
		for(ptr = _tag_tracker_hashtab[i]; NULL != ptr;)
		{
			_tag_tracker_node_t* cur = ptr;
			ptr = ptr->next;
			_tag_tracker_source_t* sour;
			for(sour = cur->sour; NULL != sour;)
			{
				_tag_tracker_source_t* this_source = sour;
				sour = sour->next;
				free(this_source);
			}
			free(cur);
		}
	}
}
/**
 * @brief the hash code for tracker node 
 * @param tag the tag index
 * @param closure the closure index
 * @param block the block index
 * @param inst the insturction index
 * @param reg the register index
 * @return the hash code of the input node 
 **/
static inline hashval_t _tag_tracker_node_hash(uint32_t tag, uint32_t closure, uint32_t block, uint32_t inst, uint32_t reg)
{
	return (tag * MH_MULTIPLY + (closure * closure + 1007 * closure + 23) * MH_MULTIPLY) ^
		   ((block * block + block * MH_MULTIPLY) * MH_MULTIPLY * MH_MULTIPLY - inst * (~MH_MULTIPLY)) ^
		   ~(reg * MH_MULTIPLY);
}
/**
 * @brief create a new tracker node
 * @param tag the tag index
 * @param dest the destination state
 * @return the newly created node, returns NULL on error
 **/
static inline _tag_tracker_node_t* _tag_tracker_node_new(uint32_t tag, const _tag_tracker_avm_stat dest)
{
	_tag_tracker_node_t* ret = (_tag_tracker_node_t*)malloc(sizeof(_tag_tracker_node_t));
	if(NULL == ret) 
	{
		LOG_ERROR("can not allocate memory for new tag tracker node");
		return NULL;
	}
	ret->tag = tag;
	ret->dest = dest;
	ret->sour = NULL;
	ret->next = NULL;
	return ret;
}
/**
 * @brief create a new source 
 * @param stat the source vm state
 * @return the newly create source, return NULL on error
 **/
static inline _tag_tracker_source_t* _tag_tracker_source_new(const _tag_tracker_avm_stat stat)
{
	_tag_tracker_source_t* ret = (_tag_tracker_source_t*)malloc(sizeof(_tag_tracker_source_t));
	if(NULL == ret)
	{
		LOG_ERROR("can't allocate memory for new tracker source object");
		return NULL;
	}
	ret->stat = stat;
	ret->next = NULL;
	return ret;
}
/**
 * @brief insert a new node to the source list
 * @param node the node to perform insertion
 * @param sour new source
 * @return return < 0 on error
 **/
static inline int _tag_tracker_source_insert(_tag_tracker_node_t* node, const _tag_tracker_avm_stat sour)
{
	_tag_tracker_source_t* source = _tag_tracker_source_new(sour);
	if(NULL == source)
	{
		LOG_ERROR("failed to allocate new source");
		return -1;
	}
	source->next = node->sour;
	node->sour = source;
	return 0;
}
/**
 * @brief return wether or not two vm state are equal
 * @param this the first vm state
 * @param that the second vm state
 * @return > 0 if two inputs are equal, == 0 if tow inputs are not equal, < 0 on error
 **/
static inline int _tag_traceker_avm_stat_equal(const _tag_tracker_avm_stat this, const _tag_tracker_avm_stat that)
{
	return this.closure == that.closure && this.block == that.block && this.instruction == that.instruction && this.reg == that.reg;
}
/**
 * @brief find a tracker node by tag id and desttination
 * @param tag the tag id
 * @param dest the destination vm state
 * @return the tracker ndoe wanted, NULL if there's no such node
 **/
static inline _tag_tracker_node_t* _tag_tracker_find(uint32_t tag, const _tag_tracker_avm_stat dest)
{
	hashval_t hash = _tag_tracker_node_hash(tag, dest.closure, dest.block, dest.instruction, dest.reg) % TAG_TRACKER_HASH_SIZE;
	_tag_tracker_node_t* ptr;
	for(ptr = _tag_tracker_hashtab[hash]; NULL != ptr && !_tag_traceker_avm_stat_equal(ptr->dest, dest); ptr = ptr->next);
	return ptr;
}
/**
 * @biref insert an edge to the tracker node 
 * @param tag the tag id 
 * @param dest the destination vm state
 * @param sour the source vm state
 * @return the affected tracker node, NULL on error
 **/
static inline _tag_tracker_node_t* _tag_tracker_insert(uint32_t tag, const _tag_tracker_avm_stat dest, const _tag_tracker_avm_stat sour)
{
	_tag_tracker_node_t* ret = _tag_tracker_find(tag, dest);
	if(NULL == ret)
	{
		ret = _tag_tracker_node_new(tag, dest);
		if(NULL == ret)
		{
			LOG_ERROR("can't allocate new tracker node");
			return NULL;
		}
		hashval_t hash = _tag_tracker_node_hash(tag, dest.closure, dest.block, dest.instruction, dest.reg);
		ret->next = _tag_tracker_hashtab[hash];
		_tag_tracker_hashtab[hash] = ret;
	}
	_tag_tracker_source_t* ptr;
	for(ptr = ret->sour; NULL != ptr && !_tag_traceker_avm_stat_equal(ptr->stat, sour); ptr = ptr->next);
	if(NULL == ptr && _tag_tracker_source_insert(ret, sour) < 0)
	{
		LOG_ERROR("can not add an edge to the tag tracker");
		return NULL;
	}
	return ret;
}
#if 0
int tag_tracker_invocation(const tag_set_t* tag_set, uint32_t caller_closure, uint32_t invoke_inst, uint32_t callee_closure, uint32_t regidx)
{
	if(NULL == tag_set) return 0;
	uint32_t ntags = tag_set_size(tag_set);
	uint32_t i;
	for(i = 0; i < ntags; i ++)
	{
		uint32_t tid = tag_set_get_tagid(tag_set, i);
		_tag_tracker_avm_stat dest = {
			.closure = callee_closure,
			.block = _TAG_TRACKER_IDX_DUMMY,
			.instruction = _TAG_TRACKER_IDX_DUMMY,
			.reg = regidx
		};
		
	}
}
//TODO
#endif
