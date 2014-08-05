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
	/* exception register */
	if(operand->header.info.type == DVM_OPERAND_TYPE_EXCEPTION)
	{
		return CESK_FRAME_EXCEPTION_REG;
	}
	/* result register */
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
 * @brief merge constant addresses in a register to a single constant address
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
			LOG_WARNING("ignoring non-constant address "PRSAddr"", current);
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
 * @param rtab the relocation table used by current context
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_const(
		const dalvik_instruction_t* ins, 
		cesk_frame_t* frame, 
		cesk_reloc_table_t* rtab,
		cesk_diff_buffer_t* D, 
		cesk_diff_buffer_t* I)
{
	if(DVM_OPERAND_TYPE_STRING == ins->operands[1].header.info.type)
	{
		const char* value = ins->operands[1].payload.string;
		uint32_t dest = _cesk_block_operand_to_regidx(ins->operands + 0);
		static const char* clspath = NULL;
		if(NULL == clspath)
		{
			clspath = stringpool_query("java/lang/String");
			if(NULL == clspath) return -1;
		}

		/* the instruction index is determined by current instruction
		 * and the offset is unknown at this time, so we just use CESK_ALLOC_NA */
		cesk_alloc_param_t param = CESK_ALLOC_PARAM(CESK_ALLOC_NA, CESK_ALLOC_NA);

		uint32_t ret = cesk_frame_store_new_object(frame, rtab, ins, &param, clspath, value, D, I);
		if(CESK_STORE_ADDR_NULL == ret)
		{
			LOG_ERROR("can not allocate new instance of class %s in frame %p", clspath, frame);
			return -1;
		}
		LOG_DEBUG("allocated new instance of class %s at store address "PRSAddr"", clspath, ret);

		cesk_value_const_t* store_value = cesk_store_get_ro(frame->store, ret);

		if(NULL == store_value || 
		   CESK_TYPE_OBJECT != store_value->type || 
		   clspath != cesk_object_classpath(store_value->pointer.object) ||
		   NULL == store_value->pointer.object->builtin)
		{
			LOG_ERROR("unexcepted object value");
			return -1;
		}
		
		return cesk_frame_register_load(frame, dest, ret, D, I);
		
	}
	else
	{
		uint32_t dest = _cesk_block_operand_to_regidx(ins->operands + 0);
		uint32_t sour = cesk_store_const_addr_from_operand(&ins->operands[1]);
		if(cesk_frame_register_load(frame, dest, sour, D, I) < 0)
		{
			LOG_ERROR("can not load the value "PRSAddr" to register %u", sour, dest);
			return -1;
		}
	}
	return 0;
}
/**
 * @brief compare the content of 2 constant address bearing registers
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
 * @param context the context ID
 * @param rtab the relocation table
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_instance(
		const dalvik_instruction_t* ins, 
		cesk_frame_t* frame, 
		uint32_t context,
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
			/* although the class can be a built-in class, but the instance-new instruction
			 * itself do not perform any initialization action. So we do not pass any intialization
			 * data to the creator, because we don't know how to initialize the object at this 
			 * time. (Notice it's different from the instance created by the const/string instruction`)*/
			cesk_alloc_param_t param = CESK_ALLOC_PARAM(CESK_ALLOC_NA, CESK_ALLOC_NA);

			ret = cesk_frame_store_new_object(frame, rtab, ins, &param, clspath, NULL, D, I);
			if(CESK_STORE_ADDR_NULL == ret)
			{
				LOG_ERROR("can not allocate new instance of class %s in frame %p", clspath, frame);
				return -1;
			}
			LOG_DEBUG("allocated new instance of class %s at store address "PRSAddr"", clspath, ret);
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
				if(CESK_STORE_ADDR_IS_CONST(addr))
				{
					/* TODO exception */
					LOG_DEBUG("throw exception java.lang.nullPointerException");
					continue;
				}
				if(cesk_frame_store_put_field(frame, addr, sour, clspath, fldname, keep_old, D, I) < 0)
				{
					LOG_ERROR("failed to write filed %s/%s at store address "PRSAddr"", clspath, fldname, addr);
					return -1;
				}
			}
			return 0;
		case DVM_FLAG_INSTANCE_SGET:
			dest = _cesk_block_operand_to_regidx(ins->operands + 0);
			clspath = ins->operands[1].payload.string;
			fldname = ins->operands[2].payload.string;
			sour = cesk_static_field_query(clspath, fldname);
			if(CESK_STORE_ADDR_NULL == sour)
			{
				LOG_ERROR("can not find static field named %s.%s", clspath, fldname);
				return -1;
			}
			else
				LOG_DEBUG("find static field index #%u for %s.%s", CESK_FRAME_REG_STATIC_IDX(sour) ,clspath, fldname);
			if(cesk_frame_register_load_from_static(frame, dest, sour, D, I) < 0)
			{
				LOG_ERROR("failed to write the vlaue of static field %s.%s to register %u", clspath, fldname, sour);
				return -1;
			}
			return 0;
		case DVM_FLAG_INSTANCE_SPUT:
			sour = _cesk_block_operand_to_regidx(ins->operands + 0);
			clspath = ins->operands[1].payload.string;
			fldname = ins->operands[2].payload.string;
			dest = cesk_static_field_query(clspath, fldname);
			if(CESK_STORE_ADDR_NULL == sour)
			{
				LOG_ERROR("can not find static field named %s.%s", clspath, fldname);
				return -1;
			}
			else 
				LOG_DEBUG("find static field index #%u for %s.%s", CESK_FRAME_REG_STATIC_IDX(dest), clspath, fldname);
			if(cesk_frame_static_load_from_register(frame, dest, sour, D, I) < 0)
			{
				LOG_ERROR("failed to write the static field %s.%s", clspath, fldname);
				return -1;
			}
			return 0;
		case DVM_FLAG_INSTANCE_OF:
			dest = _cesk_block_operand_to_regidx(ins->operands + 0);
			sour = _cesk_block_operand_to_regidx(ins->operands + 1);
			clspath = ins->operands[2].payload.string;
			set = frame->regs[sour];
			if(NULL == cesk_set_iter(set, &it))
			{
				LOG_ERROR("can not read the value set of regsiter v%d", sour);
			}
			while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&it)))
			{
				cesk_value_const_t* value = cesk_store_get_ro(frame->store, addr);
				if(NULL == value)
				{
					LOG_WARNING("can not read value from store "PRSAddr, addr);
					continue;
				}
				if(CESK_TYPE_SET == value->type)
				{
					LOG_WARNING("try to check if a value set a instance of an object, must be an error");
					continue;
				}
				//TODO TODO TODO TODO TODO HERE		
			}

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
	/* the starting address of the allocation section */
	const cesk_diff_rec_t* data = diff->data + diff->offset[CESK_DIFF_ALLOC];
	/* how many allocation record do we have */
	size_t nrec = diff->offset[CESK_DIFF_ALLOC + 1] - diff->offset[CESK_DIFF_ALLOC];
	/* of course we need to check the boundary cases */
	if(0 == nrec) return -1;
	if(addr < data[0].addr || data[nrec-1].addr < addr) return -1;
	/* okay, let's do binary search */
	int l = 0, r = nrec;
	while(r - l > 1)
	{
		int m = (l + r) / 2;
		if(data[m].addr <= addr)
			l = m;
		else
			r = m;
	}
	/* verify if the only one left in range is the one wanted */
	if(r - l > 0 && data[l].addr == addr) return l;
	else return -1;
}
/**
 * @brief do a result address --> interal address translation
 * @details this function actually do two things. First, it adjust the
 *          address of newly created object in the caller frame. Second,
 *          convert the address which is actually an relocated address
 *          in the caller frame to relocated address. ( Because we convert
 *          all relocated address to object address when we are constructing
 *          callee frame).
 * @param addr the address to be translated
 * @param frame current stack frame
 * @param diff the result diff
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
		/* convert current address to internal address */
		int idx = _cesk_block_invoke_result_allocate_bsearch(diff, addr);
		if(idx < 0)
		{
			LOG_ERROR("can not find the address in the allocation section");
			return CESK_STORE_ADDR_NULL;
		}
		uint32_t i_addr = addrmap[idx];
		if(CESK_STORE_ADDR_NULL == i_addr)
		{
			LOG_ERROR("there's no map from result address "PRSAddr" to interal address, stopping", i_addr);
			return CESK_STORE_ADDR_NULL;
		}
		r_addr = i_addr;
	}
	/* if this address is not a relocated address in this store, then the translated address is the address itself 
	 * In addition, if this address is relocated in the caller frame, we should substitude the object address to
	 * the internal relocated address. */ 
	else if(CESK_STORE_ADDR_NULL == (r_addr = cesk_alloctab_query(frame->store->alloc_tab, frame->store, addr)))
		r_addr = addr;
	LOG_DEBUG("invocation address translation: "PRSAddr" --> "PRSAddr"", addr, r_addr);
	return r_addr;
}
/**
 * @brief peform the translation process on a address set
 * @param set the address set to be translated
 * @param frame current stack frame
 * @param diff the result diff
 * @param addrmap the address map 
 * @note  the function will modify the input set, and return the same set instance
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
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		/* translate the result address to internal address */
		uint32_t i_addr = _cesk_block_invoke_result_addr_translate(addr, frame, diff, addrmap);
		if(CESK_STORE_ADDR_NULL == i_addr)
		{
			LOG_ERROR("can not translate result address "PRSAddr"", addr);
			return NULL;
		}
		if(cesk_set_modify(set, addr, i_addr) < 0)
		{
			LOG_ERROR("can not modify address "PRSAddr" to address "PRSAddr"", addr, i_addr);
			return NULL;
		}
	}
	return set;
}
/**
 * @brief translate the value, because value might contains pointer to address that
 *        needs to be patched
 * @param value the value to translate
 * @param frame current stack frame
 * @param diff the result idf
 * @param addrmap the address map
 * @return the translated value
 * @note the return value actually is the input address.
 **/
