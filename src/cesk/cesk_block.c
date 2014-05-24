#include <dalvik/dalvik_instruction.h>

#include <cesk/cesk_block.h>

/** 
 * @brief convert an register referencing operand to index of register 
 * @param operand the operand
 * @return the register id that the operand reference, CESK_STORE_ADDR_NULL indicates errors
 **/
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
/**
 * @brief merge content of a register contants constants address to a single constant address
 * @param frame
 * @param sour the index of source register 
 * @return the merged constant address, CESK_STORE_ADDR_NULL indicates errors
 **/
static inline uint32_t _cesk_block_register_const_merge(const cesk_frame_t* frame, uint32_t sour)
{
	const cesk_set_t* sour_set = frame->regs[sour];
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(sour_set, &iter))
	{
		LOG_ERROR("can not aquire set iterator for register v%d", sour);
		return CESK_STORE_ADDR_NULL;
	}
	uint32_t input = CESK_STORE_ADDR_EMPTY;
	uint32_t current;
	while(CESK_STORE_ADDR_NULL != (current = cesk_set_iter_next(&iter)))
	{
		if(!CESK_STORE_ADDR_IS_CONST(current))
		{
			LOG_WARNING("ignoring non-constant address @%x", current);
			continue;
		}
		input |= current;
	}
	return input;
}
/**
 * @brief the instruction handler for move instructions 
 * @param ins current instruction
 * @param frame current stack frame
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_move(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	uint16_t from = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint16_t to   = _cesk_block_operand_to_regidx(ins->operands + 1);
	if(ins->operands[0].header.info.size)
	{
		/* if this instruction is to move a register pair */
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
		/* otherwise move the single instruction */
		if(cesk_frame_register_move(frame, to, from, D, I) < 0)
		{
			LOG_ERROR("can not move content of register from %d to %d", from, to);
			return -1;
		}
	}
	return 0;
}
/**
 * @brief the instruction handler for constant loading instructions 
 * @param ins current instruction
 * @param frame current stack frame
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_const(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	uint32_t dest = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint32_t sour = cesk_store_const_addr_from_operand(&ins->operands[1]);
	/* TODO const-string and const-class */
	if(cesk_frame_register_load(frame, dest, sour, D, I) < 0)
	{
		LOG_ERROR("can not load the value @%x to register %u", sour, dest);
		return -1;
	}
	return 0;
}
/**
 * @brief compare the content of 2 constant address bearing register
 * @param frame current stack frame
 * @param left the index of the register which is the left operand
 * @param right the index of the register which is the right operand
 * @return the comparasion result, if the result is CESK_STORE_ADDR_NULL that means an error occured
 **/
