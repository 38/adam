#include <cesk/cesk_method.h>
/* types */

/**
 * @brief node in cache , use [block, frame] as key
 **/
typedef struct _cesk_method_cache_node_t{
	const dalvik_block_t* code;  /*!< the code block */
	cesk_frame_t* frame;          /*!< the stack frame */
	cesk_diff_t* result;          /*!< the analyze result */
	struct _cesk_method_cache_node_t* next;  /*!< next pointer for the hash table */
} _cesk_method_cache_node_t;

typedef struct _cesk_method_block_context_t _cesk_method_block_context_t;
/**
 * @brief the branch information, because we consider the value assemption we 
 *        flollows into conditional branch. So although the input and the code
 *        are the same, but in different branch, the output value might be different
 *        So that we need this struct to store the branch information
 **/
typedef struct {
	_cesk_method_block_context_t* code;  /*!< the code block struct that produces this input, if NULL this is a return branch */
	uint32_t index;                  /*!< the index of the branch in the code block */
	cesk_frame_t*  frame;            /*!< the result stack frame of this block in this branch*/
	cesk_diff_t* prv_inversion;      /*!< the previous (second youngest result) inversive diff (from branch output to block input) */
	cesk_diff_t* cur_diff;           /*!< the current (the youngest result) compute diff (from block input to branch output) */
	cesk_diff_t* cur_inversion;      /*!< the current (the youngest result) inversive diff (from branch output to block input) */
} _cesk_method_block_input_t;
/**
 * @brief the data structure we used for intermedian data storage
 **/
struct _cesk_method_block_context_t{
	const dalvik_block_t* code;  /*!< the code for this block */
	uint32_t timestamp;          /*!< when do we analyze this block last time */
	cesk_diff_t* input_diff;     /*!< the diff from previous input to current input */
	uint32_t ninputs;            /*!< number of inputs */
	_cesk_method_block_input_t* inputs;  /*!< the input branches */
	uint32_t* input_index;      /*!< input_index[k] is the index of struct for the k-th branch blocklist[block.code.index].inputs[block.input_index[k]]*/
};
/**
 * @brief an analyzer context 
 **/
typedef struct {
	uint32_t Q[CESK_METHOD_MAX_NBLOCKS]; /*!< the analyzer queue */ 
	uint32_t front;                      /*!< the earlist timestamp in the queue */
	uint32_t rear;                       /*!< next fresh timestamp */
	cesk_reloc_table_t* rtable;          /*!< relocation table*/
	cesk_alloctab_t*    atable;          /*!< allocation table*/
	uint32_t nslots;                     /*!< the number of slots */
	_cesk_method_block_context_t blocks[0];  /*!< the block contexts */
} _cesk_method_context_t;

/* static globals */

/**
 * @brief the method analysis cache 
 **/
static _cesk_method_cache_node_t* _cesk_method_cache[CESK_METHOD_CAHCE_SIZE];
/**
 * @brief the max block index in current method
 **/
static uint32_t _cesk_method_block_max_idx;
/**
 * @brief the max number of inputs 
 **/
static uint32_t _cesk_method_block_max_ninputs;
/**
 * @brief all blocks in this method 
 **/
static const dalvik_block_t* _cesk_method_block_list[CESK_METHOD_MAX_NBLOCKS];
/**
 * @brief how many inputs does this block have 
 **/
static int _cesk_method_block_ninputs[CESK_METHOD_MAX_NBLOCKS];


int cesk_method_init()
{
	memset(_cesk_method_cache, 0, sizeof(_cesk_method_cache));
	return 0;
}
void cesk_method_finalize()
{
	int i;
	for(i = 0; i < CESK_METHOD_CAHCE_SIZE; i ++)
	{
		_cesk_method_cache_node_t* node;
		for(node = _cesk_method_cache[i]; NULL != node;)
		{
			_cesk_method_cache_node_t* current = node;
			node = node->next;
			if(current->frame) cesk_frame_free(current->frame);
			if(current->result) cesk_diff_free(current->result);
			free(node);
		}
	}
}
/**
 * @brief the hash code used by the method analyzer cache, the key is the code block and current stack frame
 * @param code current code block
 * @param frame current stack frame
 * @return the hashcode computed from the input key pair
 **/
