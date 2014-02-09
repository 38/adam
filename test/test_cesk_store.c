#include <assert.h>
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

    sexpression_t* sexp;
    sexp_parse("(new-instance v1, antlr/ANTLRTokdefParser)", &sexp);
    dalvik_instruction_t* ins = dalvik_instruction_new();
    dalvik_instruction_from_sexp(sexp, ins, 0);
    sexp_free(sexp);

    uint32_t    addr = cesk_store_allocate(&store2, ins);

    cesk_store_attach(store2, addr, objval);
    
    addr = cesk_store_allocate(&store2, ins);

    cesk_store_t* store3 = cesk_store_fork(store2);

    const cesk_value_t* value_ro = cesk_store_get_ro(store3, addr);

    cesk_value_t* value_rw = cesk_store_get_rw(store3, addr);

    assert(value_ro != value_rw);
    cesk_store_free(store);
    cesk_store_free(store2);
    cesk_store_free(store3);

    anadroid_finalize();
    return 0;
}
