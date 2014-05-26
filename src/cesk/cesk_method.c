#include <cesk/cesk_method.h>
/**
 * @brief node in cache , use [block, frame] as key
 **/
typedef struct _cesk_method_cache_node_t{
	const dalvik_block_t* code;  /*!< the code block */
	cesk_frame_t* frame;          /*!< the stack frame */
	cesk_diff_t* result;          /*!< the analyze result */
	struct _cesk_method_cache_node_t* next;  /*!< next pointer for the hash table */
} _cesk_method_cache_node_t;
/**
 * @brief the cache 
 **/
_cesk_method_cache_node_t* _cesk_method_cache[CESK_METHOD_CAHCE_SIZE];

typedef struct _cesk_method_codeblock_t _cesk_method_codeblock_t;
/**
 * @brief the branch information, because we consider the value assemption we 
 *        flollows into conditional branch. So although the input and the code
 *        are the same, but in different branch, the output value might be different
 *        So that we need this struct to store the branch information
 **/
typedef struct {
	_cesk_method_codeblock_t* code;  /*!< the code block struct that produces this input */
	cesk_diff_t* prv_inversion;      /*!< the previous (second youngest result) inversive diff (from branch output to block input) */
	cesk_diff_t* cur_diff;           /*!< the current (the youngest result) compute diff (from block input to branch output) */
	cesk_diff_t* cur_inversion;      /*!< the current (the youngest result) inversive diff (from branch output to block input) */
} _cesk_method_branch_input_t;
/**
 * @brief the data structure we used for intermedian data storage
 **/
struct _cesk_method_codeblock_t{
	uint16_t init;               /*!< is this block initialized? */
	uint16_t inqueue;            /*!< is this block in queue? */ 
	const dalvik_block_t* code;  /*!< the code for this block */
	uint32_t timestamp;          /*!< when do we analyze this block last time */
	cesk_diff_t* input_diff;     /*!< the diff from previous input to current input */
	cesk_frame_t*  frame;        /*!< the result stack frame of this block */
	uint32_t ninputs;            /*!< number of inputs */
	uint32_t _idx;               /*!< an index for interal use */
	_cesk_method_branch_input_t* inputs;  /*!< the input branches */
	uint32_t* input_index;      /*!< input_index[k] is the index of struct for the k-th branch blocklist[block.code.index].inputs[block.input_index[k]]*/
};
static _cesk_method_codeblock_t _cesk_method_blocklist[CESK_METHOD_MAX_NBLOCKS];
static uint32_t _cesk_method_blockqueue[CESK_METHOD_MAX_NBLOCKS];
static uint32_t _cesk_method_max_block_idx;
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
 * @brief initialize the block list
 * @param code the block graph
 * @param input_frame the input stack frame
 * @return the result of initialization < 0 indicates an error
 **/
static inline int _cesk_method_blocklist_init(const dalvik_block_t* code, cesk_frame_t* input_frame, cesk_alloctab_t* atab)
{
	uint32_t idx = code->index;
	
	if(idx > _cesk_method_max_block_idx) 
		idx = _cesk_method_max_block_idx;

	if(_cesk_method_blocklist[idx].init) return 0;
	_cesk_method_blocklist[idx].code = code;
	
	if(NULL == (_cesk_method_blocklist[idx].input_diff = cesk_diff_empty()))
	{
		LOG_ERROR("can not allocate initial empty diff package");
		return -1;
	}
	if(NULL == (_cesk_method_blocklist[idx].frame = cesk_frame_fork(input_frame)))
	{
		LOG_ERROR("can not fork input frame");
		return -1;
	}

	cesk_frame_set_alloctab(_cesk_method_blocklist[idx].frame, atab);

	LOG_DEBUG("code block #%d is initialized", code->index);

	int i;
	for(i = 0; i < code->nbranches; i ++)
	{
		if(code->branches[i].disabled) continue;
		if(_cesk_method_blocklist_init(code->branches[i].block, input_frame, atab) < 0)
			return -1;
		_cesk_method_blocklist[idx].ninputs ++;
	}
	/* allocate the index block array */
	_cesk_method_blocklist[idx].input_index = (uint32_t*)malloc(sizeof(uint32_t) * code->nbranches);
	if(NULL == _cesk_method_blocklist[idx].input_index)
	{
		LOG_ERROR("can not allocate input index array");
		return -1;
	}
	return 0;
}
/**
 * @brief initialize the branches 
 * @return result of initialization < 0 indocates error 
 **/