static inline hashval_t _cesk_method_cache_hash(const dalvik_block_t* code, const cesk_frame_t* frame)
{
	return (((((uintptr_t)code)&(~0L)) * (((uintptr_t)code)&(~0L))) + 
	       0x35fbc27 * (((uintptr_t)code)>>32)) ^  /* for 32 bit machine, this part is 0 */
		   cesk_frame_hashcode(frame);
}
/**
 * @brief allocate a new cache node 
 * @param code current code block
 * @param frame current stack frame
 * @return the pointer to the newly created cache node, NULL indicates there's some error
 **/
static inline _cesk_method_cache_node_t* _cesk_method_cache_node_new(const dalvik_block_t* code, const cesk_frame_t* frame)
{
	_cesk_method_cache_node_t* ret = (_cesk_method_cache_node_t*)malloc(sizeof(_cesk_method_cache_node_t));
	if(NULL == ret) 
	{
		LOG_ERROR("can not allocate memory for the cesk method cache node");
		return NULL;
	}
	ret->frame = cesk_frame_fork(frame);
	ret->code = code;
	ret->result = NULL;
	ret->next = NULL;
	return ret;
}
/**
 * @brief insert a new node into the method analyzer cache
 * @note in this function we assume that there's no duplicate node in the table. And the caller
 *       should guareentee this
 * @param code current code block
 * @param frame current stack frame
 * @param return the pointer to the newly inserted node, NULL indicates there's some error
 **/
static inline _cesk_method_cache_node_t* _cesk_method_cache_insert(const dalvik_block_t* code, const cesk_frame_t* frame)
{
	hashval_t h = _cesk_method_cache_hash(code, frame);
	_cesk_method_cache_node_t* node = _cesk_method_cache_node_new(code, frame);
	if(NULL == node)
	{
		LOG_ERROR("can not allocate node for method analyzer cache");
		return NULL;
	}
	node->next = _cesk_method_cache[h%CESK_METHOD_CAHCE_SIZE];
	_cesk_method_cache[h%CESK_METHOD_CAHCE_SIZE] = node;
	return node;
}
/**
 * @brief find if there's an record in the cache matches the input key
 * @param code the current code block
 * @param frame the current stack frame
 * @return the pointer to the node that matches the input key pair, NULL indicates nothing found
 **/
static inline _cesk_method_cache_node_t* _cesk_method_cache_find(const dalvik_block_t* code, const cesk_frame_t* frame)
{
	hashval_t h = _cesk_method_cache_hash(code, frame);
	_cesk_method_cache_node_t* node;
	for(node = _cesk_method_cache[h%CESK_METHOD_CAHCE_SIZE]; NULL != node; node = node->next)
	{
		if(node->code == code && cesk_frame_equal(frame, node->frame))
		{
			return node;
		}
	}
	return NULL;
}

/** 
 * @brief explore the code block graph and save the pointer to all blocks in
 *        _cesk_method_block_list
 * @param entry the entry pointer
 * @return < 0 indicates an error
 **/
