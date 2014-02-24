/**
 * @file cesk_block.c
 * @brief implementation of block analyzer
 */
#include <cesk/cesk_frame.h>
#include <cesk/cesk_block.h>
#include <cesk/cesk_addr_arithmetic.h>
/** @brief the buffer holds all nodes of graph when the graph is constructing */
static cesk_block_t* _cesk_block_buf[DALVIK_BLOCK_MAX_KEYS];
/** @brief the maximum code block index, used for building a graph */
static int32_t      _cesk_block_max_idx;
/** @brief implementation of analyzer block construction */
static inline int _cesk_block_graph_new_imp(const dalvik_block_t* entry)
{
    if(NULL != _cesk_block_buf[entry->index]) 
    {
        /* the block has been visited */
        return 0;
    }
    
    if(_cesk_block_max_idx < entry->index) _cesk_block_max_idx = entry->index;

    size_t size = sizeof(cesk_block_t) + sizeof(cesk_block_t*) * entry->nbranches;

    cesk_block_t* ret = (cesk_block_t*)malloc(size);

    if(NULL == ret)
    {
        LOG_ERROR("can not create analAyzer block for code block %d", entry->index);
        return -1;
    }
    
    _cesk_block_buf[entry->index] = ret;
    
    ret->code_block = entry;
    ret->input = cesk_frame_new(entry->nregs);
   
    int i;
    for(i = 0; i < entry->nbranches; i ++)
    {
        if(entry->branches[i].disabled) continue;
        if(-1 == _cesk_block_graph_new_imp(entry->branches[i].block)) 
            return -1;
    }
    return 0;
}
cesk_block_t* cesk_block_graph_new(const dalvik_block_t* entry)
{
    if(NULL == entry)
        return NULL;
    memset(_cesk_block_buf, 0, sizeof(_cesk_block_buf));
    _cesk_block_max_idx = -1;
    
    _cesk_block_graph_new_imp(entry);

    int i;

    for(i = 0; i <= _cesk_block_max_idx; i ++)
        if(_cesk_block_buf[i] != NULL)
        {
            int j;
            for(j = 0; j < _cesk_block_buf[i]->code_block->nbranches; j ++)
            {
                if(_cesk_block_buf[i]->code_block->branches[j].disabled) continue;
				/* the index of next block */
				uint32_t next_block = _cesk_block_buf[i]->code_block->branches[j].block->index;
                _cesk_block_buf[i]->fanout[j]  = _cesk_block_buf[next_block]->input;
            }
        }
    return _cesk_block_buf[entry->index];
}
#define __CB_HANDLER(name) static inline int _cesk_block_interpreter_handler_##name(const dalvik_instruction_t* inst, cesk_frame_t* output)
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
__CB_HANDLER(MOVE)
{
	uint32_t dest = _cesk_block_operand_to_regidx(inst->operands + 0);
	uint32_t sour = _cesk_block_operand_to_regidx(inst->operands + 1);
	LOG_DEBUG("current operation: move register %d --> register %d", sour, dest);
	cesk_frame_register_move(output, inst, dest, sour);  
	return 0;
}
__CB_HANDLER(NOP)
{
	LOG_DEBUG("current operation: nop");
	return 0;
}
__CB_HANDLER(CONST)
{
	uint32_t dest = _cesk_block_operand_to_regidx(inst->operands + 0);
	if(inst->operands[1].header.info.type == DVM_OPERAND_TYPE_STRING)
	{
		LOG_TRACE("fixme : string constant requires implementation of java/lang/String");
		return 0;
	}
	uint32_t sour = cesk_store_const_addr_from_operand(&inst->operands[1]);

	LOG_DEBUG("current operation: load constant @0x%x --> register %d", sour, dest);
	
	return cesk_frame_register_load(output, inst, dest, sour); 
}
__CB_HANDLER(MONITOR)
{
	LOG_DEBUG("current operation: monitor");
	LOG_TRACE("fixme : threading analysis");
	return 0;
}
__CB_HANDLER(CHECK_CAST)
{
	LOG_DEBUG("current operation: check-cast");
	LOG_TRACE("fixme : check-cast is relied on exception system");
	return 0;
}
__CB_HANDLER(THROW)
{
	LOG_DEBUG("current operation: throw");
	LOG_TRACE("fixme: throw is a part of execption system");
	return 0;
}
/** @brief convert a operand to an address, this actually require the operand 
 * 		   is either a constant or a register which constains atomtic value.
 * 		   Otherwise you may get unexcepted result
 */
