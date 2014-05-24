#include <dalvik/dalvik_instruction.h>

#include <cesk/cesk_block.h>

/** @brief convert an register referencing operand to index of register */
static inline uint32_t _cesk_block_operand_to_regidx(const dalvik_operand_t* operand)
{
	if(operand->header.info.is_const)
	{
		LOG_ERROR("can not convert a constant operand to register index");
		return CESK_STORE_ADDR_NULL;
	}
	/* the exception type is special, it just means we want to use the exception register */
	if(operand->header.info.type == DVM_OPERAND_TYPE_EXCEPTION)
	{
		return CESK_FRAME_EXCEPTION_REG;
	}
	else if(operand->header.info.is_result)
	{
		return CESK_FRAME_RESULT_REG;
	}
	else 
	{
		/* general registers */
		return CESK_FRAME_GENERAL_REG(operand->payload.uint16);
	}
}

static inline int _cesk_block_move(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	uint16_t from = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint16_t to   = _cesk_block_operand_to_regidx(ins->operands + 1);
	if(ins->operands[0].header.info.size)
	{
		if(cesk_frame_register_move(frame, to, from, D, I) < 0)
		{
			LOG_ERROR("can not move content of register from %d to %d", from, to);
			return -1;
		}
		if(cesk_frame_register_move(frame, to + 1, from + 1, D, I) < 0)
		{
			LOG_ERROR("can not move content of register from %d to %d", from, to);
			return -1;
		}
	}
	else
	{
		if(cesk_frame_register_move(frame, to, from, D, I) < 0)
		{
			LOG_ERROR("can not move content of register from %d to %d", from, to);
			return -1;
		}
	}
	return 0;
}
static inline int _cesk_block_const(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	uint32_t dest = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint32_t sour = cesk_store_const_addr_from_operand(&ins->operands[1]);
	return cesk_frame_register_load(frame, dest, sour, D, I);
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
				if(_cesk_block_move(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_CONST:
				if(_cesk_block_const(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_MONITOR:
				LOG_TRACE("fixme: this instruction is ignored");
				break;
			case DVM_CHECK_CAST:
				/* TODO */
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
EXE_ERR:
	LOG_ERROR("can not execute current instruction, aborting");
ERR:
	if(NULL != dbuf) cesk_diff_buffer_free(dbuf);
	if(NULL != ibuf) cesk_diff_buffer_free(ibuf);
	if(NULL != buf->diff) cesk_diff_free(buf->diff);
	if(NULL != buf->inverse) cesk_diff_free(buf->inverse);
	return -1;
}