static inline int _cesk_method_branches_init()
{
	int i, j;
	for(i = 0; i <= _cesk_method_max_block_idx; i ++)
	{
		_cesk_method_codeblock_t* block = _cesk_method_blocklist + i;
		/* skip this is a unintialzed slot*/
		if(NULL == block->code) continue;
		uint32_t n = block->ninputs;
		_cesk_method_branch_input_t* inputs = (_cesk_method_branch_input_t*)malloc(sizeof(_cesk_method_branch_input_t) * n);
		if(NULL == inputs)
		{
			LOG_ERROR("can not allocate an input branch array with %d element for block #%d", n, block->code->index);
			return -1;
		}
		block->inputs = inputs;
		for(j = 0; j < n; j ++)
		{
			inputs[j].code = NULL;
			if(NULL == (inputs[j].prv_inversion = cesk_diff_empty()))
			{
				LOG_ERROR("can not allocate initializal value for the block input struct");
				return -1;
			}
			if(NULL == (inputs[j].cur_diff = cesk_diff_empty()))
			{
				LOG_ERROR("can not allocate initializal value for the block input struct");
				return -1;
			}
			if(NULL == (inputs[j].cur_inversion = cesk_diff_empty()))
			{
				LOG_ERROR("can not allocate initializal value for the block input struct");
				return -1;
			}
		}
	}
	/* the second pass, set up the code block info and the input_index array */
	for(i = 0; i < _cesk_method_max_block_idx; i ++)
	{
		_cesk_method_codeblock_t* block = _cesk_method_blocklist + i;
		if(NULL == block->code) continue;
		uint32_t n = block->code->nbranches;
		for(j = 0; j < n; j ++)
		{
			if(block->code->branches[j].disabled) continue;
			uint32_t target = block->code->branches[j].block->index;
			uint32_t idx = _cesk_method_blocklist[target]._idx ++;
			_cesk_method_blocklist[target].inputs[idx].code = block;
			block->input_index[j] = idx;
		}
	}
	return 0;
}
cesk_diff_t* cesk_method_analyze(const dalvik_block_t* code, cesk_frame_t* frame)
{
	LOG_DEBUG("start analyzing code block graph at %p with frame %p (hashcode = %u)", code, frame, cesk_frame_hashcode(frame));
	/* first we try to find in the cache for this state */
	_cesk_method_cache_node_t* node = _cesk_method_cache_find(code, frame);
	if(NULL != node)
	{
		LOG_DEBUG("ya, there's an node is actually about this state, there's no need to look at this method");
		if(NULL == node->result)
		{
			LOG_DEBUG("oh? I've ever seen this before, it's a trap. I won't go inside");
			return cesk_diff_empty();
		}
		else
		{
			LOG_DEBUG("we found we have previously done that!");
			return cesk_diff_fork(node->result);
		}
	}
	
	/* insert current state to the cache, tell others I've ever been here */
	node = _cesk_method_cache_insert(code, frame);

	/* not found, ok, we analyze it */
	int i;
	
	/* set up the helper struct for each block */

	cesk_alloctab_t* atab;
	cesk_reloc_table_t* rtab;

	atab = cesk_alloctab_new();
	if(NULL == atab)
	{
		LOG_ERROR("can not create allocation table for the blocks");
		goto ERR;
	}

	rtab = cesk_reloc_table_new();
	if(NULL == rtab)
	{
		LOG_ERROR("can not create relocated address table for method");
		goto ERR;
	}

	/* now we call the initialization functions */

	memset(_cesk_method_blocklist, 0, sizeof(_cesk_method_blocklist));
	_cesk_method_max_block_idx = 0;

	if(_cesk_method_blocklist_init(code, frame, atab) < 0)
	{
		LOG_ERROR("can not intialize the block list used by method analyzer");
		goto ERR;
	}
	if(_cesk_method_branches_init() < 0)
	{
		LOG_ERROR("can not initialize the branch structs for method analyzer");
		goto ERR;
	}
	int front = 0, rear = 1;
	_cesk_method_blockqueue[0] = code->index;
	while(front < rear)
	{
		int i;
		/* current block to analyze */
		int current = _cesk_method_blockqueue[front];
		/* for each branch */
		for(i = 0; i < _cesk_method_blocklist[current].code->nbranches; i ++)
		{
			const dalvik_block_branch_t* branch = _cesk_method_blocklist[current].code->branches + i;
			/* if this branch is disabled, ignore it */
			if(branch->disabled) continue;
			/* if this is a unconditional branch */
			if(branch->conditional == 0)
			{
				/* if this is a simple jump */
				if(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_JUMP(*branch))
				{

				}
				
				/* TODO if it's a exception branch */

				/* But we do not need to handle return here, because this branch
				 * has no effect on the value of any block inside this method, 
				 * so what we should only do is after all analyze is done, accurding 
				 * this branch to construct a result diff */
			}
			/* a conditional branch */
			else 
			{

			}
		}
	}
	return 0;
ERR:
	for(i = 0; i < CESK_METHOD_MAX_NBLOCKS; i ++)
	{
		if(NULL != _cesk_method_blocklist[i].input_diff) cesk_diff_free(_cesk_method_blocklist[i].input_diff);
		if(NULL != _cesk_method_blocklist[i].inputs)
		{
			int j;
			for(j = 0; j < _cesk_method_blocklist[i].ninputs; j ++)
			{
				_cesk_method_branch_input_t* input = _cesk_method_blocklist[i].inputs + j;
				if(NULL != input->prv_inversion) cesk_diff_free(input->prv_inversion);
				if(NULL != input->cur_diff) cesk_diff_free(input->cur_diff);
				if(NULL != input->cur_inversion) cesk_diff_free(input->cur_inversion);
			}
		}
		if(NULL != _cesk_method_blocklist[i].input_index) free(_cesk_method_blocklist[i].input_index);
		if(NULL != _cesk_method_blocklist[i].frame) cesk_frame_free(_cesk_method_blocklist[i].frame);
	}
	if(NULL != atab) cesk_alloctab_free(atab);
	if(NULL != rtab) cesk_reloc_table_free(rtab);
	return NULL;
}
