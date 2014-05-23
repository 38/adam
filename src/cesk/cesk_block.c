#include <dalvik/dalvik_instruction.h>

#include <cesk/cesk_block.h>

int cesk_block_analyze(const dalvik_block_t* code, cesk_frame_t* frame, cesk_block_result_t* buf)
{
	if(NULL == code || NULL == frame || NULL == buf)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	int i;
	for(i = code->begin; i < code->end; i ++)
	{
		const dalvik_instruction_t* ins = dalvik_instruction_get(i);
		switch(ins->opcode)
		{
			default:
				LOG_WARNING("ignore unknown opcode");
		}
	}
	return 0;
}
