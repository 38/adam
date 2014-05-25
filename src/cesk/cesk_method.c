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

/**
 * @brief the data structure we used for intermedian data storage
 **/
typedef struct {
	uint16_t init;               /*!< is this block initialized? */
	uint16_t inqueue;            /*!< is this block in queue? */ 
	const dalvik_block_t* code; /*!<the code for this block */
	uint32_t timestamp;     /*!< when do we analyze this block last time */
	cesk_diff_t* last_intp_inv;  /*!< the last interpretator diff inversion (from output to input) */
	cesk_diff_t* curr_intp_diff; /*!< the current interepator diff (from input to output) */
	cesk_diff_t* curr_intp_inv;  /*!< the current interepator diff (intput to output) */
	cesk_diff_t* input_diff;     /*!< the diff from last input to current input */
	cesk_frame_t*  frame;        /*!< the result stack frame of this block */
} _cesk_method_codeblock_t;
static _cesk_method_codeblock_t _cesk_method_blocklist[CESK_METHOD_MAX_NBLOCKS];
static uint32_t _cesk_method_blockqueue[CESK_METHOD_MAX_NBLOCKS];
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
 * @brief initialize the analyzer
 * @param code the block graph
 * @param input_frame the input stack frame
 * @return the result of initialization < 0 indicates an error
 **/
static inline int _cesk_method_analyze_init(const dalvik_block_t* code, cesk_frame_t* input_frame, cesk_alloctab_t* atab)
{
	uint32_t idx = code->index;
	if(_cesk_method_blocklist[idx].init) return 0;
	_cesk_method_blocklist[idx].code = code;
	
	if(NULL == (_cesk_method_blocklist[idx].last_intp_inv = cesk_diff_empty()))
	{
		LOG_ERROR("can not allocate initial empty diff package");
		return -1;
	}
	if(NULL == (_cesk_method_blocklist[idx].curr_intp_diff = cesk_diff_empty()))
	{
		LOG_ERROR("can not allocate initial empty diff package");
		return -1;
	}
	if(NULL == (_cesk_method_blocklist[idx].curr_intp_inv = cesk_diff_empty()))
	{
		LOG_ERROR("can not allocate initial empty diff package");
		return -1;
	}
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
		if(_cesk_method_analyze_init(code->branches[i].block, input_frame, atab) < 0)
			return -1;
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
	memset(_cesk_method_blocklist, 0, sizeof(_cesk_method_blocklist));

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
	if(_cesk_method_analyze_init(code, frame, atab) < 0)
	{
		LOG_ERROR("can not intialize the method analyzer");
		goto ERR;
	}
	int front = 0, rear = 1;
	_cesk_method_blockqueue[0] = code->index;
	while(front < rear)
	{
		//TODO
	}
	return 0;
ERR:
	for(i = 0; i < CESK_METHOD_MAX_NBLOCKS; i ++)
	{
		if(NULL != _cesk_method_blocklist[i].last_intp_inv) cesk_diff_free(_cesk_method_blocklist[i].last_intp_inv);
		if(NULL != _cesk_method_blocklist[i].curr_intp_diff) cesk_diff_free(_cesk_method_blocklist[i].curr_intp_diff);
		if(NULL != _cesk_method_blocklist[i].curr_intp_inv) cesk_diff_free(_cesk_method_blocklist[i].curr_intp_inv);
		if(NULL != _cesk_method_blocklist[i].input_diff) cesk_diff_free(_cesk_method_blocklist[i].input_diff);
		if(NULL != _cesk_method_blocklist[i].frame) cesk_frame_free(_cesk_method_blocklist[i].frame);
	}
	if(NULL != atab) cesk_alloctab_free(atab);
	if(NULL != rtab) cesk_reloc_table_free(rtab);
	return NULL;
}
