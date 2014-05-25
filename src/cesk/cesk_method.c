#include <cesk/cesk_method.h>
/**
 * @brief the data structure we used for intermedian data storage
 **/
typedef struct {
	int	init;               /*!< is this block initialized? */
	const dalvik_block_t* code; /*!<the code for this block */
	uint32_t timestamp;     /*!< when do we analyze this block last time */
	cesk_diff_t* last_intp_inv;  /*!< the last interpretator diff inversion (from output to input) */
	cesk_diff_t* curr_intp_diff; /*!< the current interepator diff (from input to output) */
	cesk_diff_t* curr_intp_inv;  /*!< the current interepator diff (intput to output) */
	cesk_diff_t* input_diff;     /*!< the diff from last input to current input */
	cesk_frame_t*  frame;        /*!< the result stack frame of this block */
} _cesk_method_codeblock_t;
static _cesk_method_codeblock_t _cesk_method_blocklist[CESK_METHOD_MAX_NBLOCKS];

/**
 * @brief initialize the analyzer
 * @param block the block graph
 * @param input_frame the input stack frame
 * @return the result of initialization < 0 indicates an error
 **/
static inline int _cesk_method_analyze_init(const dalvik_block_t* block, cesk_frame_t* input_frame, cesk_alloctab_t* atab)
{
	LOG_DEBUG("start analyzing code block graph at %p", block);
	uint32_t idx = block->index;
	if(_cesk_method_blocklist[idx].init) return 0;
	_cesk_method_blocklist[idx].code = block;
	
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

	LOG_DEBUG("code block #%d is initialized", block->index);

	int i;
	for(i = 0; i < block->nbranches; i ++)
	{
		if(block->branches[i].disabled) continue;
		if(_cesk_method_analyze_init(block->branches[i].block, input_frame, atab) < 0)
			return -1;
	}

	return 0;
}
cesk_diff_t* cesk_method_analyze(const dalvik_block_t* code, cesk_frame_t* frame)
{
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
	//TODO: start analyze
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
