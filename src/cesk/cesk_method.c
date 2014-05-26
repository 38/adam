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
	_cesk_method_codeblock_t* code;  /*!< the code block struct that produces this input, if NULL this is a return branch */
	uint32_t index;                  /*!< the index of the branch in the code block */
	cesk_frame_t*  frame;            /*!< the result stack frame of this block in this branch*/
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
	uint32_t ninputs;            /*!< number of inputs */
	uint32_t _idx;               /*!< an index for interal use */
	_cesk_method_branch_input_t* inputs;  /*!< the input branches */
	uint32_t* input_index;      /*!< input_index[k] is the index of struct for the k-th branch blocklist[block.code.index].inputs[block.input_index[k]]*/
};
static _cesk_method_codeblock_t _cesk_method_blocklist[CESK_METHOD_MAX_NBLOCKS];
static uint32_t _cesk_method_blockqueue[CESK_METHOD_MAX_NBLOCKS];
static uint32_t _cesk_method_block_max_idx;
static uint32_t _cesk_method_block_max_ninputs;
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
 * @return the result of initialization < 0 indicates an error
 **/
static inline int _cesk_method_blocklist_init(const dalvik_block_t* code)
{
	uint32_t idx = code->index;
	
	if(idx > _cesk_method_block_max_idx) 
		idx = _cesk_method_block_max_idx;

	if(_cesk_method_blocklist[idx].init) return 0;
	_cesk_method_blocklist[idx].code = code;
	
	if(NULL == (_cesk_method_blocklist[idx].input_diff = cesk_diff_empty()))
	{
		LOG_ERROR("can not allocate initial empty diff package");
		return -1;
	}

	LOG_DEBUG("code block #%d is initialized", code->index);

	int i;
	for(i = 0; i < code->nbranches; i ++)
	{
		/* for disabled branch, just ignore */
		if(code->branches[i].disabled) continue;
		/* the return branch, the field for next block is not valid */
		if(0 == code->branches[i].conditional && DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(code->branches[i]))
			continue;
		/* otherwise, DFS it's neigbhour */
		if(_cesk_method_blocklist_init(code->branches[i].block) < 0)
			return -1;
		_cesk_method_blocklist[idx].ninputs ++;
	}
	/* update the maximum number of inputs, used when allocating memory */
	if(_cesk_method_blocklist[idx].ninputs > _cesk_method_block_max_ninputs)
		_cesk_method_block_max_ninputs = _cesk_method_blocklist[idx].ninputs;
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
 * @param input_frame the input stack frame
 * @param atab the allocation table
 * @return result of initialization < 0 indocates error 
 **/
static inline int _cesk_method_branches_init(const cesk_frame_t* frame, cesk_alloctab_t* atab)
{
	int i, j;
	for(i = 0; i <= _cesk_method_block_max_idx; i ++)
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

			if(NULL == (inputs[j].frame = cesk_frame_fork(frame)))
			{
				LOG_ERROR("can not fork input frame");
				return -1;
			}

			cesk_frame_set_alloctab(inputs[j].frame, atab);
		}
	}
	/* the second pass, set up the code block info and the input_index array */
	for(i = 0; i < _cesk_method_block_max_idx; i ++)
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
			_cesk_method_blocklist[target].inputs[idx].index = j;
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
	_cesk_method_block_max_idx = 0;
	_cesk_method_block_max_ninputs = 0;
	const cesk_frame_t** frame_buf = NULL;
	cesk_diff_t** diff_buf = NULL;

	if(_cesk_method_blocklist_init(code) < 0)
	{
		LOG_ERROR("can not intialize the block list used by method analyzer");
		goto ERR;
	}
	if(_cesk_method_branches_init(frame, atab) < 0)
	{
		LOG_ERROR("can not initialize the branch structs for method analyzer");
		goto ERR;
	}
	/* TODO lazy allocation */
	if(NULL == (frame_buf = (const cesk_frame_t**)malloc(sizeof(const cesk_frame_t*) * _cesk_method_block_max_ninputs)))
	{
		LOG_ERROR("can not allocate frame buffer");
		goto ERR;
	}
	if(NULL == (diff_buf = (cesk_diff_t**)malloc(sizeof(cesk_diff_t*) * _cesk_method_block_max_ninputs)))
	{
		LOG_ERROR("can not allocate different buffer");
		goto ERR;
	}
	/* initialize the queue */
	int front = 0, rear = 1;   /* actually, the index in the queue is the timestamp */
	_cesk_method_blockqueue[0] = code->index;
	
	/* start analyzing */
	/* TODO: how to determine wether or not the data has been modified */
	while(front < rear)
	{
		int i;
		/* current block to analyze */
		int current_idx = _cesk_method_blockqueue[front];
		_cesk_method_codeblock_t* current_blk = _cesk_method_blocklist + current_idx;
		/* first of all we compute the input */
		int nterms = 0;
		for(i = 0; i < current_blk->ninputs; i ++)
		{
			_cesk_method_branch_input_t* input_branch = current_blk->inputs + i;
			
			_cesk_method_codeblock_t* input_blk = _cesk_method_blocklist + input_branch->code->code->index;

			/* if this input block has not been modified since the last evaluation of this block, ignore it */
			if(input_blk->timestamp < current_blk->timestamp) continue;
			cesk_diff_t* tmp_inputs[3];
			tmp_inputs[0] = input_branch->prv_inversion;
			tmp_inputs[1] = input_blk->input_diff;
			tmp_inputs[2] = input_branch->cur_diff;
			cesk_diff_t* tmp_diff = cesk_diff_apply(3, tmp_inputs);
			if(NULL == tmp_diff)
			{
				LOG_ERROR("failed to compute the branch difference");
				goto ERR;
			}
			frame_buf[nterms] = input_branch->frame;
			diff_buf[nterms] = tmp_diff;
			nterms ++;
		}
		cesk_diff_t* input_diff = cesk_diff_factorize(nterms, diff_buf, frame_buf);
		for(i = 0; i < nterms; i ++)
			cesk_diff_free(diff_buf[i]);
		if(NULL == input_diff)
		{
			LOG_ERROR("failed to comput the input difference");
			goto ERR;
		}
		cesk_diff_free(current_blk->input_diff);
		current_blk->input_diff = input_diff;
		current_blk->timestamp = front;

		/* for each branch */
		for(i = 0; i < current_blk->code->nbranches; i ++)
		{
			const dalvik_block_branch_t* branch = current_blk->code->branches + i;
			/* if this branch is disabled, ignore it */
			if(branch->disabled) continue;
			/* if this is a unconditional branch */
			if(branch->conditional == 0)
			{
				/* if this is a simple jump */
				if(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_JUMP(*branch))
				{
					uint32_t target = branch->block->index;  /* the target block */
					_cesk_method_codeblock_t* tblk = _cesk_method_blocklist + target;
					_cesk_method_branch_input_t* input = tblk->inputs + current_blk->input_index[i];

					cesk_diff_t* tmp_buf[] = {input->cur_inversion, input_diff};
					cesk_diff_t* D = cesk_diff_apply(2, tmp_buf);   /* this is the diff we used to get the input */
					if(NULL == D)
					{
						LOG_ERROR("can not compute the branch input diff");
						goto ERR;
					}
					/* apply the diff */
					if(cesk_frame_apply_diff(input->frame, D, rtab) < 0)
					{
						LOG_ERROR("can not apply the diff to the buf");
						goto ERR;
					}
					cesk_block_result_t result;
					if(cesk_block_analyze(current_blk->code, input->frame, rtab, &result) < 0)
					{
						LOG_ERROR("can not execute the code block");
						goto ERR;
					}
					cesk_diff_free(input->prv_inversion);
					cesk_diff_free(input->cur_diff);
					input->prv_inversion = input->prv_inversion;
					input->cur_diff = result.diff;
					input->cur_inversion = result.inverse;
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
				// TODO a conditional branch
			}
		}
		
		front ++;
	}
	/* release the memory */
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
				if(NULL != input->frame) cesk_frame_free(input->frame);
			}
		}
		if(NULL != _cesk_method_blocklist[i].input_index) free(_cesk_method_blocklist[i].input_index);
	}
	if(NULL != atab) cesk_alloctab_free(atab);
	if(NULL != rtab) cesk_reloc_table_free(rtab);
	if(NULL != frame_buf) free(frame_buf);
	if(NULL != diff_buf) free(diff_buf);
	return NULL;
}
