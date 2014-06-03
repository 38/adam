#include <adam.h>
#include <assert.h>
void case1()
{
	cesk_reloc_table_t* rtab;
	cesk_alloctab_t*    atab;
	/* set up environment */
	rtab = cesk_reloc_table_new();
	assert(NULL != rtab);
	atab = cesk_alloctab_new();
	assert(NULL != atab);
	
	int rc;
	uint32_t buf[10];
	/* get code blocks */
	const dalvik_type_t  * const type[] = {NULL};
	dalvik_block_t* block = dalvik_block_from_method(stringpool_query("testClass"), stringpool_query("case1"), type);
	assert(block != NULL);
	/* build a stack frame */
	cesk_frame_t* frame = cesk_frame_new(block->nregs);
	assert(NULL != frame);
	cesk_frame_set_alloctab(frame, atab);
	/* run the block */
	cesk_block_result_t result;
	assert(0 == cesk_block_analyze(block, frame, rtab, &result)); 
	/* check the result */
	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(0), buf, 10);
	assert(rc == 1);
	assert(buf[0] != CESK_STORE_ADDR_NULL);

	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(1), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_POS);
	
	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(2), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_POS);
	
	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(3), buf, 10);
	assert(rc == 1);
	assert(CESK_STORE_ADDR_CONST_CONTAIN(buf[0], NEG));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(buf[0], POS));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(buf[0], ZERO));

	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(4), buf, 10);
	assert(rc == 1);
	assert(CESK_STORE_ADDR_CONST_CONTAIN(buf[0], NEG));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(buf[0], POS));
	assert(CESK_STORE_ADDR_CONST_CONTAIN(buf[0], ZERO));

	cesk_frame_t* frame2 = cesk_frame_new(block->nregs);
	uint32_t empty_hash = cesk_frame_hashcode(frame2);
	assert(NULL != frame2);
	cesk_frame_set_alloctab(frame2, atab);
	assert(cesk_frame_apply_diff(frame2, result.diff, rtab, NULL, NULL) > 0);
	assert(cesk_frame_hashcode(frame) == cesk_frame_hashcode(frame2));
	assert(cesk_frame_apply_diff(frame2, result.inverse, rtab, NULL, NULL) > 0);
	assert(cesk_frame_hashcode(frame2) == empty_hash);
	assert(0 == cesk_frame_apply_diff(frame2, result.inverse, rtab, NULL, NULL));
	/* clean up */
	cesk_frame_free(frame);
	cesk_frame_free(frame2);
	cesk_diff_free(result.diff);
	cesk_diff_free(result.inverse);
	cesk_reloc_table_free(rtab);
	cesk_alloctab_free(atab);
}
void case2()
{
	cesk_reloc_table_t* rtab;
	cesk_alloctab_t*    atab;
	/* set up environment */
	rtab = cesk_reloc_table_new();
	assert(NULL != rtab);
	atab = cesk_alloctab_new();
	assert(NULL != atab);

	uint32_t buf[10];
	int rc;
	/* get code blocks */
	const dalvik_type_t  * const type[] = {NULL};
	dalvik_block_t* block = dalvik_block_from_method(stringpool_query("testClass"), stringpool_query("case2"), type);
	assert(block != NULL);
	
	/* build a stack frame */
	cesk_frame_t* frame = cesk_frame_new(block->nregs);
	assert(NULL != frame);
	cesk_frame_set_alloctab(frame, atab);
	
	/* run the block */
	cesk_block_result_t result;
	assert(0 == cesk_block_analyze(block, frame, rtab, &result)); 
	
	/* verify the registers */
	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(0), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_ZERO);

	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(1), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_ZERO);

	
	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(2), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_ZERO);
	assert(0 == cesk_frame_gc(frame));


	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(3), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_ZERO);

	/* after this, all object is cleaned */
	assert(0 == frame->store->num_ent);


	cesk_frame_t* frame2 = cesk_frame_new(block->nregs);
	uint32_t empty_hash = cesk_frame_hashcode(frame2);
	assert(NULL != frame2);
	cesk_frame_set_alloctab(frame2, atab);
	assert(cesk_frame_apply_diff(frame2, result.diff, rtab, NULL, NULL) > 0);
	assert(0 == cesk_frame_gc(frame2));
	assert(cesk_frame_hashcode(frame) == cesk_frame_hashcode(frame2));
	assert(cesk_frame_apply_diff(frame2, result.inverse, rtab, NULL, NULL) > 0);
	assert(cesk_frame_hashcode(frame2) == empty_hash);
	assert(0 == cesk_frame_apply_diff(frame2, result.inverse, rtab, NULL, NULL));

	/* clean up */
	cesk_frame_free(frame);
	cesk_frame_free(frame2);
	cesk_diff_free(result.diff);
	cesk_diff_free(result.inverse);
	cesk_reloc_table_free(rtab);
	cesk_alloctab_free(atab);

}
void case3()
{
	cesk_reloc_table_t* rtab;
	cesk_alloctab_t*    atab;
	/* set up environment */
	rtab = cesk_reloc_table_new();
	assert(NULL != rtab);
	atab = cesk_alloctab_new();
	assert(NULL != atab);

	uint32_t buf[10];
	int rc;
	/* get code blocks */
	const dalvik_type_t  * const type[] = {NULL};
	dalvik_block_t* block = dalvik_block_from_method(stringpool_query("testClass"), stringpool_query("case3"), type);
	assert(block != NULL);
	
	/* build a stack frame */
	cesk_frame_t* frame = cesk_frame_new(block->nregs);
	assert(NULL != frame);
	cesk_frame_set_alloctab(frame, atab);
	
	/* run the block */
	cesk_block_result_t result;
	assert(0 == cesk_block_analyze(block, frame, rtab, &result)); 
	
	/* verify the registers */
	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(0), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_POS);

	rc = cesk_frame_register_peek(frame, CESK_FRAME_GENERAL_REG(1), buf, 10);
	assert(rc == 1);
	assert(buf[0] == CESK_STORE_ADDR_NEG);
	

	cesk_frame_t* frame2 = cesk_frame_new(block->nregs);
	uint32_t empty_hash = cesk_frame_hashcode(frame2);
	assert(NULL != frame2);
	cesk_frame_set_alloctab(frame2, atab);
	assert(cesk_frame_apply_diff(frame2, result.diff, rtab, NULL, NULL) > 0);
	assert(0 == cesk_frame_gc(frame2));
	assert(cesk_frame_hashcode(frame) == cesk_frame_hashcode(frame2));
	assert(cesk_frame_apply_diff(frame2, result.inverse, rtab, NULL, NULL) > 0);
	assert(cesk_frame_hashcode(frame2) == empty_hash);
	assert(0 == cesk_frame_apply_diff(frame2, result.inverse, rtab, NULL, NULL));
	/* clean up */
	cesk_frame_free(frame);
	cesk_frame_free(frame2);
	cesk_diff_free(result.diff);
	cesk_diff_free(result.inverse);
	cesk_reloc_table_free(rtab);
	cesk_alloctab_free(atab);

}
int main()
{
	adam_init();
	/* load package */
	dalvik_loader_from_directory("test/cases/block_analyzer");
	
	
	/* run test cases */
	case1();

	case2();

	case3();

	/* finalize */
	adam_finalize();
	return 0;
}
