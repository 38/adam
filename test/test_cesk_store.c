#include <assert.h>
#include <adam.h>
#include <cesk/cesk_store.h>
int main()
{
    adam_init();
    
    dalvik_loader_from_directory("test/data/AndroidAntlr");
    dalvik_loader_summary();
    cesk_value_t* objval = cesk_value_from_classpath(stringpool_query("antlr/ANTLRTokdefParser"));

    cesk_store_t* store = cesk_store_empty_store();    /* empty store */

    cesk_store_t* store2 = cesk_store_fork(store);     /* empty store */

    sexpression_t* sexp;
    sexp_parse("(new-instance v1, antlr/ANTLRTokdefParser)", &sexp);
    dalvik_instruction_t* ins = dalvik_instruction_new();
    dalvik_instruction_from_sexp(sexp, ins, 0);
    sexp_free(sexp);

    uint32_t    addr = cesk_store_allocate(&store2, ins, 0, NULL);

    cesk_store_attach(store2, addr, objval);  //object val
	cesk_store_release_rw(store2, addr);
    
    cesk_store_t* store3 = cesk_store_fork(store2);

    LOG_ERROR("hash code of store1 : %x", cesk_store_hashcode(store));
    LOG_ERROR("hash code of store2 : %x", cesk_store_hashcode(store2));
    LOG_ERROR("hash code of store3 : %x", cesk_store_hashcode(store3));

    cesk_value_const_t* value_ro = cesk_store_get_ro(store3, addr);
    
	assert(1 == cesk_store_equal(store2, store3));

    cesk_value_t* value_rw = cesk_store_get_rw(store3, addr);

    assert((void*)value_ro != (void*)value_rw);

    //*value_rw = cesk_value_from_classpath(stringpool_query("antlr/ANTLRLexer"));
    cesk_object_t* object = value_rw->pointer.object;
    LOG_DEBUG("object dump: %s", cesk_object_to_string(object, NULL, 0));
    LOG_DEBUG("value hash: %x", cesk_value_hashcode(value_rw));
    LOG_DEBUG("object hash: %x", cesk_object_hashcode(object));
    *cesk_object_get(object, stringpool_query("antlr/ANTLRTokdefParser"), stringpool_query("antlrTool")) = 0;
    
	cesk_store_release_rw(store3, addr);
    
	LOG_DEBUG("object dump: %s", cesk_object_to_string(object, NULL, 0));
    LOG_DEBUG("value hash: %x", cesk_value_hashcode(value_rw));
    LOG_DEBUG("object hash: %x", cesk_object_hashcode(object));

    
	assert(0 == cesk_store_equal(store2, store3));
    assert(0 == cesk_store_equal(store, store2));


    LOG_ERROR("hash code of store1 : %x", cesk_store_hashcode(store));
    LOG_ERROR("hash code of store2 : %x", cesk_store_hashcode(store2));
    LOG_ERROR("hash code of store3 : %x", cesk_store_hashcode(store3));
    LOG_ERROR("object dump origin: %s", cesk_object_to_string(value_ro->pointer.object, NULL, 0));

	assert(cesk_store_hashcode(store) == cesk_store_compute_hashcode(store));
	assert(cesk_store_hashcode(store3) == cesk_store_compute_hashcode(store3));
	assert(cesk_store_hashcode(store2) == cesk_store_compute_hashcode(store2));

    cesk_store_free(store);
    cesk_store_free(store2);
    cesk_store_free(store3);

    adam_finalize();
    return 0;
} 
