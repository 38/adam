#include <adam.h>
#include <assert.h>
int main()
{
	uint32_t result[10];
	int rc;
	adam_init();
	/* load package */
	dalvik_loader_from_directory("test/cases/block_analyzer");
	/* get code blocks */
	const dalvik_type_t  * const type[] = {NULL};
	dalvik_block_t* block = dalvik_block_from_method(stringpool_query("testClass"), stringpool_query("case1"), type);
	assert(block != NULL);
	/* setup 'this' pointer */
	cesk_block_t* ablock = cesk_block_graph_new(block);
	uint32_t self = cesk_frame_store_new_object(ablock->input, dalvik_instruction_get(0), stringpool_query("testClass"));
	assert(self != CESK_STORE_ADDR_NULL);
	cesk_frame_register_load(ablock->input, dalvik_instruction_get(0),CESK_FRAME_GENERAL_REG(0), self); 
	
	rc = cesk_frame_register_peek(ablock->input, CESK_FRAME_GENERAL_REG(0), result, sizeof(result)/sizeof(result[0]));
	assert(rc == 1);
	assert(result[0] != CESK_STORE_ADDR_NULL);
	
	assert(NULL != ablock);
	/* run */
	cesk_frame_t* output = cesk_block_interpret(ablock);
	assert(NULL != output);
	/* check the result */
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(0), result, sizeof(result)/sizeof(result[0]));
	assert(rc == 1);
	assert(result[0] != CESK_STORE_ADDR_NULL);
	adam_finalize();
	return 0;
}
