#include <tag/tag.h>
int tag_init()
{
	if(tag_set_init() < 0)
	{
		LOG_FATAL("can not intialize module tag_set");
		return -1;
	}
	if(tag_tracker_init() < 0)
	{
		LOG_FATAL("can not finalize the tag tracker module");
	}
	return 0;
}
void tag_finalize()
{
	tag_tracker_finalize();
	tag_set_finalize();
	return;
}
