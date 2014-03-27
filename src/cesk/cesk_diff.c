#include <cesk/cesk_diff.h>
cesk_diff_item_t* cesk_diff_item_new()
{
	cesk_diff_item_t* item = (cesk_diff_item_t*)malloc(sizeof(cesk_diff_item_t));
	if(NULL == item)
	{
		LOG_ERROR("can not allocate memory for the new diff item struct");
		return NULL;
	}
	memset(item, 0 ,sizeof(cesk_diff_item_t));
	return item;
}
void cesk_diff_item_free(cesk_diff_item_t* item)
{
	if(NULL == item) return;
	free(item);
}
cesk_diff_t* cesk_diff_new()
{
	cesk_diff_t* ret = (cesk_diff_t*)malloc(sizeof(cesk_diff_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocaate memory for the new diff struct");
		return NULL;
	}
	memset(ret, 0, sizeof(cesk_diff_t));
	return ret;
}
void cesk_diff_free(cesk_diff_t* diff)
{
	if(NULL == diff) return;
	cesk_diff_item_t* ptr;
	for(ptr = diff->head; NULL != ptr;)
	{
		cesk_diff_item_t* node = ptr;
		ptr = ptr->next;
		free(node);
	}
}
int cesk_diff_push_back(cesk_diff_t* diff, cesk_diff_item_t* item)
{
	if(NULL == diff || NULL == item)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	item->next = NULL;
	item->prev = diff->tail;
	diff->tail = item;
	if(NULL == diff->head) diff->head = item;
	return 0;
}
int cesk_diff_push_front(cesk_diff_t* diff, cesk_diff_item_t* item)
{
	if(NULL == diff || NULL == item)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	item->next = diff->head;
	item->prev = NULL;
	diff->head = item;
	if(NULL == diff->tail) diff->tail = item;
	return 0;
}
int cesk_diff_reduce(cesk_diff_t* diff)
{
	if(NULL == diff)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(1 == diff->reduced)
	{
		LOG_NOTICE("the diff is reduced");
		return 0;
	}

	//TODO use a hash table
	return 0;
}