static inline cesk_value_t* _cesk_block_invoke_result_value_translate(
		cesk_value_t* value,
		const cesk_frame_t* frame,
		const cesk_diff_t* diff,
		const uint32_t* addrmap)
{
	/* if the value to be translated is a set, invoke the set translation function directly */
	if(CESK_TYPE_SET == value->type)
	{
		if(NULL == _cesk_block_invoke_result_set_translate(value->pointer.set, frame, diff, addrmap))
		{
			LOG_ERROR("can not translate value of set");
			return NULL;
		}
		return value;
	}
	/* we are expecting the type of this value is object */
	if(CESK_TYPE_OBJECT != value->type)
	{
		LOG_ERROR("unsupported value type %x", value->type);
		return NULL;
	}
	/* translate an object */
	cesk_object_t* object = value->pointer.object;
	cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		/* translate the build in section in an object */
		if(this->built_in)
		{
			uint32_t buf[128];
			uint32_t offset = 0;
			for(;;)
			{
				/* read the address from this build-in structure. */
				int rc = bci_class_get_addr_list(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
				if(rc < 0)
				{
					LOG_ERROR("can not get the address list of built-in class instance %s", this->class.path->value);
					return NULL;
				}
				/* nothing left? quite. So the offset must be continous */
				if(0 == rc) break;
				int j;
				/* translate the address one by one */
				for(j = 0; j < rc; j ++)
					buf[j] = _cesk_block_invoke_result_addr_translate(buf[j], frame, diff, addrmap);
				/* ok, write the addresses back */
				if(bci_class_modify(this->bcidata, offset, buf, rc, this->class.bci->class) < 0)
				{
					LOG_ERROR("can not modify the address list of built-in class instance %s", this->class.path->value);
					return NULL;
				}
				/* update the offset */
				offset += rc;
			}
		}
		/* a source-code-defined section */
		else
		{
			int j;
			/* traverse all addreses and translsate it */
			for(j = 0; j < this->num_members; j ++)
			{
				uint32_t addr = this->addrtab[j];
				uint32_t r_addr = _cesk_block_invoke_result_addr_translate(addr, frame, diff, addrmap);
				if(CESK_STORE_ADDR_NULL == addr)
				{
					LOG_ERROR("can not translate result address "PRSAddr" to interal address", addr);
					return NULL;
				}
				this->addrtab[j] = r_addr;
			}
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return value;
}
/**
 * @brief the interal address map buf 
 **/
static uint32_t* _cesk_block_internal_addr_buf = NULL;
/**
 * @brief the size of interal addr buf 
 **/
static size_t    _cesk_block_internal_addr_bufsize = 0;
/**
 * @brief check the size of the interal address buffer, if it's not a proper size, reallocate it
 * @param nfunc how many function do this instruction actually invokes
 * @param results the invoke results
 * @param nallocation the buffer for an array of number of allocation of each result
 * @param internal_addr the buffer for internal_addr array
 * @return if there's no way to make the buffer in proper size, return a value < 0
 */
static inline int _cesk_block_internal_addr_buf_check(
		uint32_t nfunc, 
		cesk_diff_t** results, 
		size_t* nallocation, 
		uint32_t** internal_addr)
{
	uint32_t i;

	size_t needed = 0;
	for(i = 0; i < nfunc; i ++) 
		needed += (nallocation[i] = results[i]->offset[CESK_DIFF_ALLOC + 1] - results[i]->offset[CESK_DIFF_ALLOC]);

	if(needed > _cesk_block_internal_addr_bufsize)
	{
		if(NULL != _cesk_block_internal_addr_buf) free(_cesk_block_internal_addr_buf);
		/* we make sure that the size is increasing at least 4096 slots, so that we can avoid frequent memory allocation */
		_cesk_block_internal_addr_bufsize = needed + 4096;
		_cesk_block_internal_addr_buf = (uint32_t*)malloc(sizeof(uint32_t) * _cesk_block_internal_addr_bufsize);
		if(NULL == _cesk_block_internal_addr_buf) 
		{
			LOG_ERROR("can not allocate a proper size (which should be %zu uint32_t integers)", _cesk_block_internal_addr_bufsize);
			_cesk_block_internal_addr_bufsize = 0;
			return -1;
		}
	}
	internal_addr[0] = _cesk_block_internal_addr_buf;
	for(i = 1; i < nfunc; i ++) 
		internal_addr[i] = internal_addr[i - 1] + results[i - 1]->offset[CESK_DIFF_ALLOC + 1] - results[i - 1]->offset[CESK_DIFF_ALLOC];
	return 0;
}
/**
 * @brief assign each allocations of a result diff internal addresses
 * @param result the input result diff
 * @param callee_rtab the relocation table used by the target function
 * @param frame the caller frame
 * @param rtab  the caller relocation table
 * @param internal_addr the buffer for the result address array
 * @return the result of operation < 0 indicates error 
 **/
static inline int _cesk_block_allocation_address_assignment(
		const cesk_diff_t* result,
		const cesk_reloc_table_t* callee_rtab,
		const cesk_frame_t* frame,
		cesk_reloc_table_t* rtab,
		uint32_t* internal_addr)
{
	uint32_t i;
	for(i = result->offset[CESK_DIFF_ALLOC]; i < result->offset[CESK_DIFF_ALLOC + 1]; i ++)
	{
		/* append the allocation the relocation table and aquire a fresh relocated address for this allocation */
		uint32_t iaddr;
		const cesk_reloc_item_t* info = cesk_reloc_addr_info(callee_rtab, result->data[i].addr);
		if(NULL == info)
		{
			LOG_ERROR("can not get the info for relocated address "PRSAddr"", result->data[i].addr);
			return -1;
		}
		/* allocate a new relocated address for this object in caller frame 
		 * (If this allocation context already exists in caller frame, the function
		 * will return the existed address instead of allocate a new one) 
		 * In addition this allocation is a dry run, because we just want to try 
		 * what would happend if we allocate with this parameter */
		iaddr = cesk_reloc_allocate(rtab, frame->store, info, 1); 
		if(CESK_STORE_ADDR_NULL == iaddr)
		{
			LOG_ERROR("can not append a new relocated address in the relocation table for result allocation @%u", result->data[i].addr);
			return -1;
		}
		/* update the address map */
		internal_addr[i - result->offset[CESK_DIFF_ALLOC]] = iaddr;
		LOG_DEBUG("new address map "PRSAddr" --> "PRSAddr"", result->data[i].addr, iaddr);
	}
	return 0;
}
/**
 * @brief tanslate the reuse record: only target address needs to be translated
 * @param result the result diff
 * @param internal_addr the internal address list
 * @param frame the caller frame
 * @param buf the diff buffer
 * @return result of the operation < 0 indicates error
 **/
static inline int _cesk_block_invoke_result_reuse_section_translate(
		const cesk_diff_t* result, 
		const uint32_t* internal_addr, 
		const cesk_frame_t* frame, 
		cesk_diff_buffer_t* buf)
{
	uint32_t i;
	for(i = result->offset[CESK_DIFF_REUSE]; i < result->offset[CESK_DIFF_REUSE + 1]; i ++)
	{
		uint32_t ret_addr = result->data[i].addr;
		uint32_t i_addr = _cesk_block_invoke_result_addr_translate(ret_addr, frame, result, internal_addr); 
		if(cesk_diff_buffer_append(buf, CESK_DIFF_REUSE, i_addr, result->data[i].arg.generic) < 0)
		{
			LOG_ERROR("can not append reuse record to the diff buffer");
			return -1;
		}
	}
	return 0;
}
/**
 * @brief translate the allocation section, do a value translation
 * @param result the result diff
 * @param internal_addr the internal address list
 * @param frame the caller frame
 * @param raddr_limit the relocated address limit
 * @param buf the diff buffer
 * @return result of operation < 0 indicates error 
 **/
static inline int _cesk_block_invoke_result_allocation_section_translate(
		cesk_diff_t* result, 
		const uint32_t* internal_addr, 
		const cesk_frame_t* frame, 
		uint32_t raddr_limit, 
		cesk_diff_buffer_t* buf)
{
	uint32_t i;
	/* then allocation section, do a address translation and then object translation */
	for(i = result->offset[CESK_DIFF_ALLOC]; i < result->offset[CESK_DIFF_ALLOC + 1]; i ++)
	{
		uint32_t iaddr = internal_addr[i - result->offset[CESK_DIFF_ALLOC]];
		cesk_value_const_t* store_value;
		/* if the the interal addrness is smaller than raddr_limit, this object 
		 * has been allocated in the target store already, so that what we should do is
		 * to maintain the reuse flag & translate the new address */
		if(raddr_limit > iaddr && (store_value = cesk_store_get_ro(frame->store, iaddr)) != NULL) 
		{
			/* STEP1: reuse the address */
			LOG_DEBUG("address "PRSAddr" is already allocated in this frame, just setup reuse flag", iaddr);
			if(cesk_diff_buffer_append(buf, CESK_DIFF_REUSE, iaddr, CESK_DIFF_REUSE_VALUE(1)) < 0)
			{
				LOG_ERROR("can not append reuse record in the diff buffer");
				return -1;
			}
			/* STEP2: translate and merge the set
			 * after we reuse that address, we need to merge the new value to the address, but object value 
			 * do not need to do this, because if an object is allocated by the same instruction, the field
			 * set will have the same address, what we need to do this take care about the values in the set */
			if(CESK_TYPE_SET == result->data[i].arg.value->type)
			{
				cesk_value_t* value = _cesk_block_invoke_result_value_translate(result->data[i].arg.value, frame, result, internal_addr);
				if(NULL == value) 
				{
					LOG_ERROR("can not translate the value "PRSAddr, result->data[i].addr);
					return -1;
				}
				/* after the address translation, we are to merge this value with the old store value */
				if(NULL == store_value)
				{
					LOG_DEBUG("nothing in the address"PRSAddr, iaddr);
				}
				else if(cesk_set_merge(value->pointer.set, store_value->pointer.set) < 0)
				{
					LOG_ERROR("can not merge the value set");
					return -1;
				}
				if(cesk_diff_buffer_append(buf, CESK_DIFF_STORE, iaddr, value) < 0)
				{
					LOG_ERROR("can not append the new value to the diff buffer at address "PRSAddr"", iaddr);
					return -1;
				}
			}
		}
		/* otherwise, this object is new to the target store, so we need emit an allocation record to the buffer */
		else
		{
			cesk_value_t* value = _cesk_block_invoke_result_value_translate(result->data[i].arg.value, frame, result, internal_addr);
			if(NULL == value)
			{
				LOG_ERROR("can not translate the value");
				return -1;
			}
			if(cesk_diff_buffer_append(buf, CESK_DIFF_ALLOC, iaddr, result->data[i].arg.value) < 0)
			{
				LOG_ERROR("can not append allocation record to the diff buffer");
				return -1;
			}
		}
	}
	return 0;
}
/**
 * @brief translate the register section, just address set translation
 * @param result the result buffer
 * @param internal_addr the internal address lit
 * @param frame the caller frame
 * @param buf the diff buffer
 * @return the result of operations < 0 indicates error
 **/
static inline int _cesk_block_invoke_result_register_section_translation(
		cesk_diff_t* result, 
		const uint32_t* internal_addr, 
		const cesk_frame_t* frame, 
		cesk_diff_buffer_t* buf)
{
	uint32_t i;
	/* okay, register section, just address set translation*/
	for(i = result->offset[CESK_DIFF_REG]; i < result->offset[CESK_DIFF_REG + 1]; i ++)
	{
		uint32_t regid = result->data[i].addr; 
		/* translate the register value set */
		cesk_set_t* set = _cesk_block_invoke_result_set_translate(result->data[i].arg.set, frame, result, internal_addr);
		if(NULL == set)
		{
			LOG_ERROR("can not translate register v%d = %s", regid, cesk_set_to_string(result->data[i].arg.set, 0, 0));
			return -1;
		}
		if(cesk_diff_buffer_append(buf, CESK_DIFF_REG, regid, set) < 0)
		{
			LOG_ERROR("can not append register record to the diff buffer");
			return -1;
		}
	}
	return 0;
}
/**
 * @brief trnslate the store section
 * @param result the result buffer
 * @param internal_addr the internal address list
 * @param frame the caller frame
 * @param buf the diff buffer
 * @return the result operations < 0 indicates error
 **/
static inline int _cesk_block_invoke_result_store_section_translation(
		cesk_diff_t* result, 
		const uint32_t* internal_addr, 
		const cesk_frame_t* frame, 
		uint32_t raddr_limit, 
		cesk_diff_buffer_t* buf)
{
	uint32_t i;
	/* then, store section */
	for(i = result->offset[CESK_DIFF_STORE]; i < result->offset[CESK_DIFF_STORE + 1]; i ++)
	{
		uint32_t addr = _cesk_block_invoke_result_addr_translate(result->data[i].addr, frame, result, internal_addr);
		if(CESK_STORE_ADDR_NULL == addr)
		{
			LOG_ERROR("can not translate address");
			return -1;
		}
		
		cesk_value_t* value = _cesk_block_invoke_result_value_translate(result->data[i].arg.value, frame, result, internal_addr);
		
		if(NULL == value)
		{
			LOG_ERROR("can not translate the value");
			return -1;
		}
		/* Because the object instance itself do not contain any pointer to actual value,
		 * so the only possible situation that we should merge two instance is that this
		 * is an instance of a built-in class */
		if(CESK_TYPE_OBJECT == result->data[i].arg.value->type)
		{
			cesk_value_const_t* store_value;
			cesk_object_t* dest = value->pointer.object;
			
			/* if this is an allocation that merged to existing address, we need to merge the old value */
			if(CESK_STORE_ADDR_IS_RELOC(result->data[i].addr) && raddr_limit > addr && 
			   (store_value = cesk_store_get_ro(frame->store, addr)) != NULL )
			{
				const cesk_object_t* sour = store_value->pointer.object;
				if(NULL == sour->builtin || NULL == dest->builtin)
				{
					LOG_ERROR("can not merge two object instance that is non-built-in one");
					return -1;
				}
				if(sour->builtin->class.bci->class != dest->builtin->class.bci->class)
				{
					LOG_ERROR("can not merge two object instance that is not the same type");
					return -1;
				}
				if(bci_class_merge(dest->builtin->bcidata, sour->builtin->bcidata, dest->builtin->class.bci->class) < 0)
				{
					LOG_ERROR("can not merge the built-in instance %s", dest->builtin->class.path->value);
					return -1;
				}
			}
			if(bci_class_get_relocation_flag(dest->builtin->bcidata, dest->builtin->class.bci->class) > 0) result->data[i].arg.value->reloc = 1;
		}
		/* otherwise, we should merge the values in the set */
		else
		{
			cesk_set_t* set = value->pointer.set;
			cesk_value_const_t* store_value;
			/* if this is an allocation that merged to existing address, we need to merge the old value */
			if(CESK_STORE_ADDR_IS_RELOC(result->data[i].addr) && raddr_limit > addr && 
			   (store_value = cesk_store_get_ro(frame->store, addr)) != NULL )
			{
				if(NULL == store_value)
				{
					LOG_ERROR("can not read store value at address "PRSAddr"", addr);
					return -1;
				}
				if(cesk_set_merge(set, store_value->pointer.set) < 0)
				{
					LOG_ERROR("can not merge old value with the new value");
					return -1;
				}
			}
			if(cesk_set_get_reloc(set) > 0) result->data[i].arg.value->reloc = 1;
		}
		if(cesk_diff_buffer_append(buf, CESK_DIFF_STORE, addr, result->data[i].arg.value) < 0)
		{
			LOG_ERROR("can not append store record to the diff buffer");
			return -1;
		}
	}
	return 0;
}
/**
 * @brief translate a invocation result to the interal diff
 * @param inst instruction
 * @param frame the stack frame 
 * @param rtab the relocation table 
 * @param callee_rtabs the list of relocation tables for callee
 * @param results the list of result diffs
 * @param nfunc the number of functions might be called here
 * @return the translated diff
 **/
static inline cesk_diff_t* _cesk_block_invoke_result_translate(
		const dalvik_instruction_t* inst,
		const cesk_frame_t* frame,
		cesk_reloc_table_t* rtab,
		cesk_reloc_table_t** callee_rtabs,
		cesk_diff_t** results,
		uint32_t nfunc)
{
	/* create a new diff buffer for the result diff */
	uint32_t i;
	cesk_diff_buffer_t* buf = cesk_diff_buffer_new(0, 1);
	if(NULL == buf)
	{
		LOG_ERROR("can not create a new diff buffer for the traslation result");
		goto ERR;
	}
	/* because the values in different diff structure might be reused, so that
	 * we need to make sure that everthing is owned by this result only. 
	 * That is why we need to prepare for writing */
	for(i = 0; i < nfunc; i ++)
	{
		results[i] = cesk_diff_prepare_to_write(results[i]);
		if(NULL == results[i])
		{
			LOG_ERROR("can not duplicate the result diff");
			goto ERR;
		}
	}

	/* the address map, allocation record index --> internal addr */
	static size_t   nallocation[CESK_BLOCK_MAX_NUM_OF_FUNC];
	static uint32_t* internal_addr[CESK_BLOCK_MAX_NUM_OF_FUNC];
	
	if(_cesk_block_internal_addr_buf_check(nfunc, results, nallocation ,internal_addr) < 0)
	{
		LOG_ERROR("can not make the interal address buffer in proper size, aborting");
		goto ERR;
	}


	/* the minimal address which should emit an allocation record, so that 
	 * if the address is smaller than this limit that means this object is
	 * already existed in caller context. In this case, we do not need to emit
	 * an allocation record for this object. */
	uint32_t raddr_limit = cesk_reloc_next_addr(rtab);

	/* now we can get the address maps */
	for(i = 0; i < nfunc; i ++)
	{
		if(_cesk_block_allocation_address_assignment(results[i], callee_rtabs[i], frame, rtab, internal_addr[i]) < 0)
		{
			LOG_ERROR("can not assign internal addresses for allocations in result #%u", i);
			goto ERR;
		}
	}

	/* translate the reuse section */
	for(i = 0; i < nfunc; i ++)
	{
		if(_cesk_block_invoke_result_reuse_section_translate(results[i], internal_addr[i], frame, buf) < 0)
		{
			LOG_ERROR("can not translate the reuse section of result diff #%u", i);
			goto ERR;
		}
	}

	/* translate the allocation section */
	for(i = 0; i < nfunc; i ++)
	{
		if(_cesk_block_invoke_result_allocation_section_translate(results[i], internal_addr[i], frame, raddr_limit, buf) < 0)
		{
			LOG_ERROR("can not translate the allocation section of result diff #%u", i);
			goto ERR;
		}
	}

	/* translate register section */
	for(i = 0; i < nfunc; i ++)
	{
		if(_cesk_block_invoke_result_register_section_translation(results[i], internal_addr[i], frame, buf) < 0)
		{
			LOG_ERROR("can not translate the regiseter of result diff #%u", i);
			goto ERR;
		}
	}

	/* translate the store section */
	for(i = 0; i < nfunc; i ++)
	{
		if(_cesk_block_invoke_result_store_section_translation(results[i], internal_addr[i], frame, raddr_limit, buf) < 0)
		{
			LOG_ERROR("can not translate the store section of result diff #%u", i);
			goto ERR;
		}
	}

	/* finally, we make the diff */
	cesk_diff_t* ret = cesk_diff_from_buffer(buf);
	if(NULL == ret)
	{
		LOG_ERROR("can not create diff from diff buffer");
		goto ERR;
	}
	/* clean up */
	for(i = 0; i < nfunc; i ++) if(results[i]) cesk_diff_free(results[i]);
	cesk_diff_buffer_free(buf);
	return ret;
ERR:
	if(buf) cesk_diff_buffer_free(buf);
	for(i = 0; i < nfunc; i ++)
		if(results[i]) cesk_diff_free(results[i]);
	return NULL;
}
/**
 * @breif the address field of method partition heap
 **/
static uint32_t _cesk_block_method_heap_addr[CESK_BLOCK_METHOD_PARTITION_HEAP_SIZE];
/**
 * @brief the name field of method partition heap
 **/
static const dalvik_block_t* _cesk_block_method_heap_code[CESK_BLOCK_METHOD_PARTITION_HEAP_SIZE];
/**
 * @brief the size of method partition heap
 **/
static uint32_t _cesk_block_method_heap_size;
/**
 * @brief compare two entity in the method partition heap
 * @param a the subscript of the first operand
 * @param b the subscript of the second operand
 * @return the result of comparasion 
 **/
static inline int _cesk_block_method_heap_compare(int a, int b)
{
	if(_cesk_block_method_heap_code[a] > _cesk_block_method_heap_code[b]) return 1;
	if(_cesk_block_method_heap_code[a] < _cesk_block_method_heap_code[b]) return -1;
	if(_cesk_block_method_heap_addr[a] > _cesk_block_method_heap_addr[b]) return 1;
	if(_cesk_block_method_heap_addr[a] < _cesk_block_method_heap_addr[b]) return -1;
	return 0;
}
/**
 * @brief the incrase operation 
 * @param node the index of the node we start with
 * @return nothing
 **/
static inline void _cesk_block_method_heap_incrase(int node)
{
	for(;node < _cesk_block_method_heap_size;)
	{
		int next = node;
		if(node * 2 + 1 < _cesk_block_method_heap_size && _cesk_block_method_heap_compare(next, node * 2 + 1) > 0) next = node * 2 + 1;
		if(node * 2 + 2 < _cesk_block_method_heap_size && _cesk_block_method_heap_compare(next, node * 2 + 2) > 0) next = node * 2 + 2;
		if(node == next) break;
		uint32_t tmp_addr = _cesk_block_method_heap_addr[node];
		const dalvik_block_t* tmp_code = _cesk_block_method_heap_code[node];
		_cesk_block_method_heap_addr[node] = _cesk_block_method_heap_addr[next];
		_cesk_block_method_heap_code[node] = _cesk_block_method_heap_code[next];
		_cesk_block_method_heap_addr[next] = tmp_addr;
		_cesk_block_method_heap_code[next] = tmp_code;
		node = next;
	}
}
/**
 * @brief initialize the method partitionizer 
 * @return nothing
 **/
static inline void _cesk_block_method_heap_init()
{
	int i;
	for(i = _cesk_block_method_heap_size / 2; i >= 0;i --)
		_cesk_block_method_heap_incrase(i);
}
/**
 * @brief get a group address of which the code fields are the same
 * @param p_code the buffer to pass the code to caller
 * @return the result set, NULL if there's no more group
 **/
static inline cesk_set_t* _cesk_block_method_heap_get_partition(const dalvik_block_t** p_code)
{
	if(_cesk_block_method_heap_size == 0) return NULL;
	cesk_set_t* ret = cesk_set_empty_set();
	*p_code = _cesk_block_method_heap_code[0];
	for(;_cesk_block_method_heap_size > 0 && _cesk_block_method_heap_code[0] == *p_code;)
	{
		cesk_set_push(ret, _cesk_block_method_heap_addr[0]);
		_cesk_block_method_heap_addr[0] = _cesk_block_method_heap_addr[_cesk_block_method_heap_size - 1];
		_cesk_block_method_heap_code[0] = _cesk_block_method_heap_code[_cesk_block_method_heap_size - 1];
		_cesk_block_method_heap_size --;
		_cesk_block_method_heap_incrase(0);
	}
	return ret;
}
/**
 * @brief find proper methods for the function call
 * @param inst the invoke instruction
 * @param frame current stack frame
 * @param code the code buffer
 * @param self the self pointer buffer
 * @return the number of method adopted, < 0 indicates errors
 **/
static inline int _cesk_block_find_invoke_method(const dalvik_instruction_t* ins, const cesk_frame_t* frame, const dalvik_block_t** code, cesk_set_t** self)
{
	const char* classpath = ins->operands[0].payload.methpath;
	const char* methodname = ins->operands[1].payload.methpath;
	const dalvik_type_t* const * typelist = ins->operands[2].payload.typelist;
	const dalvik_type_t* rtype = ins->operands[3].payload.type;
	
	if(NULL == classpath || NULL == methodname || NULL == typelist)
	{
		LOG_ERROR("invalid instruction format");
		return -1;
	}

	
	_cesk_block_method_heap_size = 0;

	int ret = 0;

	switch(ins->flags & DVM_FLAG_INVOKE_TYPE_MSK)
	{
		case DVM_FLAG_INVOKE_DIRECT:
		case DVM_FLAG_INVOKE_VIRTUAL:
		case DVM_FLAG_INVOKE_INTERFACE:
		case DVM_FLAG_INVOKE_SUPER:
			if(ins->num_operands - 4 == 0)
			{
				LOG_ERROR("are you trying call a non-static function without a self-pointer? But I don't know how to find the function");
				return -1;
			}
			/* compiler always produces check-cast instruction during the type cast, so we can assume all invoke-direct instruction are legal */
			if(DVM_FLAG_INVOKE_DIRECT == (ins->flags & DVM_FLAG_INVOKE_TYPE_MSK)) goto STATIC_INVOKE;
			/* only invoke-virtual and invoke-interface can be here */
			const cesk_set_t* this = frame->regs[CESK_FRAME_GENERAL_REG(ins->operands[4].payload.uint16)];
			cesk_set_iter_t iter;
			if(NULL == cesk_set_iter(this, &iter))
			{
				LOG_ERROR("can not read register v%d", CESK_FRAME_GENERAL_REG(ins->operands[4].payload.uint16));
				return -1;
			}
			uint32_t addr;
			while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
			{
				cesk_value_const_t* value = cesk_store_get_ro(frame->store, addr);
				if(NULL == value || value->type != CESK_TYPE_OBJECT)
				{
					LOG_WARNING("ignore non-object value "PRSAddr, addr);
					continue;
				}
				const cesk_object_t* object = value->pointer.object;
				const cesk_object_struct_t* current = object->members;
				int i;
				_cesk_block_method_heap_addr[_cesk_block_method_heap_size] = addr;
				for(i = 0; i < object->depth; i ++)
				{
					/* for super call, just skip the class methods */
					if(0 == i && DVM_FLAG_INVOKE_SUPER == (ins->flags & DVM_FLAG_INVOKE_TYPE_MSK)) 
						continue; 
					const char* clspath = current->class.path->value;
					_cesk_block_method_heap_code[_cesk_block_method_heap_size] = dalvik_block_from_method(clspath, methodname, typelist, rtype);
					if(NULL != _cesk_block_method_heap_code[_cesk_block_method_heap_size]) 
					{
						LOG_DEBUG("method %s.%s is actually adopted for address " PRSAddr, clspath, methodname, addr);
						break;
					}
					CESK_OBJECT_STRUCT_ADVANCE(current);
				}
				if(i == object->depth)
					LOG_WARNING("can not found target method %s.%s for object, ignore this address"PRSAddr, classpath, methodname, addr);
				else 
					_cesk_block_method_heap_size ++;
			}
			_cesk_block_method_heap_init();
			for(;NULL != (self[ret] = _cesk_block_method_heap_get_partition(code + ret)); ret ++);
			break;
		case DVM_FLAG_INVOKE_STATIC:
STATIC_INVOKE:
			self[0] = NULL;
			ret = 1;   /* there's only function might be called */
			code[0] = dalvik_block_from_method(classpath, methodname, typelist, rtype);
			if(NULL == code[0])
			{
				LOG_ERROR("can not find the method!");
				return -1;
			}
			LOG_DEBUG("direct function call %s/%s ", classpath, methodname);
			break;
		default:
			LOG_ERROR("unsupported invocation type");
			return -1;
	}

	return ret;
}
/**
 * @brief the instruction handler for function calls
 * @param ins current instruction
 * @param frame current stack frame
 * @param rtab the relocation table
 * @param D the diff buffer to track the modification
 * @param I the diff buffer to tacck the how to revert the modification
 * @param context the caller context
 * @return the result of execution, < 0 indicates failure
 **/
static inline int _cesk_block_handler_invoke(
		const dalvik_instruction_t* ins, 
		cesk_frame_t* frame, 
		cesk_reloc_table_t* rtab, 
		cesk_diff_buffer_t* D, 
		cesk_diff_buffer_t* I,
		const void* context)
{
	int i;
	uint32_t nregs;
	uint32_t nargs;
	static cesk_set_t* args[65536] = {};
	
	const dalvik_block_t* code[CESK_BLOCK_MAX_NUM_OF_FUNC];
	cesk_set_t  * this[CESK_BLOCK_MAX_NUM_OF_FUNC];

	cesk_reloc_table_t* callee_rtable[CESK_BLOCK_MAX_NUM_OF_FUNC];
	cesk_diff_t* invoke_result[CESK_BLOCK_MAX_NUM_OF_FUNC];

	int nfunc = _cesk_block_find_invoke_method(ins, frame, code, this);
	if(nfunc < 0)
	{
		LOG_ERROR("can not find method to call");
		return -1;
	}
	
	int k;
	for(k = 0; k < nfunc; k ++)
	{
		
		/* make a param list */
		cesk_frame_t* callee_frame = NULL;
		nregs = code[k]->nregs;
		/* if the arg list contains a this pointer */
		int has_this = (NULL != this[k]);
		if(ins->flags & DVM_FLAG_INVOKE_RANGE)
		{
			uint32_t arg_from  = CESK_FRAME_GENERAL_REG(ins->operands[4].payload.uint16 + has_this);
			uint32_t arg_to    = CESK_FRAME_GENERAL_REG(ins->operands[5].payload.uint16);
			nargs = arg_to - arg_from + 1 + has_this;
			if(has_this) args[0] = this[k];
			for(i = arg_from; i <= arg_to; i ++)
			{
				args[i - arg_from + has_this] = cesk_set_fork(frame->regs[i]);
				if(NULL == args[i - arg_from]) goto PARAMERR_RANGE;
			}
		}
		else
		{
			nargs = ins->num_operands - 4;
			if(has_this) args[0] = this[k];
			for(i = has_this; i < nargs; i ++)
			{
				uint32_t regnum = CESK_FRAME_GENERAL_REG(ins->operands[i + 4].payload.uint16);
				args[i] = cesk_set_fork(frame->regs[regnum]);
				if(NULL == args[i]) goto PARAMERR;
			}
		}

		callee_frame = cesk_frame_make_invoke(frame, nregs, nargs, args);
		if(NULL == callee_frame || NULL == code)
		{
			LOG_ERROR("can not invoke the function");
			goto ERR;
		}

		/* do function call */
		invoke_result[k] = cesk_method_analyze(code[k], callee_frame, context, callee_rtable + k);
		if(NULL == invoke_result[k])
		{
			LOG_ERROR("can not analyze the function invocation");
			goto ERR;
		}
		
		cesk_frame_free(callee_frame);
	}

	cesk_diff_t* result = _cesk_block_invoke_result_translate(ins, frame, rtab, callee_rtable, invoke_result, nfunc);
	if(NULL == result)
	{
		LOG_ERROR("can not traslate the result diff to interal diff");
		return -1;
	}

	if(cesk_frame_apply_diff(frame, result, rtab, D, I) < 0)
	{
		LOG_ERROR("can not apply the result diff to the frame");
		return -1;
	}
	cesk_diff_free(result);
	return 0;
PARAMERR:
	LOG_ERROR("can not create argument list"); 
	for(i = 0; i < nargs; i ++)
		if(NULL != args[i]) cesk_set_free(args[i]);
	goto ERR;
PARAMERR_RANGE:
	LOG_ERROR("can not create argument list");
	for(i = 0; i < nargs; i ++)
		if(NULL != args[i]) cesk_set_free(args[i]);
ERR:
	return -1;
}
int cesk_block_analyze(
		const dalvik_block_t* code, 
		cesk_frame_t* frame, 
		cesk_reloc_table_t* rtab, 
		cesk_block_result_t* buf, 
		const void* caller_ctx)
{
#if DEBUGGER
	extern int debugger_callback(const dalvik_instruction_t* inst, cesk_frame_t* frame, const void* context);
#endif
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
	uint32_t ctx_id;
	if(NULL != caller_ctx) ctx_id = *(uint32_t*)caller_ctx;
	else ctx_id = 0;
	for(i = code->begin; i < code->end; i ++)
	{
		const dalvik_instruction_t* ins = dalvik_instruction_get(i);
		LOG_DEBUG("currently executing instruction %s", dalvik_instruction_to_string(ins, NULL, 0));
#if DEBUGGER
		if(debugger_callback(ins, frame, caller_ctx) < 0) goto ERR;
#endif
		switch(ins->opcode)
		{
			case DVM_NOP:
				break;
			case DVM_MOVE:
				if(_cesk_block_handler_move(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_CONST:
				if(_cesk_block_handler_const(ins, frame, rtab, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_CMP:
				if(_cesk_block_handler_cmp(ins, frame, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_INSTANCE:
				if(_cesk_block_handler_instance(ins, frame, ctx_id, rtab, dbuf, ibuf) < 0) goto EXE_ERR;
				break;
			case DVM_INVOKE:
				if(_cesk_block_handler_invoke(ins, frame, rtab, dbuf, ibuf, caller_ctx) < 0) goto EXE_ERR;
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
int cesk_block_init()
{
	// do nothing, just follow the protocol
	return 0;
}
void cesk_block_finalize()
{
	if(_cesk_block_internal_addr_buf) free(_cesk_block_internal_addr_buf);
}
