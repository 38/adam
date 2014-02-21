#include <cesk/cesk_frame.h>
#include <cesk/cesk_block.h>
static cesk_block_t* _cesk_block_buf[DALVIK_BLOCK_MAX_KEYS];
static int32_t      _cesk_block_max_idx;
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
                _cesk_block_buf[i]->fanout[j]  = _cesk_block_buf[_cesk_block_buf[i]->code_block->branches[j].block->index]->input;
            }
        }
    return _cesk_block_buf[entry->index];
}
#define __CB_HANDLER(name) static inline int _cesk_block_interpreter_handler_##name(dalvik_instruction_t* inst, cesk_frame_t* output)
__CB_HANDLER(MOVE)
{
	uint32_t dest = inst->operands[0].payload.uint32;
	uint32_t sour = inst->operands[1].payload.uint32;
	cesk_frame_register_move(output, inst, dest, sour);  
	return 0;
}
__CB_HANDLER(NOP)
{
	/* no operation */
	return 0;
}
__CB_HANDLER(CONST)
{
	uint32_t dest = inst->operands[0].payload.uint32;
	if(inst->operands[1].header.info.type == DVM_OPERAND_TYPE_STRING)
	{
		LOG_TRACE("fixme : string constant requires implementation of java/lang/String");
		return 0;
	}
	return cesk_frame_register_load(output, inst, dest, 
		        	                cesk_store_const_addr_from_operand(inst->operands + 1)); 
}
static inline int _cesk_block_interpret(cesk_block_t* blk)
{
    cesk_frame_t* frame = cesk_frame_fork(blk->input);   /* fork a frame for output */
    const dalvik_block_t* code_block = blk->code_block;
    int i;
#define __CB_INST(name) case DVM_##name: rc = _cesk_block_interpreter_handler_##name(inst, frame); break
    for(i = code_block->begin; i < code_block->end; i ++)
    {
        dalvik_instruction_t* inst = dalvik_instruction_get(i);
        LOG_DEBUG("current instruction: %s", dalvik_instruction_to_string(inst, NULL, 0));
        int rc;
        switch(inst->opcode)
        {
               __CB_INST(MOVE);
			   __CB_INST(NOP);
			   __CB_INST(CONST);
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
