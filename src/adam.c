#include <adam.h>
int adam_init(void)
{
	if(log_init() < 0)
	{
		return -1;
	}
	if(stringpool_init(STRING_POOL_SIZE) < 0)
	{
		LOG_FATAL("failed to initialize string pool");
		return -1;
	}
	if(dalvik_init() < 0)
	{
		LOG_FATAL("failed to initialize dalvik loader");
		return -1;
	}
	if(cesk_init() < 0)
	{
		LOG_FATAL("failed to initialize analyzer");
		return -1;
	}
	if(tag_init() < 0)
	{
		LOG_FATAL("failed to intialize the tag system");
		return -1;
	}
	if(bci_init() < 0)
	{
		LOG_FATAL("failed to initialize built-in class interface");
		return -1;
	}
	return 0;
}
void adam_finalize(void)
{
	bci_finalize();
	tag_finalize();
	cesk_finalize();
	dalvik_finalize();
	stringpool_fianlize();
	log_finalize();
}
