#include <assert.h>
#include <adam.h>
int main()
{
	adam_init();
	/* first try to make a empty tag set */
	tag_set_t* ts1 = tag_set_empty();
	assert(NULL != ts1);
	assert(0 == tag_set_size(ts1));
	assert(tag_set_hashcode(ts1) == tag_set_compute_hashcode(ts1));
	tag_set_free(ts1);
	/* then we try to construct a tag set from array */
	uint32_t tagid[] = {TAG_FILENAME};
	uint32_t resol[] = {TAG_FULL};
	tag_set_t* ts2 = tag_set_from_array(tagid, resol, 1);
	assert(NULL != ts2);
	assert(1 == tag_set_size(ts2));
	assert(TAG_FILENAME == tag_set_get_tagid(ts2, 0));
	assert(TAG_FULL == tag_set_get_resol(ts2, 0));
	assert(tag_set_hashcode(ts2) == tag_set_compute_hashcode(ts2));

	uint32_t tagid1[] = {TAG_FILENAME, TAG_FILECONTENT};
	uint32_t resol1[] = {TAG_PARTIAL, TAG_PARTIAL};
	tag_set_t* ts3 = tag_set_from_array(tagid1, resol1, 2);
	assert(NULL != ts3);
	assert(2 == tag_set_size(ts3));
	assert(TAG_FILENAME == tag_set_get_tagid(ts3, 0));
	assert(TAG_PARTIAL == tag_set_get_resol(ts3, 0));
	assert(TAG_FILECONTENT == tag_set_get_tagid(ts3, 1));
	assert(TAG_PARTIAL == tag_set_get_resol(ts3, 1));
	assert(tag_set_hashcode(ts3) == tag_set_compute_hashcode(ts3));

	tag_set_t* ts4 = tag_set_merge(ts2, ts3);
	assert(tag_set_hashcode(ts4) == tag_set_compute_hashcode(ts4));
	assert(NULL != ts4);
	assert(!tag_set_equal(ts3, ts4));
	
	uint32_t tagid2[] = {TAG_FILENAME, TAG_FILECONTENT};
	uint32_t resol2[] = {TAG_FULL, TAG_PARTIAL};
	tag_set_t* ts5 = tag_set_from_array(tagid2, resol2, 2);
	assert(2 == tag_set_size(ts5));
	assert(tag_set_equal(ts5, ts4));

	assert(0 == tag_set_change_resolution(ts5, TAG_FILENAME, TAG_PARTIAL));
	assert(tag_set_equal(ts5, ts3));

	tag_set_free(ts4);
	tag_set_free(ts5);
	tag_set_free(ts3);
	tag_set_free(ts2);
	adam_finalize();
	return 0;
}
