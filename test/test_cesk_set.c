#include <adam.h>
#include <cesk/cesk_set.h>
#include <assert.h>
int main()
{
    adam_init();
    cesk_set_t* set1 = cesk_set_empty_set();    /* S1 = {} */
    assert(NULL != set1);
    assert(0 == cesk_set_push(set1, 123));      /* S1 = {123} */
	assert(cesk_set_hashcode(set1) == cesk_set_compute_hashcode(set1));

    cesk_set_t* set2 = cesk_set_fork(set1);     /* S1 = {123} S2 = {123} */
    assert(NULL != set2);
	assert(cesk_set_hashcode(set2) == cesk_set_compute_hashcode(set2));
    assert(0 == cesk_set_push(set2, 789));      /* S1 = {123} S2 = {123,789} */
	assert(cesk_set_hashcode(set2) == cesk_set_compute_hashcode(set2));

    cesk_set_t* set3 = cesk_set_fork(set1);     /* S1 = {123} S2 = {123,789} S3 = {123} */
    assert(NULL != set3);
	assert(cesk_set_hashcode(set3) == cesk_set_compute_hashcode(set3));
    
    assert(0 == cesk_set_push(set1, 456));      /* S1 = {123,456} S2 = {123,789} S3 = {123} */
	assert(cesk_set_hashcode(set1) == cesk_set_compute_hashcode(set1));

    assert(0 == cesk_set_merge(set1, set2));     /* S1 = {123,456,789} S2 = {123,789} S3 = {123} */
	assert(cesk_set_hashcode(set1) == cesk_set_compute_hashcode(set1));

    assert(3 == cesk_set_size(set1));
	assert(cesk_set_hashcode(set1) == cesk_set_compute_hashcode(set1));

    assert(2 == cesk_set_size(set2));
	assert(cesk_set_hashcode(set2) == cesk_set_compute_hashcode(set2));

    assert(1 == cesk_set_size(set3));
	assert(cesk_set_hashcode(set3) == cesk_set_compute_hashcode(set3));

    assert(1 == cesk_set_contain(set1, 123));
    assert(1 == cesk_set_contain(set1, 456));
    assert(1 == cesk_set_contain(set1, 789));
    assert(0 == cesk_set_contain(set1, 321));

    assert(1 == cesk_set_contain(set2, 123));
    assert(0 == cesk_set_contain(set2, 456));
    assert(1 == cesk_set_contain(set2, 789));
    assert(0 == cesk_set_contain(set2, 321));


    assert(1 == cesk_set_contain(set3, 123));
    assert(0 == cesk_set_contain(set3, 456));
    assert(0 == cesk_set_contain(set3, 789));
    assert(0 == cesk_set_contain(set3, 321));

    LOG_ERROR("cesk_hashcode: %08x", cesk_set_hashcode(set1));
    LOG_ERROR("cesk_compute_hashcode: %08x", cesk_set_compute_hashcode(set1));
    LOG_ERROR("cesk_hashcode: %08x", cesk_set_hashcode(set2));
    LOG_ERROR("cesk_compute_hashcode: %08x", cesk_set_compute_hashcode(set2));
    LOG_ERROR("cesk_hashcode: %08x", cesk_set_hashcode(set3));
    LOG_ERROR("cesk_compute_hashcode: %08x", cesk_set_compute_hashcode(set3));

    cesk_set_t* set4 = cesk_set_empty_set();
    assert(0 == cesk_set_push(set4, 123));    /* S4 = {123} */
    assert(1 == cesk_set_equal(set3, set4));  
    assert(0 == cesk_set_equal(set3, set1));
    assert(0 == cesk_set_push(set4, 789));     /* S4 = {123, 789} */
    assert(1 == cesk_set_equal(set4, set2));
    assert(0 == cesk_set_push(set4, 456));   /* S4 = {123,456,789} */
    assert(1 == cesk_set_equal(set4,set1));   

	assert(cesk_set_compute_hashcode(set1) == cesk_set_hashcode(set1));
	assert(cesk_set_compute_hashcode(set2) == cesk_set_hashcode(set2));
	assert(cesk_set_compute_hashcode(set3) == cesk_set_hashcode(set3));
	assert(cesk_set_compute_hashcode(set4) == cesk_set_hashcode(set4));

	assert(0 == cesk_set_modify(set4, 456 , 111));
	assert(1 == cesk_set_contain(set4, 111));
	assert(0 == cesk_set_contain(set4, 456));
    
    cesk_set_free(set1);
    cesk_set_free(set2);
    cesk_set_free(set3);
	cesk_set_free(set4);



    adam_finalize();
    return 0;
}
