#include <cesk/cesk.h>
int cesk_init(void)
{
	if(cesk_value_init() < 0)
	{
		LOG_FATAL("can not initialize module cesk_value");
		return -1;
	}
	if(cesk_set_init() < 0)
	{
		LOG_FATAL("can not initialize module cesk_set");
		return -1;
	}
	if(cesk_reloc_init() < 0)
	{
		LOG_FATAL("can not initialize relocation table");
		return -1;
	}
	if(cesk_method_init() < 0)
	{
		LOG_FATAL("can not initialize method analyzer");
		return -1;
	}
	if(cesk_static_init() < 0)
	{
		LOG_FATAL("can not initialize static field table module");
		return -1;
	}
	return 0;
}
void cesk_finalize(void)
{
	cesk_static_finalize();
	cesk_method_finalize();
	cesk_reloc_finalize();
	cesk_value_finalize();
	cesk_set_finalize();
}
