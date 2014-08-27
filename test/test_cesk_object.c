#include <adam.h>
#include <cesk/cesk_object.h>
#include <assert.h>
int main()
{
	adam_init();
	dalvik_loader_from_directory("test/data/AndroidAntlr");
	dalvik_loader_summary();
	
	/* test creating a new object */
	cesk_object_t* object = cesk_object_new(stringpool_query("antlr/ANTLRTokdefParser"));
	uint32_t* res1 = cesk_object_get(object, stringpool_query("antlr/Parser"), stringpool_query("tokenName"),NULL,NULL);
	assert(NULL == res1);

	/* now try to get a field */
	uint32_t* res2 = cesk_object_get(object, stringpool_query("antlr/Parser"), stringpool_query("tokenNames"),NULL,NULL);
	assert(res2 != NULL);
	assert(*res2 == 0xfffffffful);
	LOG_ERROR("object dump: %s", cesk_object_to_string(object, NULL, 0, 0));
	LOG_ERROR("hash value: %x", cesk_object_hashcode(object));
	assert(cesk_object_hashcode(object) == cesk_object_compute_hashcode(object));

	/* try to clone it */
	cesk_object_t* object_clone = cesk_object_duplicate(object);
	assert(NULL != object_clone);
	LOG_ERROR("cloned object dump: %s", cesk_object_to_string(object, NULL, 0, 0));
	LOG_ERROR("hash value of cloned object: %x", cesk_object_hashcode(object_clone));
	assert(cesk_object_hashcode(object_clone) == cesk_object_compute_hashcode(object_clone));
	assert(cesk_object_hashcode(object) == cesk_object_hashcode(object_clone));

	/* try to compare two object */
	assert(1 == cesk_object_equal(object, object_clone));

	/* try to modify the field */
	*res2 = 0;
	LOG_ERROR("new hash value: %x", cesk_object_hashcode(object));
	
	/* compare it again */
	assert(0 == cesk_object_equal(object, object_clone));

	/* now also modify the cloned one */
	res2 = cesk_object_get(object_clone, stringpool_query("antlr/Parser"), stringpool_query("tokenNames"),NULL,NULL);
	assert(NULL != res2);
	assert(0xfffffffful == *res2);
	*res2 = 0;
	LOG_ERROR("new has value of cloned object: %x", cesk_object_hashcode(object_clone));

	/* noew compare it again */
	assert(1 == cesk_object_equal(object, object_clone));

	cesk_object_free(object);
	
	/* test 2 */
	object = cesk_object_new(stringpool_query("antlr/ANTLRLexer"));
	assert(NULL != object);
	LOG_ERROR("hash value: %x", cesk_object_hashcode(object));
	/* compare two object with different class path */
	assert(0 == cesk_object_equal(object, object_clone));

	cesk_object_free(object_clone);
	cesk_object_free(object);
	adam_finalize();
	return 0;
}
