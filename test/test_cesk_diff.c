#include <adam.h>
#include <assert.h>
void test_cesk_diff_reduce()
{
	cesk_diff_t* diff = cesk_diff_new();
	cesk_diff_item_t* item;

	item = cesk_diff_item_new();
	assert(NULL != item);
	item->type = CESK_DIFF_ITEM_TYPE_SET;
	item->frame_addr.type = CESK_DIFF_FRAME_REGISTER;
	item->frame_addr.value = 0;
	item->data.value = cesk_value_empty_set();
	cesk_value_incref(item->data.value);
	assert(NULL != item->data.value);
	assert(0 == cesk_diff_push_back(diff, item));


	item = cesk_diff_item_new();
	assert(NULL != item);
	item->type = CESK_DIFF_ITEM_TYPE_ALLOCATE;
	item->frame_addr.type = CESK_DIFF_FRAME_STORE;
	item->frame_addr.value = CESK_STORE_ADDR_RELOC_PREFIX + 1;
	assert(0 == cesk_diff_push_back(diff, item));


	item = cesk_diff_item_new();
	assert(NULL != item);
	item->type = CESK_DIFF_ITEM_TYPE_SET;
	item->frame_addr.type = CESK_DIFF_FRAME_REGISTER;
	item->frame_addr.value = 0;
	item->data.value = cesk_value_empty_set();
	cesk_value_incref(item->data.value);
	assert(NULL != item->data.value);
	assert(0 == cesk_set_push(item->data.value->pointer.set, 1));
	assert(0 == cesk_diff_push_back(diff, item));

	assert(0 == cesk_diff_reduce(diff));

	cesk_diff_free(diff);
}
int main()
{
	adam_init();
	test_cesk_diff_reduce();
	adam_finalize();
	return 0;
}
