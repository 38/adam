#include <adam.h>
#include <cesk/cesk_object.h>
#include <assert.h>
int main()
{
    adam_init();
    dalvik_loader_from_directory("../testdata/AndroidAntlr");
    dalvik_loader_summary();
    cesk_object_t* object = cesk_object_new(stringpool_query("antlr/ANTLRTokdefParser"));
    uint32_t* res1 = cesk_object_get(object, stringpool_query("antlr/Parser"), stringpool_query("tokenName"));
    assert(NULL == res1);
    uint32_t* res2 = cesk_object_get(object, stringpool_query("antlr/Parser"), stringpool_query("tokenNames"));
    assert(res2 != NULL);
    assert(*res2 == 0xfffffffful);
    cesk_object_free(object);
    adam_finalize();
    return 0;
}
