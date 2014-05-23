#include <dalvik/dalvik_instruction.h>

#include <cesk/cesk_block.h>
int _cesk_block_move(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	return 0;
}
int cesk_block_analyze(const dalvik_block_t* code, cesk_frame_t* frame, cesk_block_result_t* buf)
{
	if(NULL == code || NULL == frame || NULL == buf)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	buf->diff = NULL;
	buf->inverse = NULL;
	/* prepare the diff buffers */
	cesk_diff_buffer_t* dbuf; 
	cesk_diff_buffer_t* ibuf;
	dbuf = cesk_diff_buffer_new(0);
	if(NULL == dbuf)
	{
		LOG_ERROR("can not allocate diff buffer");
		goto ERR;
	}
	ibuf = cesk_diff_buffer_new(1);
	if(NULL == ibuf)
	{
		LOG_ERROR("can not allocate inverse diff buffer");
		goto ERR;
	}
	/* ok, let's go */
	int i;
	for(i = code->begin; i < code->end; i ++)
	{
		const dalvik_instruction_t* ins = dalvik_instruction_get(i);
		LOG_DEBUG("currently executing instruction %s", dalvik_instruction_to_string(ins, NULL, 0));
		switch(ins->opcode)
		{
			case DVM_NOP:
				break;
			case DVM_MOVE:
				if(_cesk_block_move(ins, frame, dbuf, ibuf) < 0)
				{
					LOG_ERROR("can not execute current instruction, aborting");
					goto ERR;
				}
				break;
			default:
				LOG_WARNING("ignore unknown opcode");
		}
	}
	/* we almost done, fill up the result buffer */
	buf->frame = frame;
	buf->diff = cesk_diff_from_buffer(dbuf);
	if(NULL == buf->diff)
	{
		LOG_ERROR("can not create diff package");
		goto ERR;
	}
	buf->inverse = cesk_diff_from_buffer(ibuf);
	if(NULL == buf->diff)
	{
		LOG_ERROR("can not create inverse diff pakcage");
		goto ERR;
	}
	/* clean up */
	cesk_diff_buffer_free(dbuf);
	cesk_diff_buffer_free(ibuf);

	return 0;
ERR:
	if(NULL != dbuf) cesk_diff_buffer_free(dbuf);
	if(NULL != ibuf) cesk_diff_buffer_free(ibuf);
	if(NULL != buf->diff) cesk_diff_free(buf->diff);
	if(NULL != buf->inverse) cesk_diff_free(buf->inverse);
	return -1;
}
