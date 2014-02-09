#include <adam.h>
#include <cesk/cesk_set.h>
#include <assert.h>
int main()
{
    adam_init();
    cesk_set_t* set1 = cesk_set_empty_set();
    assert(NULL != set1);
    assert(0 == cesk_set_push(set1, 123));

    cesk_set_t* set2 = cesk_set_fork(set1);
    assert(NULL != set2);
    assert(0 == cesk_set_push(set2, 789));

    cesk_set_t* set3 = cesk_set_fork(set2);
    assert(NULL != set3);
    
    assert(0 == cesk_set_push(set1, 456));

    assert(0 == cesk_set_join(set1, set2));

    cesk_set_free(set1);
    cesk_set_free(set2);
    cesk_set_free(set3);



    adam_finalize();
    return 0;
}
