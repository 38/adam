#include <cesk/cesk.h>
int cesk_init(void)
{
	if(cesk_value_init() < 0)
	{
		LOG_ERROR("can not initialize module cesk_value");
		return -1;
	}
	if(cesk_set_init() < 0)
	{
		LOG_ERROR("can not initialize module cesk_set");
		return -1;
	}
	return 0;
}
void cesk_finalize(void)
{
	cesk_value_finalize();
	cesk_set_finalize();
}
