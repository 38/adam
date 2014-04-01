#include <assert.h>
#include <adam.h>
int main()
{
	adam_init();
	int i;
	cesk_store_t* store = cesk_store_empty_store(); 
	assert(NULL != store);
	cesk_alloctab_t* tab = cesk_alloctab_new();
	assert(NULL != tab);
	/* insert 10000 address pair */
	for(i = 0; i < 10000; i ++)
	{
		uint32_t obj_addr = i;
		uint32_t rel_addr = (CESK_STORE_ADDR_RELOC_PREFIX | (i + 1));
		assert(cesk_alloctab_insert(tab, store, obj_addr, rel_addr) >= 0);
		assert(cesk_alloctab_insert(tab, store, rel_addr, obj_addr) >= 0);
	}
	/* try to retrive the 10000 address */
	for(i = 0; i < 10000; i ++)
	{
		uint32_t obj_addr = i;
		uint32_t rel_addr = (CESK_STORE_ADDR_RELOC_PREFIX | (i + 1));
		assert(cesk_alloctab_query(tab, store, obj_addr) == rel_addr);
		assert(cesk_alloctab_query(tab, store, rel_addr) == obj_addr);
	}
	adam_finalize();
	return 0;
}
