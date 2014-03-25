#include <adam.h>
#include <assert.h>
void case1()
{
	uint32_t result[10];
	int rc;
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
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(0), result, 10);
	assert(rc == 1);
	assert(result[0] != CESK_STORE_ADDR_NULL);
	
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(1), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_POS);
	
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(2), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_POS);
	
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(3), result, 10);
	assert(rc == 1);
	assert(CESK_STORE_ADDR_CONST_CONTAIN(result[0], NEG));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(result[0], POS));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(result[0], ZERO));

	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(4), result, 10);
	assert(rc == 1);
	assert(CESK_STORE_ADDR_CONST_CONTAIN(result[0], NEG));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(result[0], POS));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(result[0], ZERO));

	cesk_frame_free(output);
	cesk_block_graph_free(ablock);
}
void case2()
{
	uint32_t result[10];
	int rc;
	/* get code blocks */
	const dalvik_type_t  * const type[] = {NULL};
	dalvik_block_t* block = dalvik_block_from_method(stringpool_query("testClass"), stringpool_query("case2"), type);
	assert(block != NULL);
	/* create a new analyzer graph */
	cesk_block_t* ablock = cesk_block_graph_new(block);
	assert(NULL != ablock);
	/* run */
	cesk_frame_t* output = cesk_block_interpret(ablock);
	assert(NULL != output);
	
	/* verify the registers */
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(0), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_ZERO);

	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(1), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_ZERO);

	
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(2), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_ZERO);
	assert(0 == cesk_frame_gc(output));


	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(3), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_TRUE);

	/* after this, all object is cleaned */
	assert(0 == output->store->num_ent);

	cesk_frame_free(output);
	cesk_block_graph_free(ablock);
}
void case3()
{
	uint32_t result[10];
	int rc;
	/* get code blocks */
	const dalvik_type_t  * const type[] = {NULL};
	dalvik_block_t* block = dalvik_block_from_method(stringpool_query("testClass"), stringpool_query("case3"), type);
	assert(block != NULL);
	/* create a new analyzer graph */
	cesk_block_t* ablock = cesk_block_graph_new(block);
	assert(NULL != ablock);
	/* run */
	cesk_frame_t* output = cesk_block_interpret(ablock);
	assert(NULL != output);
	
	/* verify the registers */
	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(0), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_POS);

	rc = cesk_frame_register_peek(output, CESK_FRAME_GENERAL_REG(1), result, 10);
	assert(rc == 1);
	assert(result[0] == CESK_STORE_ADDR_NEG);

	cesk_frame_free(output);
	cesk_block_graph_free(ablock);
}
int main()
{
	adam_init();
	/* load package */
	dalvik_loader_from_directory("test/cases/block_analyzer");
	case1();
	case2();
	adam_finalize();
	return 0;
}
