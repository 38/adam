#include <dalvik/dalvik_instruction.h>

#include <cesk/cesk_block.h>
#include <cesk/cesk_method.h>

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
	uint16_t to     = _cesk_block_operand_to_regidx(ins->operands + 0);
	uint16_t from   = _cesk_block_operand_to_regidx(ins->operands + 1);
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
	const cesk_set_t* set;
	cesk_set_iter_t it;
	int keep_old = 0;
	uint32_t addr;
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
			dest = _cesk_block_operand_to_regidx(ins->operands + 1);
			sour = _cesk_block_operand_to_regidx(ins->operands + 0);
			clspath = ins->operands[2].payload.string;
			fldname = ins->operands[3].payload.string;
			set = frame->regs[dest];
			if(cesk_set_size(set) == 1) keep_old = 0;
			else keep_old = 1;
			if(NULL == cesk_set_iter(set, &it))
			{
				LOG_ERROR("can not aquire set iterator for register v%d", dest);
				return -1;
			}
			while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&it)))
			{
				if(cesk_frame_store_put_field(frame, addr, sour, clspath, fldname, keep_old, D, I) < 0)
				{
					LOG_ERROR("failed to write filed %s/%s at store address @%x", clspath, fldname, addr);
					return -1;
				}
			}
			return 0;
		/* TODO other instance instructions, static things */
		default:
			LOG_ERROR("unknwon instruction flag %x", ins->flags);
	}
	return -1;
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
/**
 * @brief binary search for the relocated address
 * @param diff the result diff
 * @param addr the address in result diff
 * @return the index in data array < 0 means not found
 **/
static inline int _cesk_block_invoke_result_allocate_bsearch(const cesk_diff_t* diff, uint32_t addr)
{
	const cesk_diff_rec_t* data = diff->data + diff->offset[CESK_DIFF_ALLOC];
	size_t nrec = diff->offset[CESK_DIFF_ALLOC + 1] - diff->offset[CESK_DIFF_ALLOC];
	if(0 == nrec) return -1;
	if(addr < data[0].addr || data[nrec-1].addr < addr) return -1;
	int l = 0, r = nrec;
	while(r - l > 1)
	{
		int m = (l + r) / 2;
		if(data[m].addr < addr)
			r = m;
		else
			l = m;
	}
	if(r - l > 0) return l;
	else return -1;
}
/**
 * @brief do a result address --> interal address translation
 * @param addr the address to be translated
 * @param frame current stack frame
 * @param result the result diff
 * @param addrmap the address map from result address to interal address
 * @return the interal address traslated from the result address
 **/
static inline uint32_t _cesk_block_invoke_result_addr_translate(
		uint32_t addr,
		const cesk_frame_t* frame,
		const cesk_diff_t* diff,
		const uint32_t* addrmap)
{
	uint32_t r_addr;
	/*if this address is a relocated address, that means we need translate it from result 
	 * address space to the interal address space */
	if(CESK_STORE_ADDR_IS_RELOC(addr))
	{
		int idx = _cesk_block_invoke_result_allocate_bsearch(diff, addr);
		if(idx < 0)
		{
			LOG_ERROR("can not find the address in the allocation section");
			return CESK_STORE_ADDR_NULL;
		}
		uint32_t i_addr = addrmap[idx];
		if(CESK_STORE_ADDR_NULL == i_addr)
		{
			LOG_ERROR("there's no map between result address @%x to interal address, stopping translating", i_addr);
			return CESK_STORE_ADDR_NULL;
		}
		r_addr = i_addr;
	}
	/* if this address is not a relocated address in this store, then the translated address is the address itself */ 
	else if(CESK_STORE_ADDR_NULL == (r_addr = cesk_alloctab_query(frame->store->alloc_tab, frame->store, addr)))
		r_addr = addr;
	LOG_DEBUG("invocation result translation: @%x --> @%x", addr, r_addr);
	return r_addr;
}
/**
 * @brief peform the translation process on a address set
 * @param set the address set to be translated
 * @param frame current stack frame
 * @param result the result diff
 * @param addrmap the address map 
 * @note  the function will modify the input set, and return the same address
 * @return the translated set
 **/