static inline uint32_t _cesk_block_operand_to_addr(cesk_frame_t* frame,const dalvik_operand_t* val)
{
	if(val->header.info.is_const)
	{
		/* if the value is a constant value */
		return cesk_store_const_addr_from_operand(val);
	}
	else
	{
		/* if it refer to a register */
		uint32_t reg = _cesk_block_operand_to_regidx(val);
		if(reg < 0 || reg > frame->size) 
		{
			LOG_ERROR("invalid register reference %d", reg);
			return CESK_STORE_ADDR_NULL;
		}
		cesk_set_iter_t iter;
		if(NULL == cesk_set_iter(frame->regs[reg], &iter))
		{
			LOG_ERROR("can not get a iterator for register %d", reg);
			return CESK_STORE_ADDR_NULL;
		}
		uint32_t ret = 0;
		uint32_t addr;
		while((addr = cesk_set_iter_next(&iter)) != CESK_STORE_ADDR_NULL)
		{
			if(!CESK_STORE_ADDR_IS_CONST(addr))
			{
				LOG_WARNING("the address @%x is not a value constant address, ignoring", addr);
				continue;
			}
			ret |= addr;
		}
		if(0 == ret)
		{
			LOG_WARNING("there's no valid value in the register, returning NULL");
			ret = CESK_STORE_ADDR_NULL;
		}
		return ret;
	}
}
__CB_HANDLER(CMP)
{
	uint32_t dest = CESK_FRAME_RESULT_REG;
	uint32_t val1 = _cesk_block_operand_to_addr(output, inst->operands);
	uint32_t val2 = _cesk_block_operand_to_addr(output, inst->operands + 1);

	LOG_DEBUG("current operation: compare address@0x%x to address@0x%x", val1, val2);

	uint32_t res  = cesk_addr_arithmetic_sub(val1, val2);

	if(res == CESK_STORE_ADDR_CONST_PREFIX)
	{
		LOG_WARNING("the result set is empty, just clear the register");
		cesk_frame_register_clear(output, inst, dest);
		return 0;
	}
	cesk_frame_register_load(output, inst, dest, res);
	return 0;
}
static inline int _cesk_block_handler_instance_of(const dalvik_instruction_t* inst, cesk_frame_t* frame)
{
	/* (instance-of result, object, type) */
	//uint32_t dst = inst->operands[0].payload.uint32;
	//uint32_t src = inst->operands[1].payload.uint32;
	uint32_t dst = _cesk_block_operand_to_regidx(inst->operands + 0);
	uint32_t src = _cesk_block_operand_to_regidx(inst->operands + 1);

	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(frame->regs[src],&iter))
	{
		LOG_ERROR("can not aquire iterator for register %d", src);
		return -1;
	}
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(inst->operands[2].header.info.type == DVM_OPERAND_TYPE_CLASS)
	{
		/* use class path */
		const char* classpath = inst->operands[2].payload.methpath;

		LOG_DEBUG("current operation: is object refered by register %d a instance of %s --> register %d", 
				  src, classpath, dst);
		/* try to find the class path */
		uint32_t addr;
		while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
		{
			cesk_value_const_t* value = cesk_store_get_ro(frame->store, addr);
			if(NULL == value)
			{
				LOG_WARNING("can not get value @ address %x", addr);
				continue;
			}
			if(value->type != CESK_TYPE_OBJECT)
			{
				LOG_WARNING("the element in same store is not homogeneric, it's likely a bug");
				continue;
			}
			const cesk_object_t* object = value->pointer.object;
			if(NULL == object)
			{
				LOG_WARNING("can not aquire the object from cell %x", addr);
				continue;
			}
			if(cesk_object_instance_of(object, classpath))
			{
				ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
			}
			else
			{
				ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
			}
		}
		if(CESK_STORE_ADDR_CONST_PREFIX == ret) 
		{
			LOG_WARNING("the result is empty set");
		}
		cesk_frame_register_load(frame, inst, dst, ret);
		return 0;
	}
	else if(inst->operands[2].header.info.type == DVM_OPERAND_TYPE_TYPEDESC)
	{
		LOG_TRACE("fixme: instance-of for a type desc like [object java/lang/String] is still unimplemented");
		return 0;
	}
	LOG_ERROR("invalid operand");
	return -1;
}
static inline int _cesk_block_handler_instance_get(const dalvik_instruction_t* inst, cesk_frame_t* frame)
{
	uint32_t dest = _cesk_block_operand_to_regidx(&inst->operands[0]);
	uint32_t sour = _cesk_block_operand_to_regidx(&inst->operands[1]);
	const char* classpath = inst->operands[2].payload.methpath;
	const char* fieldname = inst->operands[3].payload.methpath;
	dalvik_type_t* type   = inst->operands[4].payload.type;
	if(dest >= frame->size ||
	   sour >= frame->size ||
	   NULL == classpath ||
	   NULL == fieldname ||
	   NULL == type)
	{
		LOG_ERROR("invalid instruction instance-get");
		return -1;
	}
	
	LOG_DEBUG("current operation: objects refered by register %d , field %s/%s with type %s --> %d",
			  sour, classpath, fieldname, dalvik_type_to_string(type, NULL, 0), dest);
	
	cesk_set_t* sour_set = frame->regs[sour];
	cesk_set_iter_t iter;
	
	if(NULL == cesk_set_iter(sour_set, &iter))
	{
		LOG_ERROR("can not aquire iterator for register %d", sour);
		return -1;
	}

	uint32_t addr;
	if(cesk_frame_register_clear(frame, inst, dest) < 0)
	{
		LOG_ERROR("can not clear the old value of register %d", dest);
		return -1;
	}
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		cesk_value_const_t* val_obj = cesk_store_get_ro(frame->store, addr);
		if(NULL == val_obj)
		{
			LOG_WARNING("can not aquire readonly pointer @0x%x", addr);
			continue;
		}
		if(CESK_TYPE_OBJECT != val_obj->type)
		{
			LOG_WARNING("can not get a class memeber from an non-class type");
			continue;
		}
		const cesk_object_t* object = val_obj->pointer.object;

		uint32_t paddr;
		if(cesk_object_get_addr(object, classpath, fieldname, &paddr) < 0)
		{
			LOG_ERROR("can not get member %s/%s", classpath, fieldname);
			continue;
		}
		if(paddr == CESK_STORE_ADDR_NULL) continue;
		cesk_frame_register_append_from_store(frame, inst, dest, paddr);
	}
	return 0;
}
static inline int _cesk_block_handler_instance_put(const dalvik_instruction_t* inst, cesk_frame_t* frame)
{
	uint32_t sour = _cesk_block_operand_to_regidx(&inst->operands[0]);
	uint32_t dest = _cesk_block_operand_to_regidx(&inst->operands[1]);
	const char* classpath = inst->operands[2].payload.methpath;
	const char* fieldname = inst->operands[3].payload.methpath;
	dalvik_type_t* type = inst->operands[4].payload.type;
	
	if(dest >= frame->size ||
	   sour >= frame->size ||
	   NULL == classpath ||
	   NULL == fieldname ||
	   NULL == type)
	{
		LOG_ERROR("invalid instruction instance-get");
		return -1;
	}
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(frame->regs[dest], &iter))
	{
		LOG_ERROR("can not aquire iterator for register %d", dest); 
	}
	/* for each object in the set */
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		
	}
	return 0;
}
__CB_HANDLER(INSTANCE)
{
	if(inst->flags == DVM_FLAG_INSTANCE_OF)
	{
		return _cesk_block_handler_instance_of(inst, output);
	}
	else if(inst->flags == DVM_FLAG_INSTANCE_GET)
	{
		return _cesk_block_handler_instance_get(inst, output);
	}
	else if(inst->flags == DVM_FLAG_INSTANCE_PUT)
	{
		return _cesk_block_handler_instance_put(inst, output);
	}
	//TODO: other operations
	LOG_ERROR("unknown instruction flags 0x%x for opcode = INSTANCE", inst->flags);
	return -1;
}
static inline int _cesk_block_interpret(cesk_block_t* blk)
{
    cesk_frame_t* frame = cesk_frame_fork(blk->input);   /* fork a frame for output */
    const dalvik_block_t* code_block = blk->code_block;
    int i;
#define __CB_INST(name) case DVM_##name: rc = _cesk_block_interpreter_handler_##name(inst, frame); break
    for(i = code_block->begin; i < code_block->end; i ++)
    {
        const dalvik_instruction_t* inst = dalvik_instruction_get(i);
        LOG_DEBUG("current instruction: %s", dalvik_instruction_to_string(inst, NULL, 0));
        int rc;
        switch(inst->opcode)
        {
               __CB_INST(MOVE);
			   __CB_INST(NOP);
			   __CB_INST(CONST);
			   __CB_INST(MONITOR);
			   __CB_INST(CHECK_CAST);
			   __CB_INST(THROW);
			   __CB_INST(CMP);
			   __CB_INST(INSTANCE);
			   /* TODO: other instructions */
        }
        if(rc < 0)
        {
            LOG_ERROR("error during interpert this instruction");
        }
    }
    return 0;
}
#undef __CB_HANDLER
#undef __CB_INST
int cesk_block_analyze(cesk_block_t* block)
{
    if(NULL == block)
    {
        LOG_ERROR("invalid argument");
        return -1;
    }
    _cesk_block_interpret(block);
    return 0;
}
