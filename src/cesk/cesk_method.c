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
	uint32_t index;                  /*!< the index of the branch in the code block */
	_cesk_method_block_context_t* block;  /*!< the code block struct that produces this input, if NULL this is a return branch */
	cesk_frame_t* frame;             /*!< the result stack frame of this block in this branch*/
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
cesk_diff_t* __deb__ = NULL;

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

static cesk_diff_t *_cesk_method_empty_diff = NULL;

int cesk_method_init()
{
	memset(_cesk_method_cache, 0, sizeof(_cesk_method_cache));
	_cesk_method_empty_diff = cesk_diff_empty();
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
			free(current);
		}
	}
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
		if(node->code == code && cesk_frame_equal(frame, node->frame))
			return node;
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
	if(_cesk_method_block_max_idx < entry->index || _cesk_method_block_max_idx == 0xffffffffu)
		_cesk_method_block_max_idx = entry->index;
	int i;
	for(i = 0; i < entry->nbranches; i ++)
	{
		const dalvik_block_branch_t* branch = entry->branches + i;
		if(branch->disabled || (0 == branch->conditional && DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(*branch)))
			continue;
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
 * @param frame the stack frame
 * @return the analyzer context
 **/
static inline _cesk_method_context_t* _cesk_method_context_new(const dalvik_block_t* entry, const cesk_frame_t* frame)
{
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
		/* skip block not reachable */
		if(NULL == _cesk_method_block_list[i]) continue;
		ret->blocks[i].code = _cesk_method_block_list[i];
		ret->blocks[i].input_index = (uint32_t*)malloc(ret->blocks[i].code->nbranches * sizeof(uint32_t));
		ret->blocks[i].input_diff = cesk_diff_empty();
		if(NULL == ret->blocks[i].input_diff)
		{
			LOG_ERROR("can not create an empty diff for intial input diff");
			goto ERR;
		}
		if(NULL == ret->blocks[i].input_index)
		{
			LOG_ERROR("can not allocate input index array for block #%d", i);
			goto ERR;
		}
		int j;
		for(j = 0; j < ret->blocks[i].code->nbranches; j ++)
		{
			ret->blocks[i].input_index[j] = 0;
			const dalvik_block_branch_t* branch = ret->blocks[i].code->branches + j;
			if(branch->disabled || DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(*branch))
				continue;
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
			ret->blocks[t].inputs[0].block = ret->blocks + i;
			ret->blocks[t].inputs[0].index = j;
			ret->blocks[t].inputs[0].frame = cesk_frame_fork(frame);
			if(NULL == ret->blocks[t].inputs[0].frame)
			{
				LOG_ERROR("failed to duplicate the input frame");
				goto ERR;
			}
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
			ret->blocks[i].input_index[j] = _cesk_method_block_inputs_used[t] ++;
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
	/* for each input */
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
		
		/* actually the output_diff = prv_inversion * input_diff * cur_diff */
		cesk_diff_t* factors[] = {input->prv_inversion, sourctx->input_diff, input->cur_diff};
		term_diff[nways] = cesk_diff_apply(3, factors);
		if(NULL == term_diff[nways]) 
		{
			LOG_ERROR("can not compute prv_inversion * input_diff * cur_diff");
			goto ERR;
		}
		if(cesk_diff_sub(term_diff[nways], blkctx->input_diff) < 0)
		{
			LOG_ERROR("can not strip the output diff");
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
 * @brief utilze the branch condition information, generate a diff that is the actual input
 * @param block_ctx the context of the input block
 * @param input_ctx the input context
 * @param diff_buf buffer to pass the diff to caller
 * @param inv_buf buffer to pass the inversion of the diff to caller
 * @return result of the opration < 0 indicates some errors
 **/
static inline int _cesk_method_get_branch_input(
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

cesk_diff_t* cesk_method_analyze(const dalvik_block_t* code, cesk_frame_t* frame)
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
			return cesk_diff_empty();
		}
		else
		{
			LOG_DEBUG("I find I did previous work on this invocation context!");
			return cesk_diff_fork(node->result);
		}
	}
	
	/* insert current node to the cache, tell others I've ever been here */
	node = _cesk_method_cache_insert(code, frame);
	
	/* start analyzing */
	cesk_diff_t* result = NULL;
	_cesk_method_context_t* context = _cesk_method_context_new(code, frame);
	if(NULL == context)
	{
		LOG_ERROR("can not create context");
		goto ERR;
	}

	context->front = 0, context->rear = 1;
	context->Q[0] = code->index;
	/* build the result, things we should care about: result register, exception register and static variables */
	/* currently, we only consider the result result register */
	cesk_set_t* result_reg = NULL;
	/* TODO: exception & static support & allocation */
	
	while(context->front < context->rear)
	{
		/* current block context */
		_cesk_method_block_context_t *blkctx = context->blocks + context->Q[(context->front++)%CESK_METHOD_MAX_NBLOCKS];
		LOG_DEBUG("current block : block #%d", blkctx->code->index);
		if(_cesk_method_compute_next_input(context, blkctx) < 0) 
		{
			LOG_ERROR("can not compute the next input");
			goto ERR;
		}
		/* then we compute the new output for each branch */
		int i;
		for(i = 0; i < blkctx->code->nbranches; i ++)
		{
			if(0 == blkctx->code->branches[i].conditional && DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(blkctx->code->branches[i]))
			{
				LOG_DEBUG("return branch diff: %s", cesk_diff_to_string(blkctx->input_diff, NULL, 0));
				if(blkctx->code->branches[i].left->header.info.type != DALVIK_TYPECODE_VOID)
				{
					if(NULL == result_reg) 
					{
						result_reg = cesk_set_empty_set();
						if(NULL == result_reg) 
						{
							LOG_ERROR("can not allocate set the result register");
							goto ERR;
						}
					}
					uint32_t regid = CESK_FRAME_GENERAL_REG(blkctx->code->branches[i].left->payload.uint16);
					uint32_t j;
					for(j = blkctx->input_diff->offset[CESK_DIFF_REG]; 
						j < blkctx->input_diff->offset[CESK_DIFF_REG + 1] &&
						blkctx->input_diff->data[j].addr < regid;
						j ++);
					if(j < blkctx->input_diff->offset[CESK_DIFF_REG + 1]  && regid == blkctx->input_diff->data[j].addr)
					{
						LOG_DEBUG("return %s", cesk_set_to_string(blkctx->input_diff->data[j].arg.set, NULL, 0)); 
						if(cesk_set_merge(result_reg, blkctx->input_diff->data[j].arg.set) < 0)
							LOG_WARNING("can not merge the new return value to result");
					}
				}
				continue;
			}
			_cesk_method_block_context_t* target_ctx = context->blocks + blkctx->code->branches[i].block->index;
			_cesk_method_block_input_t*   input_ctx  = target_ctx->inputs + blkctx->input_index[i];
			if(blkctx->code->branches[i].disabled) continue;
			/* first of all, apply the branch info to the frame */
			cesk_diff_t *b_diff = NULL;
			cesk_diff_t *b_inv  = NULL;
			/* TODO: check for the fix point */
			if(_cesk_method_get_branch_input(blkctx, input_ctx, &b_diff, &b_inv) < 0)
			{
				LOG_ERROR("can not compute the branch diff");
				goto ERR;
			}
			/* then compute the actual diff to generate a correct input frame, so that we can call the block analyzer 
			 * ((current_output * current_inversion) * input_diff) * branch_diff ==> 
			 * (current_input * input_diff) * branch_diff ==>
			 * next_input * branch_diff ==>
			 * branch_input */
			cesk_diff_t* diffbuf[] = {input_ctx->cur_inversion, blkctx->input_diff, b_diff};
			cesk_diff_t* branch_diff = cesk_diff_apply(3, diffbuf);
			if(NULL == branch_diff)
			{
				LOG_ERROR("can not compute the branch input");
				goto ERR;
			}
			int rc;
			if((rc = cesk_frame_apply_diff(input_ctx->frame, branch_diff, context->rtable, NULL, NULL)) < 0)
			{
				LOG_ERROR("can not apply branch input diff to the branch frame");
				goto ERR;
			}
			cesk_diff_free(branch_diff);

			if(0 == rc && context->front  != 1)
			{
				LOG_DEBUG("there's no modification in this branch, so we skip");
				cesk_diff_free(b_diff);
				cesk_diff_free(b_inv);
				continue;
			}
			
			/* then we can run the code */
			cesk_block_result_t res;
			if(cesk_block_analyze(blkctx->code, input_ctx->frame, context->rtable, &res) < 0)
			{
				LOG_ERROR("failed to analyze the block #%d with frame hash = 0x%x",  blkctx->code->index, cesk_frame_hashcode(input_ctx->frame));
				goto ERR;
			}
			/* compute new diff */
			cesk_diff_t* buf[] = {b_diff, res.diff};
			cesk_diff_t* ibuf[] = {res.inverse, b_inv};
			cesk_diff_t* cur_diff = cesk_diff_apply(2, buf);
			if(NULL == cur_diff)
			{
				LOG_ERROR("failed to compute new diff");
				goto ERR;
			}
			cesk_diff_t* cur_inversion = cesk_diff_apply(2, ibuf);
			cesk_diff_free(res.diff);
			cesk_diff_free(res.inverse);
			cesk_diff_free(b_diff);
			cesk_diff_free(b_inv);
			if(NULL == cur_inversion)
			{
				LOG_ERROR("failed to compute the inversion");
				goto ERR;
			}
			/* finally, update the queue and timestamp */
			uint32_t target_ts = input_ctx->block->timestamp;
			input_ctx->block->timestamp = context->rear;
			if(target_ts <= context->front)
			{
				cesk_diff_free(input_ctx->prv_inversion);
				cesk_diff_free(input_ctx->cur_diff);
				input_ctx->prv_inversion = input_ctx->cur_inversion;
				input_ctx->cur_diff = cur_diff;
				input_ctx->cur_inversion = cur_inversion;
				context->Q[context->rear%CESK_METHOD_MAX_NBLOCKS] = target_ctx->code->index;
				context->rear ++;
			}	
		}
	}
	cesk_diff_buffer_t* db = cesk_diff_buffer_new(0);
	if(NULL == db) 
	{
		LOG_ERROR("can not build result diff buffer");
		goto ERR;
	}
	if(NULL != result_reg && cesk_diff_buffer_append(db, CESK_DIFF_REG, CESK_FRAME_RESULT_REG, result_reg)  < 0)
	{
		LOG_ERROR("can not append record modifying the result register");
		cesk_diff_buffer_free(db);
		goto ERR;
	}
	result = cesk_diff_from_buffer(db);
	if(NULL == result)
	{
		LOG_ERROR("can not construct the result diff");
		cesk_diff_buffer_free(db);
		goto ERR;
	}
	cesk_diff_buffer_free(db);
	db = NULL;
	cesk_set_free(result_reg);
	/* cache the result diff */
	node->result = result;
	_cesk_method_context_free(context);
	LOG_DEBUG("end of analysis result diff = %s", cesk_diff_to_string(result, NULL, 0));
	return cesk_diff_fork(result);
ERR:
	if(result) cesk_diff_free(result);
	if(result_reg) cesk_set_free(result_reg);
	if(context) _cesk_method_context_free(context);
	return NULL;
}
