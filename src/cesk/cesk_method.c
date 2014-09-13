#include <cesk/cesk_method.h>
/* types */

/**
 * @brief node in cache , use [block, frame] as key
 **/
typedef struct _cesk_method_cache_node_t{
	const dalvik_block_t* code;  /*!< the code block */
	cesk_frame_t* frame;          /*!< the stack frame */
	cesk_diff_t* result;          /*!< the analyze result */
	cesk_reloc_table_t* rtable;   /*!< the relocation table */
	struct _cesk_method_cache_node_t* next;  /*!< next pointer for the hash table */
} _cesk_method_cache_node_t;

typedef struct _cesk_method_block_context_t _cesk_method_block_context_t;
/**
 * @brief the branch information, because we consider the value that might
 *        flow into conditional branch. Although the input and the code
 *        are the same, different branch might lead different input because of the 
 *        branch constrains. So that we need this struct to store the branch information
 **/
typedef struct {
	uint32_t index:31;                  /*!< the index of the branch in the code block */
	uint8_t  visited:1;                 /*!< Visited before? */
	_cesk_method_block_context_t* block;  /*!< the code block struct that produces this input, if NULL this is a return branch */
	cesk_frame_t* frame;             /*!< the result stack frame of this block in this branch*/
	cesk_diff_t* prv_inversion;      /*!< the previous (second youngest result) inversive diff (from branch output to block input) */
	cesk_diff_t* cur_diff;           /*!< the current (the youngest result) execution diff (from block input to branch output) */
	cesk_diff_t* cur_inversion;      /*!< the current (the youngest result) inversive diff (from branch output to block input) */
} _cesk_method_block_input_t;
/**
 * @brief the data structure we used for block context data storage
 **/
struct _cesk_method_block_context_t{
	const dalvik_block_t* code;  /*!< the code for this block */
	uint32_t timestamp;          /*!< when do we analyze this block last time */
	uint32_t queue_position;     /*!< the position in the queue */
	cesk_diff_t* input_diff;     /*!< the diff from previous input to current input */
	uint32_t ninputs;            /*!< number of inputs */
	_cesk_method_block_input_t* inputs;  /*!< the input branches */
	cesk_diff_t* result_diff;       /*!< if the block is a return block, this field will be used instead of input_index, becuase return is the only branch*/
	/* input_index[k] actually means the index of input of target blocks for the k-th output of current block */
	uint32_t* input_index;      /*!< input_index[k] is the index of struct for the k-th branch blocklist[block.code.index].inputs[block.input_index[k]]*/
};
/**
 * @brief an analyzer context 
 **/
typedef struct _cesk_method_context_t{
	uint32_t tick;
	const cesk_frame_t* input_frame;     /*!< the input frame for this context */
	uint32_t Q[CESK_METHOD_MAX_NBLOCKS]; /*!< the analyzer queue */ 
	uint32_t front;                      /*!< the earlist timestamp in the queue */
	uint32_t rear;                       /*!< next fresh timestamp */
	cesk_reloc_table_t* rtable;          /*!< relocation table*/
	cesk_alloctab_t*    atable;          /*!< allocation table*/
	uint32_t nslots;                     /*!< the number of slots */
	cesk_diff_buffer_t* result_buffer;   /*!< the result diff buffer */
	const struct _cesk_method_context_t* caller;      /*!< the caller context */
	_cesk_method_block_context_t blocks[0];  /*!< the block contexts */
} _cesk_method_context_t;
CONST_ASSERTION_LAST(_cesk_method_context_t, blocks);

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
/**
 * @brief how many input slots are used 
 **/
static int _cesk_method_block_inputs_used[CESK_METHOD_MAX_NBLOCKS];

/**
 * @brief a pointer to hold the empty diff, at least one refcount
 **/
static cesk_diff_t *_cesk_method_empty_diff = NULL;

int cesk_method_init()
{
	memset(_cesk_method_cache, 0, sizeof(_cesk_method_cache));
	_cesk_method_empty_diff = cesk_diff_empty();
	return 0;
}
void cesk_method_clean_cache()
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
			if(current->rtable) cesk_reloc_table_free(current->rtable);
			free(current);
		}
		_cesk_method_cache[i] = NULL;
	}
}
void cesk_method_finalize()
{
	cesk_method_clean_cache();
	if(NULL != _cesk_method_empty_diff) cesk_diff_free(_cesk_method_empty_diff);
}
/**
 * @brief the hash code used by the method analyzer cache, the key is the code block and current stack frame
 * @param code current code block
 * @param frame current stack frame
 * @return the hashcode computed from the input key pair
 **/
