#include <tag/tag.h>
int tag_init()
{
	if(tag_set_init() < 0)
	{
		LOG_FATAL("can not intialize module tag_set");
		return -1;
	}
	return 0;
}
void tag_finalize()
{
	tag_set_finalize();
	return;
}
