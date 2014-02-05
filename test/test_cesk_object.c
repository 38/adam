#include <anadroid.h>
#include <cesk/cesk_object.h>
#include <assert.h>
int main()
{
    anadroid_init();
    dalvik_loader_from_directory("../testdata/AndroidAntlr");
    dalvik_loader_summary();
    cesk_object_t* object = cesk_object_new(stringpool_query("antlr/ANTLRTokdefParser"));
    cesk_value_set_t** res1 = cesk_object_get(object, stringpool_query("antlr/Parser"), stringpool_query("tokenName"));
    assert(NULL == res1);
    cesk_value_set_t** res2 = cesk_object_get(object, stringpool_query("antlr/Parser"), stringpool_query("tokenNames"));
    assert(NULL != res2);
    assert(NULL == *res2);
    anadroid_finalize();
    return 0;
}
