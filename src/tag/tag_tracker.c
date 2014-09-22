#include <tag/tag_tracker.h>
/**
 * @brief the abstract virtual machine state
 **/
typedef struct {
	uint32_t closure; /*!< the closure index */
	uint32_t block;   /*!< the block index */
	uint32_t target;  /*!< the target block index */
	uint32_t instruction; /*!< the instruction index */
} _tag_tracker_avm_stat_t;
/** 
 * @brief the hash node
 **/
typedef struct _tag_tracker_hash_node_t _tag_tracker_hash_node_t;
struct _tag_tracker_hash_node_t{
	uint32_t what;   /*!< the index of the tag set */
	_tag_tracker_avm_stat_t when; /*!< when this tag set created */
	tag_set_t* set;   /*!< the tag set itself */
	uint32_t visit_flag; /*!< the vistited flag */
	_tag_tracker_hash_node_t* next; /*!< the next node */
	size_t ninputs;
	uint32_t inputs[0];
};
/**
 * @breif the hashtable
 **/
_tag_tracker_hash_node_t* _tag_tracker_hash[TAG_TRACKER_HASH_SIZE];
/**
 * @brief check if there's a currently opening transaction
 **/
static uint32_t _tag_tracker_sp = 0;
/**
 * @brief the transaction detials
 **/
static _tag_tracker_avm_stat_t _tag_tracker_current_stack[TAG_TRACKER_STACK_SIZE];

/**
 * @brief the hash code for tag-set index
 * @param tags_idx the tag-set index
 * @return the result hashcode
 **/
hashval_t _tag_tracker_tagset_hashcode(uint32_t tags_idx)
{
	return tags_idx * MH_MULTIPLY;
}

/**
 * @brief create a new hash node
 * @param tsid the tag-set index
 * @param avmst the abstract virtual machine status
 * @param set the tag-set
 * @return a newly created node, NULL if error
 **/
_tag_tracker_hash_node_t* _tag_tracker_hash_new(uint32_t tsid, const _tag_tracker_avm_stat_t avmst, const tag_set_t* set, size_t ninputs, const uint32_t* inputs)
{
	_tag_tracker_hash_node_t* ret = (_tag_tracker_hash_node_t*)malloc(sizeof(_tag_tracker_hash_node_t) + sizeof(uint32_t) * ninputs);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate a new tracker hash node");
		return 0;
	}
	ret->what = tsid;
	ret->when = avmst;
	ret->set = tag_set_fork(set);
	ret->next = NULL;
	ret->ninputs = ninputs;
	memcpy(ret->inputs, inputs, sizeof(uint32_t) * ninputs);
	return ret;
}
/**
 * @brief insert a new node to the hash, assume that there's no duplications
 * @param what the tag-set index
 * @param set the tag-set
 * @param avmst the avm status
 * @return the newly inserted node in the hash table, NULL if error
 **/
static inline const _tag_tracker_hash_node_t* _tag_tracker_hash_insert(uint32_t what, const tag_set_t* set, const _tag_tracker_avm_stat_t avmst, size_t ninputs, const uint32_t* inputs)
{
	uint32_t slot_id = _tag_tracker_tagset_hashcode(what) % TAG_TRACKER_HASH_SIZE;
	_tag_tracker_hash_node_t* node = _tag_tracker_hash_new(what, avmst, set, ninputs, inputs);
	if(NULL == node)
	{
		LOG_ERROR("can not create the node");
		return NULL;
	}
	node->next = _tag_tracker_hash[slot_id];
	_tag_tracker_hash[slot_id] = node;
	return node;
}
/**
 * @brief find a node by the tag-set id
 * @param tsid the tag-set id
 * @return the node found, NULL when not found
 **/