static inline hashval_t _cesk_method_cache_hash(const dalvik_block_t* code, const cesk_frame_t* frame)
{
	return (((((uintptr_t)code)&0xffffffffu) * (((uintptr_t)code)&0xffffffffu)) + 
		   0x35fbc27 * (((uint64_t)((uintptr_t)code))>>32)) ^  /* for 32 bit machine, this part is 0 */
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
	/* we can not make modifications in the input frame, so we need to fork the frame before we start */
	ret->frame = cesk_frame_fork(frame);
	ret->code = code;
	ret->result = NULL;
	ret->next = NULL;
	ret->rtable = NULL;
	return ret;
}
/**
 * @brief insert a new node into the method analyzer cache
 * @note in this function we assume that there's no duplicate node in the table. And the caller
 *       should guareentee this
 * @param code current code block
 * @param frame current stack frame
 * @return the pointer to the newly inserted node, NULL indicates there's some error
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
		/* each piece of code is a signleton in the memory that is why we just compare the address */
		if(node->code == code && cesk_frame_equal(frame, node->frame))  
			return node;
	return NULL;
}

/** 
 * @brief explore the code block graph and save the pointer to all blocks in
 *        _cesk_method_block_list, so that we can prepare the block context for this
 *        function. 
 * @param entry the entry pointer
 * @todo  for perforamnce reason, this funcution should also been cached to accelereate the recursive 
 *        function calls
 * @return < 0 indicates an error
 **/
static inline int _cesk_method_explore_code(const dalvik_block_t* entry)
{  
	if(NULL == entry) return 0;
	LOG_DEBUG("found block #%d", entry->index);
	_cesk_method_block_list[entry->index] = entry;
	/* update the maximum block index for current function */ 
	if(_cesk_method_block_max_idx < entry->index || _cesk_method_block_max_idx == 0xffffffffu)
		_cesk_method_block_max_idx = entry->index;
	int i;
	for(i = 0; i < entry->nbranches; i ++)
	{
		const dalvik_block_branch_t* branch = entry->branches + i;
		if(branch->disabled ||    /* a disabled branch ? */
		  (0 == branch->conditional && DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(*branch))) /* a return branch, which means no target block */
			continue;
		_cesk_method_block_ninputs[branch->block->index] ++;   /* update the number of inputs the target block depends on */
		if(NULL != _cesk_method_block_list[branch->block->index]) continue;   /* we have visit the target block before? ok skip it */
		if(_cesk_method_explore_code(branch->block) < 0)
		{
			LOG_ERROR("can not explore the code blocks from block #%d", branch->block->index);
			return -1;
		}
	}
	return 0;
}
/**
 * @brief clean up a method analyzer context
 * @param context the context to be freed
 * @return nothing
 **/
