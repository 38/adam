#include <dalvik/dalvik.h>
int dalvik_static_field_count = 0;
int dalvik_init(void)
{
	if(dalvik_instruction_init() < 0)
	{
		LOG_ERROR("can not initialize dalvik_instruction.c");
		return -1;
	}
	if(dalvik_tokens_init() < 0)
	{
		LOG_ERROR("can not inintialize dalvik_token.c");
		return -1;
	}
	if(dalvik_label_init() < 0)
	{
		LOG_ERROR("can not initialize dalvik_label.c");
		return -1;
	}
	if(dalvik_type_init() < 0)
	{
		LOG_ERROR("can not intialize dalvik_type.c");
		return -1;
	}
	if(dalvik_memberdict_init() < 0)
	{
		LOG_ERROR("can not intialize dalvik_memberdict.c");
		return -1;
	}
	if(dalvik_exception_init() < 0)
	{
		LOG_ERROR("can not intialize dalvik_exception.c");
		return -1;
	}
	if(dalvik_block_init() < 0)
	{
		LOG_ERROR("can not initialize dalvik_block.c");
		return -1;
	}
	return 0;
}
void dalvik_finalize(void)
{
	dalvik_block_finalize();
	dalvik_exception_finalize();
	dalvik_memberdict_finalize();
	dalvik_instruction_finalize();
	dalvik_label_finalize();
	dalvik_type_finalize();
}