static inline uint32_t _cesk_block_regcmp(const cesk_frame_t* frame, uint32_t left, uint32_t right)
{
	if(left >= frame->size || right >= frame->size)
	{
		LOG_ERROR("invalid register reference");
		return CESK_STORE_ADDR_NULL;
	}
	uint32_t aleft = _cesk_block_register_const_merge(frame, left);
	uint32_t aright = _cesk_block_register_const_merge(frame, right);
	if(CESK_STORE_ADDR_NULL == aleft || CESK_STORE_ADDR_NULL == aright)
		return CESK_STORE_ADDR_NULL;
	return cesk_arithmetic_sub(aleft, aright);
}
/**
 * @brief the instruction handler for comparasion instructions 
 * @param ins current instruction
 * @param frame current stack frame
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_cmp(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	uint32_t dest = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint32_t left = _cesk_block_operand_to_regidx(ins->operands + 1);
	uint32_t right = _cesk_block_operand_to_regidx(ins->operands + 2);
	if(CESK_STORE_ADDR_NULL == left || CESK_STORE_ADDR_NULL == right)
	{
		LOG_ERROR("invalid operand");
		return -1;
	}
	int wide = ins->operands[1].header.info.size;

	/* compare the first part */
	uint32_t result = _cesk_block_regcmp(frame, left, right);
	if(CESK_STORE_ADDR_NULL == result)
	{
		LOG_ERROR("can not compare regiseter v%d and v%d", left, right);
		return -1;
	}

	/* if this is a wide comparasion and the first part might be equal, then we need to compare the second part */
	if(wide && CESK_STORE_ADDR_CONST_CONTAIN(result, ZERO))
	{
		/* clear the equal bit */
		result ^= (CESK_STORE_ADDR_ZERO ^ CESK_STORE_ADDR_CONST_PREFIX);
		result |= _cesk_block_regcmp(frame, left + 1, right + 1);
	}

	/* finally, we load the result to the destination */
	return cesk_frame_register_load(frame, dest, result, D, I);
}
/**
 * @brief the instruction handler for instance operating instructions 
 * @param ins current instruction
 * @param frame current stack frame
 * @param rtab the relocation table
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_instance(
		const dalvik_instruction_t* ins, 
		cesk_frame_t* frame, 
		cesk_reloc_table_t* rtab, 
		cesk_diff_buffer_t* D, 
		cesk_diff_buffer_t* I)
{
	uint32_t dest, sour;
	const char* clspath;
	const char* fldname;
	uint32_t ret;
	switch(ins->flags)
	{
		case DVM_FLAG_INSTANCE_NEW:
			dest = _cesk_block_operand_to_regidx(ins->operands + 0);
			clspath = ins->operands[1].payload.string;
			ret = cesk_frame_store_new_object(frame, rtab, ins, clspath, D, I);
			if(CESK_STORE_ADDR_NULL == ret)
			{
				LOG_ERROR("can not allocate new instance of class %s in frame %p", clspath, frame);
				return -1;
			}
			LOG_DEBUG("allocated new instance of class %s at store address @%x", clspath, ret);
			return cesk_frame_register_load(frame, dest, ret, D, I);
		case DVM_FLAG_INSTANCE_GET:
			dest = _cesk_block_operand_to_regidx(ins->operands + 0);
			sour = _cesk_block_operand_to_regidx(ins->operands + 1);
			clspath = ins->operands[2].payload.string;
			fldname = ins->operands[3].payload.string;
			return cesk_frame_register_load_from_object(frame, dest, sour, clspath, fldname, D, I); 
		case DVM_FLAG_INSTANCE_PUT:
			dest = _cesk_block_operand_to_regidx(ins->operands + 0);
			sour = _cesk_block_operand_to_regidx(ins->operands + 1);
			clspath = ins->operands[2].payload.string;
			fldname = ins->operands[3].payload.string;
			return cesk_frame_store_put_field(frame, dest, sour, clspath, fldname, D, I); 
		/* TODO other instance instructions, static things */
		default:
			LOG_ERROR("unknwon instruction flag %x", ins->flags);
	}
	return 0;
}
/**
 * @brief the instruction handler for unary operation  
 * @param ins current instruction
 * @param frame current stack frame
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_unop(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	uint32_t dest = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint32_t sour = _cesk_block_operand_to_regidx(ins->operands + 1);
	if(CESK_STORE_ADDR_NULL == dest || CESK_STORE_ADDR_NULL == sour)
	{
		LOG_ERROR("bad register reference");
		return -1;
	}
	/* we merge the source first */
	uint32_t input = _cesk_block_register_const_merge(frame, sour);
	if(CESK_STORE_ADDR_NULL == input)
	{
		LOG_ERROR("invalid source register");
		return -1;
	}
	/* acctual computation */
	uint32_t output;
	switch(ins->flags)
	{
		case DVM_FLAG_UOP_NEG:
			output = cesk_arithmetic_neg(input);
			break;
		case DVM_FLAG_UOP_NOT:
			output = cesk_arithmetic_neg(input);
			break;
		case DVM_FLAG_UOP_TO:
			output = input;
			break;
		default:
			LOG_ERROR("unknown instruction flag %x", ins->flags);
			return -1;
	}
	/* load the new value */
	return cesk_frame_register_load(frame, dest, output, D, I);
}
/**
 * @brief the instruction handler for binary operation  
 * @param ins current instruction
 * @param frame current stack frame
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_binop(const dalvik_instruction_t* ins, cesk_frame_t* frame, cesk_diff_buffer_t* D, cesk_diff_buffer_t* I)
{
	uint32_t dest = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint32_t left = CESK_STORE_ADDR_NULL;
	uint32_t right = CESK_STORE_ADDR_NULL;
	if(ins->operands[1].header.info.is_const)
		left = cesk_store_const_addr_from_operand(ins->operands + 1);
	else
		left = _cesk_block_register_const_merge(frame, _cesk_block_operand_to_regidx(ins->operands + 1));
	if(ins->operands[2].header.info.is_const)
		right = cesk_store_const_addr_from_operand(ins->operands + 2);
	else
		right = _cesk_block_register_const_merge(frame, _cesk_block_operand_to_regidx(ins->operands + 2));
	//uint32_t aleft = _cesk_block_register_const_merge(frame, left);
	//uint32_t aright = _cesk_block_register_const_merge(frame, right);

	if(CESK_STORE_ADDR_NULL == left || CESK_STORE_ADDR_NULL == right)
	{
		LOG_ERROR("invalid source registers content");
		return -1;
	}

	uint32_t result = 0;
	/* actual operation */
	switch(ins->flags)
	{
		case DVM_FLAG_BINOP_ADD:
			result = cesk_arithmetic_add(left, right);
			break;
		case DVM_FLAG_BINOP_SUB:
			result = cesk_arithmetic_sub(left, right);
			break;
		case DVM_FLAG_BINOP_MUL:
			result = cesk_arithmetic_mul(left, right);
			break;
		case DVM_FLAG_BINOP_DIV:
			result = cesk_arithmetic_div(left, right);
			break;
		case DVM_FLAG_BINOP_AND:
			result = cesk_arithmetic_and(left, right);
			break;
		case DVM_FLAG_BINOP_OR:
			result = cesk_arithmetic_or(left, right);
			break;
		case DVM_FLAG_BINOP_XOR:
			result = cesk_arithmetic_xor(left, right);
			break;
		/* TODO other operations */
		default:
			LOG_ERROR("unknown instruction flag %x", ins->flags);
			return -1;
	}

	/* finally load the result */
	return cesk_frame_register_load(frame, dest, result, D, I);
}
int cesk_block_analyze(const dalvik_block_t* code, cesk_frame_t* frame, cesk_reloc_table_t* rtab, cesk_block_result_t* buf)
{
	if(NULL == code || NULL == frame || NULL == buf || NULL == rtab)
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
				if(_cesk_block_handler_move(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_CONST:
				if(_cesk_block_handler_const(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_CMP:
				if(_cesk_block_handler_cmp(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_INSTANCE:
				if(_cesk_block_handler_instance(ins, frame, rtab, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_INVOKE:
				/* TODO inter-procedual */
				break;
			case DVM_UNOP:
				if(_cesk_block_handler_unop(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_BINOP:
				if(_cesk_block_handler_binop(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			/* TODO implement other instructions */
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
