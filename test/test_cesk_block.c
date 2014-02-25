#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	dalvik_loader_from_directory("test/cases/block_analyzer");
	const dalvik_type_t  * const type[] = {NULL};
	dalvik_block_t* block = dalvik_block_from_method(stringpool_query("testClass"), stringpool_query("case1"), type);
	assert(block != NULL);
	cesk_block_t* ablock = cesk_block_graph_new(block);
	uint32_t self = cesk_frame_store_new_object(ablock->input, dalvik_instruction_get(0), stringpool_query("testClass"));
	assert(self != CESK_STORE_ADDR_NULL);
	cesk_frame_register_load(ablock->input, dalvik_instruction_get(0),CESK_FRAME_GENERAL_REG(0), self); 
	assert(NULL != ablock);
	assert(0 == cesk_block_analyze(ablock));
	adam_finalize();
	return 0;
}