static inline int _cesk_method_explore_code(const dalvik_block_t* entry)
{
	if(NULL == entry) return 0;
	LOG_DEBUG("found block #%d", entry->index);
	_cesk_method_block_list[entry->index] = entry;
	if(_cesk_method_block_max_idx < entry->index || _cesk_method_block_max_idx == (~0L))
		_cesk_method_block_max_idx = entry->index;
	int i;
	for(i = 0; i < entry->nbranches; i ++)
	{
		const dalvik_block_branch_t* branch = entry->branches + i;
		if(branch->disabled || (0 == branch->conditional && DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(*branch)))
		{
			LOG_DEBUG("ignore the disabled and return branch");
			continue;
		}
		_cesk_method_block_ninputs[branch->block->index] ++;
		if(NULL != _cesk_method_block_list[branch->block->index]) continue;
		if(_cesk_method_explore_code(branch->block) < 0)
		{
			LOG_ERROR("can not explore the code blocks from block #%d", branch->block->index);
			return -1;
		}
	}
	return 0;
}
/**
 * @brief free a method analyzer context
 * @param context the context to be freed
 * @return nothing
 **/
static inline void _cesk_method_context_free(_cesk_method_context_t* context)
{
	if(NULL == context) return;
	int b;
	for(b = 0; b < context->nslots; b ++)
	{
		_cesk_method_block_context_t* block = context->blocks + b;
		if(NULL == block->code) continue;
		if(NULL != block->input_diff) cesk_diff_free(block->input_diff);
		if(NULL != block->input_index) free(block->input_index);
		int i;
		if(NULL != block->inputs)
		{
			for(i = 0; i < block->ninputs; i ++)
			{
				if(NULL != block->inputs[i].prv_inversion) cesk_diff_free(block->inputs[i].prv_inversion);
				if(NULL != block->inputs[i].cur_inversion) cesk_diff_free(block->inputs[i].cur_inversion);
				if(NULL != block->inputs[i].cur_diff) cesk_diff_free(block->inputs[i].cur_diff);
				if(NULL != block->inputs[i].frame) cesk_frame_free(block->inputs[i].frame);
			}
			free(block->inputs);
		}
	}
	if(NULL != context->rtable) cesk_reloc_table_free(context->rtable);
	if(NULL != context->atable) cesk_alloctab_free(context->atable);
	free(context);
}
/**
 * @brief initialize the method analyzer
 * @param entry the entry point of this method
 * @return the analyzer context
 **/
static inline _cesk_method_context_t* _cesk_method_context_new(const dalvik_block_t* entry)
{
	_cesk_method_context_t* ret;
	_cesk_method_block_max_ninputs = 0;
	_cesk_method_block_max_idx = ~0L;
	memset(_cesk_method_block_list, 0, sizeof(_cesk_method_block_list));
	memset(_cesk_method_block_ninputs, 0, sizeof(_cesk_method_block_ninputs));
	/* first explore all blocks belongs to this method */
	if(_cesk_method_explore_code(entry) < 0)
	{
		LOG_ERROR("can not explor the code block graph");
		return NULL;
	}
	/* construct context */
	size_t context_size = sizeof(_cesk_method_context_t) + (_cesk_method_block_max_idx + 1) * sizeof(_cesk_method_block_context_t);
	ret = (_cesk_method_context_t*)malloc(context_size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for analyzer context");
		return NULL;
	}
	memset(ret, 0, context_size);
	ret->nslots = _cesk_method_block_max_idx + 1;
	/* create allocation table and relocation table */
	ret->rtable = cesk_reloc_table_new();
	if(NULL == ret->rtable)
	{
		LOG_ERROR("can not create new relocation table");
		goto ERR;
	}
	ret->atable = cesk_alloctab_new();
	if(NULL == ret->atable)
	{
		LOG_ERROR("can not create new allocation table");
		goto ERR;
	}
	/* initialize all context for every blocks */
	int i;
	for(i = 0; i < _cesk_method_block_max_idx + 1; i ++)
	{
		/* skip block that not reachable */
		if(NULL == _cesk_method_block_list[i]) continue;
		ret->blocks[i].code = _cesk_method_block_list[i];
		ret->blocks[i].input_index = (uint32_t*)malloc(_cesk_method_block_ninputs[i]);

		/* TODO */
	}
	return ret;
ERR:
	if(NULL != ret) _cesk_method_context_free(ret);
	return -1;
}