static inline _tag_tracker_hash_node_t* _tag_tracker_hash_find(uint32_t tsid)
{
	uint32_t slot_id = _tag_tracker_tagset_hashcode(tsid) % TAG_TRACKER_HASH_SIZE;
	_tag_tracker_hash_node_t* ptr;
	for(ptr = _tag_tracker_hash[slot_id]; NULL != ptr && ptr->what != tsid; ptr = ptr->next);
	return ptr;
}
int tag_tracker_init()
{
	return 0;
}
void tag_tracker_finalize()
{
	int i;
	for(i = 0; i < TAG_TRACKER_HASH_SIZE; i ++)
	{
		_tag_tracker_hash_node_t* ptr;
		for(ptr = _tag_tracker_hash[i]; NULL != ptr; )
		{
			_tag_tracker_hash_node_t* cur = ptr;
			ptr = ptr->next;
			if(NULL != cur->set) tag_set_free(cur->set);
			free(cur);
		}
	}
}
/**
 * @brief open a transaction
 * @param closure the closture id
 * @param instruction the instruction id
 * @param block the block id
 * @param target the target block id
 * @return < 0 for error
 **/
int _tag_tracker_transaction_open(uint32_t closure, uint32_t instruction, uint32_t block, uint32_t target)
{
	if(_tag_tracker_sp >= TAG_TRACKER_STACK_SIZE)
	{
		LOG_ERROR("stack overflow");
		return -1;
	}
	_tag_tracker_avm_stat_t stat = {
		.closure = closure,
		.instruction = instruction,
		.block = -1,
		.target = -1
	};
	_tag_tracker_current_stack[_tag_tracker_sp ++] = stat;
	return 0;
}
int tag_tracker_instruction_transaction_begin(uint32_t closure, uint32_t instruction)
{
	return _tag_tracker_transaction_open(closure, instruction, -1, -1);
}

int tag_tracker_block_transaction_begin(uint32_t closure, uint32_t blockidx)
{
	return _tag_tracker_transaction_open(closure, -1, blockidx, -1);
}
int tag_tracker_branch_transaction_begin(uint32_t closure, uint32_t from, uint32_t to)
{
	return _tag_tracker_transaction_open(closure, -1, from, to);
}
int tag_tracker_transaction_close()
{
	if(_tag_tracker_sp) _tag_tracker_sp --;
	return 0;
}
int tag_tracker_register_tagset(uint32_t tsid, const tag_set_t* tagset, const uint32_t* inputs, uint32_t ninputs)
{
	if(!_tag_tracker_sp) return 0;
	if(NULL == _tag_tracker_hash_insert(tsid, tagset, _tag_tracker_current_stack[_tag_tracker_sp-1], ninputs, inputs)) 
		return -1;
	return 0;
}
static uint32_t** inst_buf;
static size_t _N;
static int _tag_tracker_dfs(uint32_t tag_id, uint32_t tagset_id, int depth, uint32_t tick)
{
	static uint32_t path[1024];
	_tag_tracker_hash_node_t* node = _tag_tracker_hash_find(tagset_id);
	if(NULL == node || node->ninputs == 0)
	{
		inst_buf[0] = (uint32_t*)malloc(sizeof(uint32_t) * depth);
		memcpy(inst_buf[0], path, sizeof(uint32_t) * depth);
		_N--;
		inst_buf ++;
		return 1;
	}
	if(tick == node->visit_flag || !tag_set_contains(node->set, tag_id))
		return 0;
	node->visit_flag = 1;
	if(node->when.instruction != -1) path[depth ++] = node->when.instruction;
	int i , rc = 0;
	for(i = 0; i < node->ninputs; i ++)
		rc += _tag_tracker_dfs(tag_id, node->inputs[i], depth, tick);
	return rc;
}
int tag_tacker_get_path(uint32_t tag_id, uint32_t tagset_id, uint32_t** instruction, size_t N)
{
	inst_buf = instruction;
	_N = N;
	static int tick = 0;
	return _tag_tracker_dfs(tag_id, tagset_id, 0, tick ++);
}