static inline void _cesk_method_context_free(_cesk_method_context_t* context)
{
	if(NULL == context) return;
	int b;
	/* first, we clean up each block one by one */
	for(b = 0; b < context->nslots; b ++)
	{
		_cesk_method_block_context_t* block = context->blocks + b;
		if(NULL == block->code) continue;
		if(NULL != block->input_diff) cesk_diff_free(block->input_diff);
		if(NULL != block->input_index) free(block->input_index);
		if(NULL != block->result_diff) cesk_diff_free(block->result_diff);
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
	/* rtable will be freed by caller */
	//if(NULL != context->rtable) cesk_reloc_table_free(context->rtable);
	if(NULL != context->atable) cesk_alloctab_free(context->atable);
	if(NULL != context->result_buffer) cesk_diff_buffer_free(context->result_buffer);
	free(context);
}
/**
 * @brief initialize the method analyzer
 * @param entry the entry point of this method
 * @param frame the stack frame
 * @param caller the caller context
 * @return the analyzer context
 **/
static inline _cesk_method_context_t* _cesk_method_context_new(
		const dalvik_block_t* entry, 
		const cesk_frame_t* frame, 
		const _cesk_method_context_t* caller)
{
	static uint32_t tick = 0;
	_cesk_method_context_t* ret;
	_cesk_method_block_max_ninputs = 0;
	_cesk_method_block_max_idx = 0xfffffffful;
	memset(_cesk_method_block_list, 0, sizeof(_cesk_method_block_list));
	memset(_cesk_method_block_ninputs, 0, sizeof(_cesk_method_block_ninputs));
	memset(_cesk_method_block_inputs_used, 0, sizeof(_cesk_method_block_inputs_used));
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
	ret->tick = tick ++;
	/* set the input frame */
	ret->input_frame = frame;
	/* create allocation table and relocation table */
	ret->rtable = cesk_reloc_table_new();
	if(NULL == ret->rtable)
	{
		LOG_ERROR("invalid relocation table");
		goto ERR;
	}
	ret->caller = caller;
	ret->atable = cesk_alloctab_new(caller==NULL?NULL:caller->atable);
	if(NULL == ret->atable)
	{
		LOG_ERROR("can not create new allocation table");
		goto ERR;
	}
	
	/* then we allocate the result diff buffer */
	ret->result_buffer = cesk_diff_buffer_new(0, 1);
	if(NULL == ret->result_buffer)
	{
		LOG_ERROR("can not create new diff buffer for analysis result");
		goto ERR;
	}
	/* initialize all context for every blocks */
	int i;
	for(i = 0; i < _cesk_method_block_max_idx + 1; i ++)
	{
		/* skip block not reachable */
		if(NULL == _cesk_method_block_list[i]) continue;
		/* initialize the i-th code block */
		ret->blocks[i].code = _cesk_method_block_list[i];
		/* if there's an output, we should allocate the input_index for this block */
		if(_cesk_method_block_list[i]->nbranches > 1 || !DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(_cesk_method_block_list[i]->branches[0]))
		{
			ret->blocks[i].input_index = (uint32_t*)malloc(ret->blocks[i].code->nbranches * sizeof(uint32_t));
			if(NULL == ret->blocks[i].input_index)
			{
				LOG_ERROR("can not allocate input index array for block #%d", i);
				goto ERR;
			}
			ret->blocks[i].result_diff = NULL;
		}
		/* otherwise we do not need to do so */
		else
		{
			/* even though the return statement is return-void, the value can be passed to the caller by
			 * static member or exceptions. Therefore, we need result diff for any return function */
			ret->blocks[i].result_diff = cesk_diff_empty();
			ret->blocks[i].input_index = NULL;
		}
		/* the initial input diff for this block is an empty diff */
		ret->blocks[i].input_diff = cesk_diff_empty();
		if(NULL == ret->blocks[i].input_diff)
		{
			LOG_ERROR("can not create an empty diff for intial input diff");
			goto ERR;
		}
		/* initialize each branch that starts from this block */
		int j;
		for(j = 0; j < ret->blocks[i].code->nbranches; j ++)
		{
			/* the branch description */
			const dalvik_block_branch_t* branch = ret->blocks[i].code->branches + j;
			/* it's disabled or do not outputing anything? there's no need to initialize for this branch */
			if(branch->disabled || DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(*branch))
				continue;
			//ret->blocks[i].input_index[j] = 0;
			/* the target index */
			uint32_t t = branch->block->index;
			/* if the input context array is not initialized, allocate it */
			if(NULL == ret->blocks[t].inputs)
			{
				size_t inputs_size;
				ret->blocks[t].ninputs = _cesk_method_block_ninputs[t];
				inputs_size = sizeof(_cesk_method_block_input_t) * ret->blocks[t].ninputs;
				ret->blocks[t].inputs = (_cesk_method_block_input_t*)malloc(inputs_size);
				if(NULL == ret->blocks[t].inputs)
				{
					LOG_ERROR("can not allocate input structs for block #%d", i);
					goto ERR;
				}
				memset(ret->blocks[t].inputs, 0, inputs_size);
			}
			/* append this branch to the input table */
			/* so why inputs[0]? because for each time we append one input in the block context,
			 * we keep moving the input pointer to the next empty slot that is avaliable for the 
			 * future input branches. */
			ret->blocks[t].inputs[0].block = ret->blocks + i;
			ret->blocks[t].inputs[0].index = j;
			ret->blocks[t].inputs[0].frame = cesk_frame_fork(frame);
			if(NULL == ret->blocks[t].inputs[0].frame)
			{
				LOG_ERROR("failed to duplicate the input frame");
				goto ERR;
			}
			/* share the allocation table among all blocks in this method invocation context */
			cesk_frame_set_alloctab(ret->blocks[t].inputs[0].frame, ret->atable);
			ret->blocks[t].inputs[0].prv_inversion = cesk_diff_empty();
			if(NULL == ret->blocks[t].inputs[0].prv_inversion)
			{
				goto DIFFERR;
			}
			ret->blocks[t].inputs[0].cur_inversion = cesk_diff_empty();
			if(NULL == ret->blocks[t].inputs[0].cur_inversion)
			{
				goto DIFFERR;
			}
			ret->blocks[t].inputs[0].cur_diff = cesk_diff_empty();
			if(NULL == ret->blocks[t].inputs[0].cur_diff)
			{
				goto DIFFERR;
			}
			/* now we update the input_index array */
			ret->blocks[i].input_index[j] = _cesk_method_block_inputs_used[t] ++;
			/* this is why we use inputs[0] */
			ret->blocks[t].inputs ++;
		}
	}
	/* restore the input pointer */
	for(i = 0; i < _cesk_method_block_max_idx + 1; i ++)
	{
		if(NULL == _cesk_method_block_list[i]) continue;
		ret->blocks[i].inputs -= ret->blocks[i].ninputs;
	}
	return ret;
DIFFERR:
	LOG_ERROR("failed to initialize diffs");
ERR:
	if(NULL != ret && NULL != ret->rtable) cesk_reloc_table_free(ret->rtable);
	if(NULL != ret) _cesk_method_context_free(ret);
	return NULL;
}
/**
 * @brief compute the next input for this branch, update the input context and apply the diff to frame
 * @details this function will merge the content of input frame
 * @param ctx the current context
 * @param blkctx the current block context
 * @return < 0 error
 **/
static inline int _cesk_method_compute_next_input(_cesk_method_context_t* ctx, _cesk_method_block_context_t* blkctx)
{
	int i;
	LOG_DEBUG("computing the input frame for block #%d", blkctx->code->index);
	/* for the multiple way diff, we merge use the factorize function, so we need diff in each way,
	 * and the output frame applied each diff */
	uint32_t nways = 0;    /* how many ways of diff needs to be merged */
	cesk_diff_t* term_diff[CESK_METHOD_MAX_NBLOCKS];
	const cesk_frame_t* term_frame[CESK_METHOD_MAX_NBLOCKS];
	for(i = 0; i < blkctx->ninputs; i ++)
	{
		_cesk_method_block_input_t*   input = blkctx->inputs + i;                    /* current input */
		_cesk_method_block_context_t* sourctx = ctx->blocks + input->block->code->index;   /* the context of the source block */
		/* if there's no modification since the last computation */
		if(blkctx->timestamp > sourctx->timestamp)
		{
			LOG_DEBUG("this block uses the input from block #%d, but there's no modification on the block, so skipping", sourctx->code->index);
			/* we actually needs an empty diff for this, because there's no modification */
			term_frame[nways] = input->frame;
			term_diff[nways] = cesk_diff_empty();
			if(NULL == term_diff[nways])
			{
				LOG_ERROR("can not allocate a new indentity diff");
				goto ERR;
			}
			nways ++;
			continue;
		}
		/* so this is the input that has been modified since last intepration of current block, we need to compute the output diff for that */
		
		/* output_diff = prv_inversion * input_diff * cur_diff */
		cesk_diff_t* factors[] = {input->prv_inversion, sourctx->input_diff, input->cur_diff};
		term_diff[nways] = cesk_diff_apply(3, factors);
		if(NULL == term_diff[nways]) 
		{
			LOG_ERROR("can not compute prv_inversion * input_diff * cur_diff");
			goto ERR;
		}
		term_frame[nways] = input->frame;
		nways ++;
	}
	/* so let factorize */
	cesk_diff_t* result = cesk_diff_factorize(nways, term_diff, term_frame);
	if(NULL == result)
	{
		LOG_ERROR("can not factorize");
		goto ERR;
	}
	/* clean up */
	for(i = 0; i < nways; i ++)
		cesk_diff_free(term_diff[i]);
	
	if(cesk_diff_sub(result, blkctx->input_diff) < 0)
	{
		LOG_ERROR("can not strip the output diff");
		goto ERR;
	}

	cesk_diff_free(blkctx->input_diff);
	blkctx->input_diff = result;
	blkctx->timestamp = ctx->rear;
	return 0;
ERR:
	for(i = 0; i < nways; i ++)
	{
		if(NULL != term_diff) cesk_diff_free(term_diff[i]);
	}
	return -1;
}
/**
 * @brief using the branch condition information, generate a diff that reflect the constains of this branch
 * @param block_ctx the context of the input block
 * @param input_ctx the input context
 * @param diff_buf buffer to pass the diff to caller
 * @param inv_buf buffer to pass the inversion of the diff to caller
 * @return result of the opration < 0 indicates some errors
 **/
static inline int _cesk_method_get_branch_constrain_diff(
		_cesk_method_block_context_t* block_ctx, 
		_cesk_method_block_input_t* input_ctx, 
		cesk_diff_t** diff_buf,
		cesk_diff_t** inv_buf)
{
	//TODO
	*diff_buf = cesk_diff_empty();
	*inv_buf = cesk_diff_empty();
	return 0;
}
/**
 * @brief compute the input frame of a given branch
 * @param context the analyzer context
 * @param input_ctx the input context
 * @param b_diff the branch constrain diff
 * @return > 0 means there's some modification, so that we need to go on. = 0 means 
 *         the computation is successful, but nothing has changed, so that we can 
 *         skip this status. < 0 indicates something goes wrong
 **/
static inline int _cesk_method_compute_branch_frame(_cesk_method_context_t* context, _cesk_method_block_input_t* input_ctx, cesk_diff_t* b_diff)
{
	/* then compute the actual diff to generate a correct input frame, so that we can call the block analyzer 
	 * ((current_output * current_inversion) * input_diff) * branch_diff ==> 
	 * (current_input * input_diff) * branch_diff ==>
	 * next_input * branch_diff ==>
	 * branch_input */

	cesk_diff_t* diffbuf[] = {input_ctx->cur_inversion, input_ctx->block->input_diff, b_diff};
	cesk_diff_t* branch_diff = cesk_diff_apply(3, diffbuf);
	if(NULL == branch_diff)
	{
		LOG_ERROR("can not compute the branch input");
		return -1;
	}
	int rc;
	if((rc = cesk_frame_apply_diff(input_ctx->frame, branch_diff, context->rtable, NULL, NULL)) < 0)
	{
		LOG_ERROR("can not apply branch input diff to the branch frame");
		cesk_diff_free(branch_diff);
		return -1;
	}
	/* at this point we have to compute the modified object number against current_input, rather than current_output. although the output frame is lost after
	 * we applied the delta to it, but fortunately the cur_diff has saved a snapshot of current_output frame that contains all value that is intersted to us */
	rc = cesk_diff_correct_modified_object_number(input_ctx->cur_inversion, input_ctx->cur_diff, input_ctx->frame, rc);
	if(rc < 0)
	{
		LOG_ERROR("can not compute the number of modified object agains current_input (aka cururent_output * cur_inservion )");
		cesk_diff_free(branch_diff);
		return -1;
	}
	cesk_diff_free(branch_diff);
	return rc;
}
/**
 * @brief when we meet a branch with a return instruction, the analyzer calls this function.
 *        This function will forward the return value to a diff buffer, based on this, we can
 *        construct a return diff after the analysis for this closure is done.
 * @note  the store and result register won't be override, we need to merge all values that to 
 *        be forwarded. (in register section only result register and exception is allowed
 * @param context the context for analyzer
 * @param branch the block branch structure describing this branch
 * @param input_diff the diff of previous input and current input
 * @return the result of this operation < 0 indicates an error
 */
static inline int _cesk_method_return(
		_cesk_method_context_t* context, 
		const dalvik_block_branch_t* branch, 
		const cesk_diff_t* input_diff)
{
	uint32_t regid;
	uint32_t j;

	/* TODO a linear search is waste of time, so we can replicate it using a binary search */

	if(branch->left->header.info.type != DVM_OPERAND_TYPE_VOID)
		regid = CESK_FRAME_GENERAL_REG(branch->left->payload.uint16);
	else 
		regid = CESK_STORE_ADDR_NULL;
	/* find the register to return && forward static fields */
	
	for(j = input_diff->offset[CESK_DIFF_REG]; j < input_diff->offset[CESK_DIFF_REG + 1]; j ++)
	{
		/* if this record stands for static field changes, forward it */
		if(CESK_FRAME_REG_IS_STATIC(input_diff->data[j].addr))
		{
			if(cesk_diff_buffer_append(context->result_buffer, CESK_DIFF_REG, input_diff->data[j].addr, input_diff->data[j].arg.set) < 0)
			{
				LOG_ERROR("can not forward static field #%u", CESK_FRAME_REG_STATIC_IDX(input_diff->data[j].addr));
				return -1;
			}
			LOG_DEBUG("static field #%u has been forwarded", CESK_FRAME_REG_STATIC_IDX(input_diff->data[j].addr));
		}
		/* if we find the result bearing register and this return instruction is not return-void, set the return code */
		if(input_diff->data[j].addr == regid && branch->left->header.info.type != DVM_OPERAND_TYPE_VOID)
		{
			if(cesk_diff_buffer_append(context->result_buffer, CESK_DIFF_REG, CESK_FRAME_RESULT_REG, input_diff->data[j].arg.set) < 0)
			{
				LOG_ERROR("can not append the new value to diff buffer");
				return -1;
			}
		}
	}
	
	/* forward the allocations */
	uint32_t i;
	for(i = input_diff->offset[CESK_DIFF_ALLOC]; i < input_diff->offset[CESK_DIFF_ALLOC + 1]; i ++)
	{
		if(cesk_diff_buffer_append(context->result_buffer, CESK_DIFF_ALLOC, input_diff->data[i].addr, input_diff->data[i].arg.generic) < 0)
		{
			LOG_ERROR("can not append the allocation record the the result buffer");
			return -1;
		}
	}
	/* forward the reuse flags */
	for(i = input_diff->offset[CESK_DIFF_REUSE]; i < input_diff->offset[CESK_DIFF_REUSE + 1]; i ++)
	{
		if(cesk_diff_buffer_append(context->result_buffer, CESK_DIFF_REUSE, input_diff->data[i].addr, input_diff->data[i].arg.generic) < 0)
		{
			LOG_ERROR("can not append the reuse bit record to the result buffer");
			return -1;
		}
	}
	/* forward the store modification */
	for(i = input_diff->offset[CESK_DIFF_STORE]; i < input_diff->offset[CESK_DIFF_STORE + 1]; i ++)
	{
		if(cesk_diff_buffer_append(context->result_buffer, CESK_DIFF_STORE, input_diff->data[i].addr, input_diff->data[i].arg.generic) < 0)
		{
			LOG_ERROR("can not append the store modification record to the result buffer");
			return -1;
		}
	}
	/* ok everything is done */
	return 0;
}
/* TODO: exception return */
cesk_diff_t* cesk_method_analyze(const dalvik_block_t* code, cesk_frame_t* frame, const void* caller, cesk_reloc_table_t** p_rtab)
{
	LOG_DEBUG("start analyzing code block graph at %p with frame %p (hashcode = %u)", code, frame, cesk_frame_hashcode(frame));
	/* first we try to find in the cache for this state */
	_cesk_method_cache_node_t* node = _cesk_method_cache_find(code, frame);
	if(NULL != node)
	{
		LOG_DEBUG("ya, there's an node is actually about this invocation context, there's no need to look at this method");
		if(NULL == node->result)
		{
			LOG_DEBUG("oh? This context won't return, it's a trap. I don't wanna go inside it");
			*p_rtab = NULL;
			return cesk_diff_empty();
		}
		else
		{
			LOG_DEBUG("I find I did previous work on this invocation context!");
			*p_rtab = node->rtable;
			return cesk_diff_fork(node->result);
		}
	}
	
	cesk_diff_t* result = NULL;
	_cesk_method_context_t* context = NULL;
	
	/* insert current node to the cache, tell others I've ever been here */
	node = _cesk_method_cache_insert(code, frame);
	if(NULL == node)
	{
		LOG_ERROR("can not allocate a new node in method analyzer cache");
		goto ERR;
	}

	context = _cesk_method_context_new(code, frame, (const _cesk_method_context_t*)caller);
	if(NULL == context)
	{
		LOG_ERROR("can not create context");
		goto ERR;
	}

	cesk_method_print_backtrace(context);
	
	context->front = 0, context->rear = 1;
	context->Q[0] = code->index;
	/* TODO: exception */
	
	while(context->front < context->rear)
	{
		/* current block context */
		_cesk_method_block_context_t *blkctx = context->blocks + context->Q[(context->front++)%CESK_METHOD_MAX_NBLOCKS];
		LOG_DEBUG("current block : block #%d", blkctx->code->index);
#if LOG_LEVEL >= 6
		LOG_DEBUG("========block info==========");
		LOG_DEBUG("code");
		uint32_t insidx;
		for(insidx = blkctx->code->begin; insidx < blkctx->code->end; insidx ++)
			LOG_DEBUG("\t0x%x\t%s", insidx, dalvik_instruction_to_string(dalvik_instruction_get(insidx), NULL, 0));
		LOG_DEBUG("========end info============");
#endif
		if(_cesk_method_compute_next_input(context, blkctx) < 0) 
		{
			LOG_ERROR("can not compute the next input");
			goto ERR;
		}
		LOG_DEBUG("Block input diff: %s", cesk_diff_to_string(blkctx->input_diff, NULL, 0));
		/* then we compute the new output for each branch */
		int i;
		for(i = 0; i < blkctx->code->nbranches; i ++)
		{
			if(blkctx->code->branches[i].disabled) continue;
			/* if current branch is return */
			if(0 == blkctx->code->branches[i].conditional && DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(blkctx->code->branches[i]))
			{
				if(NULL == blkctx->result_diff) continue;
				cesk_diff_t* tmp[] = {blkctx->result_diff, blkctx->input_diff};
				cesk_diff_t* new_res = cesk_diff_apply(2, tmp);
				if(NULL == new_res)
				{
					LOG_ERROR("can not merge new input_diff to the result");
					goto ERR;
				}
				cesk_diff_free(blkctx->result_diff);
				blkctx->result_diff = new_res;
				continue;
			}
			_cesk_method_block_context_t* target_ctx = context->blocks + blkctx->code->branches[i].block->index;
			_cesk_method_block_input_t*   input_ctx  = target_ctx->inputs + blkctx->input_index[i];
			/* first of all, apply the branch constrains to the frame */
			cesk_diff_t *b_diff = NULL;
			cesk_diff_t *b_inv  = NULL;
			
			/* let's think about the branch constain */
			if(_cesk_method_get_branch_constrain_diff(blkctx, input_ctx, &b_diff, &b_inv) < 0)
			{
				LOG_ERROR("can not compute the branch diff");
				goto ERR;
			}

			/* then we should compute the actual input frame for this branch before we strat run the code in this block 
			 * if nothing has been changed, it's a fix point*/
			if(0 == _cesk_method_compute_branch_frame(context, input_ctx, b_diff) && input_ctx->visited)
			{
				LOG_DEBUG("there's no modification in this branch, so we skip");
				cesk_diff_free(b_diff);
				cesk_diff_free(b_inv);
				continue;
			}

			input_ctx->visited = 1;

			LOG_DEBUG("=============Input frame=================");
			cesk_frame_print_debug(input_ctx->frame);
			LOG_DEBUG("=============End of Frame================");
			/* then we can run the code */
			cesk_block_result_t res;
			if(cesk_block_analyze(blkctx->code, input_ctx->frame, context->rtable, &res, context) < 0)
			{
				LOG_ERROR("failed to analyze the block #%d with frame hash = 0x%x",  blkctx->code->index, cesk_frame_hashcode(input_ctx->frame));
				goto ERR;
			}
			LOG_DEBUG("=============Result frame=================");
			cesk_frame_print_debug(input_ctx->frame);
			LOG_DEBUG("=============End of Frame================");

			/* compute new branch analysis diff */
			cesk_diff_t* buf[] = {b_diff, res.diff};
			cesk_diff_t* ibuf[] = {res.inverse, b_inv};
			cesk_diff_t* cur_diff = cesk_diff_apply(2, buf);
			cesk_diff_t* cur_inversion = cesk_diff_apply(2, ibuf);
			if(NULL == cur_diff || NULL == cur_inversion)
			{
				LOG_ERROR("failed to compute the analysis diff after interpretation of this branch");
				goto ERR;
			}
			/* update the diff */
			cesk_diff_free(input_ctx->prv_inversion);
			cesk_diff_free(input_ctx->cur_diff);
			input_ctx->prv_inversion = input_ctx->cur_inversion;
			input_ctx->cur_diff = cur_diff;
			input_ctx->cur_inversion = cur_inversion;
			LOG_DEBUG("prv_insersion = %s", cesk_diff_to_string(input_ctx->prv_inversion, NULL, 0));
			LOG_DEBUG("cur_insersion = %s", cesk_diff_to_string(input_ctx->cur_inversion, NULL, 0));
			LOG_DEBUG("cur_diff = %s", cesk_diff_to_string(input_ctx->cur_diff, NULL, 0));
			/* finally, update the queue and timestamp */
			uint32_t target_qp = target_ctx->queue_position;
			/* if the target context is not in the queue, enqueu */
			if(target_qp < context->front)
			{
				target_ctx->queue_position = context->rear;
				context->Q[context->rear%CESK_METHOD_MAX_NBLOCKS] = target_ctx->code->index;
				context->rear ++;
			}	
			/* clean up */
			cesk_diff_free(res.diff);
			cesk_diff_free(res.inverse);
			cesk_diff_free(b_diff);
			cesk_diff_free(b_inv);
		}
	}

	/* forward the result */
	int i;
	for(i = 0; i < context->nslots; i ++)
	{
		_cesk_method_block_context_t *blkctx = context->blocks + i;
		if(NULL == blkctx->code) continue;
		if(blkctx->code->nbranches != 1 || !DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(blkctx->code->branches[0])) continue;
		if(_cesk_method_return(context,  blkctx->code->branches, blkctx->result_diff) < 0)
		{
			LOG_ERROR("can not forward the result");
		}
	}
	result = cesk_diff_from_buffer(context->result_buffer);
	if(NULL == result)
	{
		LOG_ERROR("can not build the result diff from result diff buffer");
		goto ERR;
	}

	/* cache the result diff */
	node->result = result;
	*p_rtab = node->rtable = context->rtable;
	_cesk_method_context_free(context);
	LOG_DEBUG("---------------------");
	LOG_DEBUG("Function return with diff = %s", cesk_diff_to_string(result, NULL, 0));
	LOG_DEBUG("---------------------");
	return cesk_diff_fork(result);
ERR:
	if(result) cesk_diff_free(result);
	if(context->rtable) cesk_reloc_table_free(context->rtable);
	if(context) _cesk_method_context_free(context);
	return NULL;
}
void cesk_method_print_backtrace(const void* method_context)
#if LOG_LEVEL >= 6
{
	const _cesk_method_context_t* context = (const _cesk_method_context_t*) method_context;
	LOG_DEBUG("================stack bracktrace==================");
	for(;NULL != context; context = context->caller)
	{
		uint32_t blk = context->Q[(context->front - 1)%CESK_METHOD_MAX_NBLOCKS];
		if(context->front == 0)  blk = 0;
		LOG_DEBUG("%s.%s @ block#%d(from instruction 0x%x to 0x%x)", 
				context->blocks[blk].code->info->class, 
				context->blocks[blk].code->info->method,
				blk,
				context->blocks[blk].code->begin,
				context->blocks[blk].code->end);
		LOG_DEBUG("Code");
		uint32_t insidx;
		for(insidx = context->blocks[blk].code->begin; insidx < context->blocks[blk].code->end; insidx ++)
			LOG_DEBUG("\t0x%x\t%s", insidx, dalvik_instruction_to_string(dalvik_instruction_get(insidx), NULL, 0));
		int i;
		LOG_DEBUG("Registers");
		for(i = 0; i < context->input_frame->size; i ++)
			LOG_DEBUG("\tv%d\t%s", i, cesk_set_to_string(context->input_frame->regs[i], NULL, 0));
		LOG_DEBUG("Store");
		uint32_t blkidx;
		for(blkidx = 0; blkidx < context->input_frame->store->nblocks; blkidx ++)
		{
			const cesk_store_block_t* blk = context->input_frame->store->blocks[blkidx];
			uint32_t ofs;
			for(ofs = 0; ofs < CESK_STORE_BLOCK_NSLOTS; ofs ++)
			{
				if(blk->slots[ofs].value == NULL) continue;
				uint32_t addr = blkidx * CESK_STORE_BLOCK_NSLOTS + ofs;
				uint32_t reloc_addr = cesk_alloctab_query(context->input_frame->store->alloc_tab, context->input_frame->store, addr);
				if(reloc_addr != CESK_STORE_ADDR_NULL)
					LOG_DEBUG("\t"PRSAddr"("PRSAddr")\t%s", reloc_addr, addr,cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
				else
					LOG_DEBUG("\t"PRSAddr"\t%s", addr, cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
			}
		}
		LOG_DEBUG("---------------------------------------------------");
	}
	LOG_DEBUG("==================end bracktrace====================");
}
#else
{}
#endif
const cesk_frame_t* cesk_method_context_get_input_frame(const void* context)
{
	if(NULL == context) return NULL;
	const _cesk_method_context_t *frame_context = (const _cesk_method_context_t*)context;
	return frame_context->input_frame;
}
const dalvik_block_t* cesk_method_context_get_current_block(const void* context)
{
	if(NULL == context) return NULL;
	const _cesk_method_context_t *frame_context = (const _cesk_method_context_t*)context;
	uint32_t blk = frame_context->Q[(frame_context->front - 1)%CESK_METHOD_MAX_NBLOCKS];
	if(frame_context->front == 0)  blk = 0;
	return frame_context->blocks[blk].code;
}
const void* cesk_method_context_get_caller_context(const void* context)
{
	if(NULL == context) return NULL;
	const _cesk_method_context_t *frame_context = (const _cesk_method_context_t*)context;
	return frame_context->caller;
}
