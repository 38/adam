#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	dalvik_loader_from_directory("test/data/AndroidAntlr");

	/* build a new relocated object table */
	cesk_reloc_table_t* reloc_tab = cesk_reloc_table_new();
	assert(NULL != reloc_tab);

	/* make a new store that uses this global relocated object table */
	cesk_store_t* store1 = cesk_store_empty_store();
	assert(NULL != store1);

	/* allocate a allocation table */
	cesk_alloctab_t *alloc_tab = cesk_alloctab_new(NULL);
	assert(NULL != alloc_tab);

	/* set up the store */
	assert(0 <= cesk_store_set_alloc_table(store1, alloc_tab));
	
	/* make a dumb instruction for test */
	sexpression_t* sexp;
	sexp_parse("(new-instance v1, antlr/ANTLRTokdefParser)", &sexp);
	dalvik_instruction_t* ins = dalvik_instruction_new();
	dalvik_instruction_from_sexp(sexp, ins, 0);
	sexp_free(sexp);

	cesk_alloc_param_t param = CESK_ALLOC_PARAM(dalvik_instruction_get_index(ins), CESK_ALLOC_NA);
	/* okay, try to allocate a new relocated object */
	uint32_t ra1 = cesk_reloc_allocate(reloc_tab, store1, &param, 0);
	assert(CESK_STORE_ADDR_NULL != ra1);

	/* find out the object address */
	uint32_t oa1 = cesk_alloctab_query(alloc_tab, store1, ra1);
	assert(CESK_STORE_ADDR_NULL != oa1);

	/* assign this value */
	cesk_value_t* objval = cesk_value_from_classpath(stringpool_query("antlr/ANTLRTokdefParser"));
	assert(NULL != objval);
	assert(0 == cesk_store_attach(store1, ra1, objval));
	cesk_store_release_rw(store1, ra1);

	/* create a new store */
	cesk_store_t* store2 = cesk_store_empty_store();

	/* set another value there */
	param.offset = 1;
	uint32_t _unused = cesk_store_allocate(store2, &param);   /* just make the store allocate a new block */
	assert(CESK_STORE_ADDR_NULL != _unused);
	cesk_value_t* setval = cesk_value_empty_set();
	assert(NULL != setval);
	assert(0 == cesk_store_attach(store2, oa1, setval));
	cesk_store_release_rw(store2, oa1);

	/* setup the store */
	assert(0 == cesk_store_set_alloc_table(store2, alloc_tab));

	cesk_store_free(store1);
	cesk_store_free(store2);
	cesk_alloctab_free(alloc_tab);
	cesk_reloc_table_free(reloc_tab);
	adam_finalize();
	return 0;
}
