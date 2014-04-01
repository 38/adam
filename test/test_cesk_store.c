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

	uint32_t    addr = cesk_store_allocate(&store2, ins, 0, 0);

	cesk_store_attach_oa(store2, addr, objval);  //object val
	cesk_store_release_rw(store2, addr);

	cesk_store_t* store3 = cesk_store_fork(store2);

	LOG_ERROR("hash code of store1 : %x", cesk_store_hashcode(store));
	LOG_ERROR("hash code of store2 : %x", cesk_store_hashcode(store2));
	LOG_ERROR("hash code of store3 : %x", cesk_store_hashcode(store3));

	cesk_value_const_t* value_ro = cesk_store_get_ro(store3, addr);

	assert(1 == cesk_store_equal(store2, store3));

	cesk_value_t* value_rw = cesk_store_get_rw(store3, addr, 0);

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

	/* now we test store with allocation table */
	cesk_store_t* store4 = cesk_store_empty_store();
	cesk_alloc_table_t* alloc_tab = cesk_alloc_table_new();
	assert(NULL != store4);
	assert(NULL != alloc_tab);
	assert(cesk_store_set_alloc_table(store4, alloc_tab) == 0);
	objval = cesk_value_from_classpath(stringpool_query("antlr/ANTLRTokdefParser"));

	uint32_t oa = cesk_store_allocate(&store4, ins , 0, 0);
	assert(oa != CESK_STORE_ADDR_NULL);
	assert(cesk_alloc_table_insert(alloc_tab, store4, CESK_STORE_ADDR_RELOC_PREFIX, oa) == 0);
	assert(cesk_alloc_table_insert(alloc_tab, store4, oa, CESK_STORE_ADDR_RELOC_PREFIX) == 0);
	
	
	object->members[0].valuelist[0] = CESK_STORE_ADDR_RELOC_PREFIX + 1;   /* setup an relocated address to it */
	cesk_value_set_reloc(objval);

	assert(cesk_store_attach_oa(store4, oa, objval) == 0);
	cesk_store_release_rw(store4, oa);

	/* test the relocation logic */
	cesk_value_const_t* a = cesk_store_get_ro(store4, CESK_STORE_ADDR_RELOC_PREFIX);
	cesk_value_const_t* b = cesk_store_get_ro(store4, oa);

	assert(a == b);
	assert(NULL != a);

	/* test the fork function */
	cesk_value_t* new_val = cesk_value_from_classpath(stringpool_query("antlr/ANTLRTokdefParser"));
	assert(NULL != new_val);
	cesk_value_t* set_val = cesk_value_empty_set();
	assert(NULL != set_val);

	/* push the relocated address to the set */
	assert(-1 != cesk_set_push(set_val->pointer.set, CESK_STORE_ADDR_RELOC_PREFIX + 2));

	object = objval->pointer.object;
	assert(NULL != object);
	object->members[0].valuelist[0] = CESK_STORE_ADDR_RELOC_PREFIX | 1; // this is the set object

	uint32_t oa1 = cesk_store_allocate(&store4, ins, CESK_STORE_ADDR_RELOC_PREFIX, CESK_OBJECT_FIELD_OFS(object, object->members[0].valuelist));
	uint32_t oa2 = cesk_store_allocate(&store4, ins, 0, 12345);

	assert(CESK_STORE_ADDR_NULL != oa1);
	assert(CESK_STORE_ADDR_NULL != oa2);


	assert(0 == cesk_alloc_table_insert(alloc_tab, store4, oa1, CESK_STORE_ADDR_RELOC_PREFIX + 1));
	assert(0 == cesk_alloc_table_insert(alloc_tab, store4,      CESK_STORE_ADDR_RELOC_PREFIX + 1, oa1));
	assert(0 == cesk_alloc_table_insert(alloc_tab, store4, oa2, CESK_STORE_ADDR_RELOC_PREFIX + 2));
	assert(0 == cesk_alloc_table_insert(alloc_tab, store4,      CESK_STORE_ADDR_RELOC_PREFIX + 2, oa2));
	
	cesk_value_set_reloc(set_val);
	
	assert(0 == cesk_store_attach_oa(store4, oa1, set_val));
	assert(0 == cesk_store_attach_oa(store4, oa2, new_val));
	cesk_store_release_rw(store4, oa1);
	cesk_store_release_rw(store4, oa2);

	cesk_store_t* store5 = cesk_store_fork(store4);

	assert(NULL != store5);

	/* after we copy the content in store4, the relocation should be applied, so check it */
	cesk_value_const_t* conval = cesk_store_get_ro(store5, oa1);
	assert(1 == cesk_set_size(conval->pointer.set));
	assert(1 == cesk_set_contain(conval->pointer.set, oa2));


	cesk_store_free(store4);
	cesk_alloc_table_free(alloc_tab);
	cesk_store_free(store5);

	adam_finalize();
	return 0;
} 
