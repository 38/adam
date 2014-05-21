#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	/* load the test data */
	assert(0 == dalvik_loader_from_directory("test/data"));
	/*test the buffer */
	cesk_diff_buffer_t* buf = cesk_diff_buffer_new(0);
	assert(NULL != buf);
	/* try to append one */
	cesk_set_t* set0 = cesk_set_empty_set();
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REG, 3, set0));
	/* try to make a new set, and set it to the resiger*/
	cesk_set_t* set1 = cesk_set_empty_set();
	assert(NULL != set1);
	assert(0 == cesk_set_push(set1, 1));
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REG, 3, set1));
	/* put an allocation there */
	cesk_value_t* objval1 = cesk_value_from_classpath(stringpool_query("antlr/ANTLRTokdefParser"));
	assert(NULL != objval1);
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_ALLOC, 255, objval1));
	/* then try to set another value to that */
	cesk_set_t* set2 = cesk_set_empty_set();
	assert(NULL != set2);
	assert(0 == cesk_set_push(set2, 2));
	assert(0 == cesk_set_push(set2, 3));
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REG, 3, set2));
	/* then we set the reuse flag */
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REUSE, 255, CESK_DIFF_REUSE_VALUE(1)));
	/* finally we deallocate it */
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_DEALLOC, 255, NULL));
	/* ok, let convert it to a actuall diff */
	cesk_diff_t* diff0 = cesk_diff_from_buffer(buf);
	cesk_diff_buffer_free(buf);
	cesk_set_free(set0);
	cesk_set_free(set1);
	cesk_set_free(set2);
	assert(NULL != diff0);
	assert(diff0->offset[CESK_DIFF_NTYPES] == 4); 

	/* now we test the apply function */
	cesk_diff_t* input[10] = {diff0};
	cesk_diff_t* diff1 = cesk_diff_apply(1, input);
	assert(diff1 != NULL);
	adam_finalize();
	return 0;
}
