#include <anadroid.h>
#include <cesk/cesk_store.h>
int main()
{
    anadroid_init();
    
    dalvik_loader_from_directory("../testdata/AndroidAntlr");
    dalvik_loader_summary();
    cesk_value_t* objval = cesk_value_from_classpath(stringpool_query("antlr/ANTLRTokdefParser"));

    cesk_store_t* store = cesk_store_empty_store();

    cesk_store_t* store2 = cesk_store_fork(store);

    uint32_t    addr = cesk_store_allocate(&store2);

    cesk_store_attach(store2, addr, objval);
    anadroid_finalize();
    return 0;
}