static inline cesk_set_t* _cesk_block_invoke_result_set_translate(
		cesk_set_t* set,
		const cesk_frame_t* frame,
		const cesk_diff_t* diff,
		const uint32_t* addrmap)
{
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not aquire the set iterator");
		return NULL;
	}
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL == (addr = cesk_set_iter_next(&iter)))
	{
		uint32_t i_addr = _cesk_block_invoke_result_addr_translate(addr, frame, diff, addrmap);
		if(CESK_STORE_ADDR_NULL == i_addr)
		{
			LOG_ERROR("can not translate result address @%x", addr);
			return NULL;
		}
		if(cesk_set_modify(set, addr, i_addr) < 0)
		{
			LOG_ERROR("can not modify address @%x to address @%x", addr, i_addr);
			return NULL;
		}
	}
	return set;
}
/**
 * @brief translate a invocation result to the interal diff
 * @param inst instruction
 * @param frame the stack frame 
 * @param rtab the relocation table 
 * @param result the result diff
 * @return the translated diff
 **/
static inline cesk_diff_t* _cesk_block_invoke_result_translate(
		const dalvik_instruction_t* inst,
		const cesk_frame_t* frame,
		cesk_reloc_table_t* rtab,
		cesk_diff_t* result)
{
	cesk_diff_buffer_t* buf = cesk_diff_buffer_new(0, 0);
	if(NULL == buf)
	{
		LOG_ERROR("can not create a new diff buffer for the traslation result");
		return NULL;
	}
	size_t nallocation = result->offset[CESK_DIFF_ALLOC + 1] - result->offset[CESK_DIFF_ALLOC];
	uint32_t interal_addr[nallocation];
	memset(interal_addr, 0xff, sizeof(uint32_t) * nallocation);
	/* proceed the allocation section */
	uint32_t i;
	for(i = result->offset[CESK_DIFF_ALLOC]; i < result->offset[CESK_DIFF_ALLOC + 1]; i ++)
	{
		/* append the allocation the relocation table and aquire a fresh relocated address for this allocation */
		uint32_t iaddr;
		if(CESK_TYPE_OBJECT == result->data[i].arg.value->type)
			iaddr = cesk_reloc_table_append(
					rtab, inst, 
					CESK_STORE_ADDR_NULL, 
					0, result->data[i].addr, 
					cesk_object_classpath(result->data[i].arg.value->pointer.object));
		else
			iaddr = cesk_reloc_table_append(
					rtab, inst,
					CESK_STORE_ADDR_NULL,
					0, result->data[i].addr,
					NULL);
		if(CESK_STORE_ADDR_NULL == iaddr)
		{
			LOG_ERROR("can not append a new relocated address in the relocation table for result allocation @%u", result->data[i].addr);
			goto ERR;
		}
		interal_addr[i - result->offset[CESK_DIFF_ALLOC]] = iaddr;
		if(cesk_diff_buffer_append(buf, CESK_DIFF_ALLOC, iaddr, result->data[i].arg.value) < 0)
		{
			LOG_ERROR("can not append allocation record to the diff buffer");
			goto ERR;
		}
	}
	/* then proceed the reuse section, perform an address translation */
	for(i = result->offset[CESK_DIFF_REUSE]; i < result->offset[CESK_DIFF_REUSE + 1]; i ++)
	{
		uint32_t ret_addr = result->data[i].addr;
		uint32_t i_addr = _cesk_block_invoke_result_addr_translate(ret_addr, frame, result, interal_addr); 
		if(cesk_diff_buffer_append(buf, CESK_DIFF_REUSE, i_addr, result->data[i].arg.generic) < 0)
		{
			LOG_ERROR("can not append reuse record to the diff buffer");
			goto ERR;
		}
	}
	/* okay, register section */
	for(i = result->offset[CESK_DIFF_REG]; i < result->offset[CESK_DIFF_REG + 1]; i ++)
	{
		uint32_t regid = result->data[i].addr; 
		/* translate the register value set */
		cesk_set_t* set = _cesk_block_invoke_result_set_translate(result->data[i].arg.set, frame, result, interal_addr);
		if(NULL == set)
		{
			LOG_ERROR("can not translate register v%d = %s", regid, cesk_set_to_string(result->data[i].arg.set, 0, 0));
			goto ERR;
		}
		if(cesk_diff_buffer_append(buf, CESK_DIFF_REG, regid, set) < 0)
		{
			LOG_ERROR("can not append register record to the diff buffer");
			goto ERR;
		}
	}
	/* then, store section */
	for(i = result->offset[CESK_DIFF_STORE]; i < result->offset[CESK_DIFF_STORE + 1]; i ++)
	{
		uint32_t addr = _cesk_block_invoke_result_addr_translate(result->data[i].addr, frame, result, interal_addr);
		if(CESK_STORE_ADDR_NULL == addr)
		{
			LOG_ERROR("can not translate address");
			goto ERR;
		}
		if(CESK_TYPE_OBJECT == result->data[i].arg.value->type)
		{
			LOG_ERROR("invalid store section, only set value allowed");
			goto ERR;
		}
		cesk_set_t* set = _cesk_block_invoke_result_set_translate(result->data[i].arg.value->pointer.set, frame, result, interal_addr);
		if(NULL == set)
		{
			LOG_ERROR("can not translate the value set");
			goto ERR;
		}
		if(cesk_diff_buffer_append(buf, CESK_DIFF_STORE, addr, set) < 0)
		{
			LOG_ERROR("can not append store record to the diff buffer");
			goto ERR;
		}
	}
	cesk_diff_t* ret = cesk_diff_from_buffer(buf);
	if(NULL == ret)
	{
		LOG_ERROR("can not create diff from diff buffer");
		goto ERR;
	}
	cesk_diff_free(result);
	cesk_diff_buffer_free(buf);
	return ret;
ERR:
	if(buf) cesk_diff_buffer_free(buf);
	if(result) cesk_diff_free(result);
	return NULL;
}
/**
 * @brief the instruction handler for function calls
 * @param ins current instruction
 * @param frame current stack frame
 * @param rtab the relocation table
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_invoke(
		const dalvik_instruction_t* ins, 
		cesk_frame_t* frame, 
		cesk_reloc_table_t* rtab, 
		cesk_diff_buffer_t* D, 
		cesk_diff_buffer_t* I)
{
	const char* classpath = ins->operands[0].payload.methpath;
	const char* methodname = ins->operands[1].payload.methpath;
	const dalvik_type_t* const * typelist = ins->operands[2].payload.typelist;
	if(NULL == classpath || NULL == methodname || NULL == typelist)
	{
		LOG_ERROR("invalid instruction format");
		return -1;
	}
	const dalvik_block_t* code = NULL;
	cesk_frame_t* callee_frame = NULL;
	/* currently, we only consider the static and direct invocation */
	switch(ins->flags & DVM_FLAG_INVOKE_TYPE_MSK)
	{
		case DVM_FLAG_INVOKE_STATIC:
		case DVM_FLAG_INVOKE_DIRECT:
			code = dalvik_block_from_method(classpath, methodname, typelist);
			if(NULL == code)
			{
				LOG_ERROR("can not find the method!");
				break;
			}
			LOG_INFO("function invocation %s/%s", classpath, methodname);
			if(ins->flags & DVM_FLAG_INVOKE_RANGE)
			{
				/* TODO range invocation support */
				LOG_TRACE("fixme: We need range invocation support");
			}
			else
			{
				uint32_t nregs = code->nregs;
				uint32_t nargs = ins->num_operands - 3;
				cesk_set_t* args[16] = {};
				int i;
				for(i = 0; i < nargs; i ++)
				{
					uint32_t regnum = CESK_FRAME_GENERAL_REG(ins->operands[i + 3].payload.uint16);
					args[i] = cesk_set_fork(frame->regs[regnum]);
					if(NULL == args[i]) goto PARAMERR;
				}
				callee_frame = cesk_frame_make_invoke(frame, nregs, nargs, args);
				break;
PARAMERR:
				LOG_ERROR("can not create argument lst"); 
				for(i = 0; i < nargs; i ++)
					if(NULL != args[i]) cesk_set_free(args[i]);
				goto ERR;
			}
			break;
		default:
			LOG_ERROR("unsupported invocation type");
			goto ERR;
	}
	if(NULL == callee_frame || NULL == code)
	{
		LOG_ERROR("can not invoke the function");
		goto ERR;
	}
	cesk_diff_t* result = cesk_method_analyze(code, callee_frame);
	if(NULL == result)
	{
		LOG_ERROR("can not analyze the function invocation");
		goto ERR;
	}
	result = _cesk_block_invoke_result_translate(ins, frame, rtab, result);
	if(NULL == result)
	{
		LOG_ERROR("can not traslate the result diff to interal diff");
		goto ERR;
	}
	/* TODO maintain the rtable to pass the newly created object in the subroutine to the caller */
	if(cesk_frame_apply_diff(frame, result, rtab, D, I) < 0)
	{
		LOG_ERROR("can not apply the result diff to the frame");
		return -1;
	}
	cesk_frame_free(callee_frame);
	cesk_diff_free(result);
	return 0;
ERR:
	return -1;
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
	cesk_diff_buffer_t* dbuf = NULL; 
	cesk_diff_buffer_t* ibuf = NULL;
	dbuf = cesk_diff_buffer_new(0, 0);
	if(NULL == dbuf)
	{
		LOG_ERROR("can not allocate diff buffer");
		goto ERR;
	}
	ibuf = cesk_diff_buffer_new(1, 0);
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
				/* TODO */
				if(_cesk_block_handler_invoke(ins, frame, rtab, dbuf, ibuf) < 0) goto EXE_ERR;
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
