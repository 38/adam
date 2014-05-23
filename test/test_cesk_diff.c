#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	/* load the test data */
	assert(0 == dalvik_loader_from_directory("test/data"));
	/*test the buffer */
	cesk_diff_buffer_t* buf = cesk_diff_buffer_new(0);
	assert(NULL != buf);
	/* try to append one */
	cesk_set_t* set0 = cesk_set_empty_set();
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REG, 3, set0));
	/* try to make a new set, and set it to the resiger*/
	cesk_set_t* set1 = cesk_set_empty_set();
	assert(NULL != set1);
	assert(0 == cesk_set_push(set1, 1));
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REG, 3, set1));
	/* put an allocation there */
	cesk_value_t* objval1 = cesk_value_from_classpath(stringpool_query("antlr/ANTLRTokdefParser"));
	assert(NULL != objval1);
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_ALLOC, 255, objval1));
	/* then try to set another value to that */
	cesk_set_t* set2 = cesk_set_empty_set();
	assert(NULL != set2);
	assert(0 == cesk_set_push(set2, 2));
	assert(0 == cesk_set_push(set2, 3));
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REG, 3, set2));
	/* then we set the reuse flag */
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REUSE, 255, CESK_DIFF_REUSE_VALUE(1)));
	/* finally we deallocate it */
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_DEALLOC, 255, NULL));
	/* ok, let convert it to a actuall diff */
	cesk_diff_t* diff0 = cesk_diff_from_buffer(buf);
	cesk_diff_buffer_free(buf);
	assert(NULL != diff0);
	assert(diff0->offset[CESK_DIFF_NTYPES] == 4);
	assert(1 == cesk_set_contain(diff0->data[2].arg.set, 2));
	assert(1 == cesk_set_contain(diff0->data[2].arg.set, 3));


	/* now we test the apply function */
	cesk_diff_t* input[10] = {diff0};
	cesk_diff_t* diff1 = cesk_diff_apply(1, input);
	assert(diff1 != NULL);
	assert(diff1->offset[CESK_DIFF_NTYPES] - diff1->offset[0] == 2);

	/* create another diff */
	buf = cesk_diff_buffer_new(0);
	assert(NULL != buf);
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_REG, 3, set1));
	assert(0 == cesk_diff_buffer_append(buf, CESK_DIFF_STORE, 0x123, cesk_value_empty_set()));
	cesk_diff_t* diff2 = cesk_diff_from_buffer(buf);
	cesk_diff_buffer_free(buf);
	assert(NULL != diff2);
	assert(diff2->offset[CESK_DIFF_NTYPES] == 2);

	/*apply them */
	input[0] = diff0;
	input[1] = diff2;
	cesk_diff_t* diff3 = cesk_diff_apply(2, input);
	assert(diff3->offset[CESK_DIFF_NTYPES] - diff3->offset[0] == 3);

	cesk_diff_free(diff0);
	cesk_diff_free(diff1);
	cesk_diff_free(diff2);
	cesk_diff_free(diff3);

	assert(NULL != diff3);

	cesk_set_free(set0);
	cesk_set_free(set1);
	cesk_set_free(set2);

	/* now we test the factorize */
	/* setup 3 frames */
	cesk_frame_t* frame1 = cesk_frame_new(32);
	cesk_frame_t* frame2 = cesk_frame_new(32);
	cesk_frame_t* frame3 = cesk_frame_new(32);
	assert(NULL != frame1 && NULL != frame2 && NULL != frame3);
	
	cesk_alloctab_t* atab    = cesk_alloctab_new();
	cesk_reloc_table_t* rtab = cesk_reloc_table_new();
	
	cesk_frame_set_alloctab(frame1, atab);
	cesk_frame_set_alloctab(frame2, atab);
	cesk_frame_set_alloctab(frame3, atab);

	/* now we use 2 gourps of diff buffers to keep tracking the modification */
	cesk_diff_buffer_t *db1 = cesk_diff_buffer_new(0);
	cesk_diff_buffer_t *ib1 = cesk_diff_buffer_new(1);
	cesk_diff_buffer_t *db2 = cesk_diff_buffer_new(0);
	cesk_diff_buffer_t *ib2 = cesk_diff_buffer_new(1);

	assert(NULL != db1 && NULL != db2  &&
	       NULL != ib1 && NULL != ib2 );

	/* a stub instruction that used for testing */
	const dalvik_instruction_t* inst1 = dalvik_instruction_get(123);
	/* another stub instruction */
	const dalvik_instruction_t* inst2 = dalvik_instruction_get(124);
	/* the third instruction */
	const dalvik_instruction_t* inst3 = dalvik_instruction_get(125);
	const dalvik_instruction_t* inst4 = dalvik_instruction_get(126);
	/* the class path we want to set */
	const char* classpath = stringpool_query("antlr/ANTLRLexer");
	const char* superclass = stringpool_query("antlr/CharScanner");
	const char* field = stringpool_query("literals");

	/* to make frame1 and frame2 different first we make some untracked changes */
	cesk_diff_buffer_t *btmp = cesk_diff_buffer_new(0);
	uint32_t addr1 = cesk_frame_store_new_object(frame1, rtab, inst1, classpath, btmp, NULL);  
	assert(CESK_STORE_ADDR_NULL != addr1);
	assert(0 == cesk_frame_register_load(frame1, 3, addr1, NULL, NULL));

	uint32_t addr2 = cesk_frame_store_new_object(frame2, rtab, inst2, classpath, btmp, NULL);  
	assert(CESK_STORE_ADDR_NULL != addr2);
	assert(0 == cesk_frame_register_load(frame2, 3, addr2, NULL, NULL));
	/* make a record that can set reg #3 to {addd1, addr2} */
	cesk_set_t* tmpset = cesk_set_empty_set();
	assert(NULL != tmpset);
	cesk_set_push(tmpset, addr1);
	cesk_set_push(tmpset, addr2);
	cesk_diff_buffer_append(btmp, CESK_DIFF_REG, 3, tmpset);
	cesk_set_free(tmpset);

	/* then we shoud make the frame3 looks like the result of merge of frame1 and frame2 */
	cesk_diff_t* tmp_diff = cesk_diff_from_buffer(btmp);
	cesk_diff_buffer_free(btmp);
	assert(0 == cesk_frame_apply_diff(frame3, tmp_diff, rtab));
	cesk_diff_free(tmp_diff);

	/* now track changes in frame1 and frame2 */
	/* allocate a new object in frame1 */
	uint32_t addr3 = cesk_frame_store_new_object(frame1, rtab, inst3, classpath, db1, ib1);
	/* allocate another object in frame2 */
	uint32_t addr4 = cesk_frame_store_new_object(frame2, rtab, inst4, classpath, db2, ib2);


	assert(CESK_STORE_ADDR_NULL != addr4);
	assert(CESK_STORE_ADDR_NULL != addr3);
	assert(addr4 != addr1);

	assert(0 == cesk_frame_register_load(frame1, 5, addr3, db1, ib1));
	assert(0 == cesk_frame_register_load(frame2, 5, addr4, db2, ib2));

	/* set a field of addr3*/
	assert(0 == cesk_frame_register_load(frame1, 4, CESK_STORE_ADDR_POS, db1, ib1));
	assert(0 == cesk_frame_store_put_field(frame1, addr3, 4, superclass, field, db1, ib1));

	/* set a field of addr4*/
	assert(0 == cesk_frame_register_load(frame2, 4, CESK_STORE_ADDR_NEG, db2, ib2));
	assert(0 == cesk_frame_store_put_field(frame2 , addr4, 4, superclass, field, db2, ib2));

	/* build diff */
	cesk_diff_t* D1 = cesk_diff_from_buffer(db1);
	cesk_diff_t* D2 = cesk_diff_from_buffer(db2);
	cesk_diff_t* I1 = cesk_diff_from_buffer(ib1);
	cesk_diff_t* I2 = cesk_diff_from_buffer(ib2);
	
	cesk_diff_buffer_free(db1);
	cesk_diff_buffer_free(db2);
	cesk_diff_buffer_free(ib1);
	cesk_diff_buffer_free(ib2);

	assert(NULL != D1 && NULL != D2 && NULL != I1 && NULL != I2);

	/* let's try factorize */
	cesk_diff_t* diffs[] = {D1, D2};
	const cesk_frame_t* frames[] = {frame1, frame2};
	cesk_diff_t* D = cesk_diff_factorize(2, diffs, frames);
	assert(NULL != D);

	cesk_diff_free(D1);
	cesk_diff_free(D2);
	cesk_diff_free(I1);
	cesk_diff_free(I2);

	/* apply this to frame3 */
	assert(0 == cesk_frame_apply_diff(frame3, D, rtab));

	cesk_diff_free(D);

	/* okay, let's verify the result of apply */
	assert(2 == cesk_set_size(frame3->regs[3]));
	assert(cesk_set_contain(frame3->regs[3], addr1));
	assert(cesk_set_contain(frame3->regs[3], addr2));
	assert(2 == cesk_set_size(frame3->regs[5]));
	assert(cesk_set_contain(frame3->regs[5], addr3));
	assert(cesk_set_contain(frame3->regs[5], addr4));
	assert(2 == cesk_set_size(frame3->regs[4]));
	assert(cesk_set_contain(frame3->regs[4], CESK_STORE_ADDR_POS));
	assert(cesk_set_contain(frame3->regs[4], CESK_STORE_ADDR_NEG));
	uint32_t ubuf[3];
	int k;
	assert((k = cesk_frame_store_peek_field(frame3, addr3, superclass, field, ubuf, 3 )) >= 0);
	assert(1 == k);
	assert((k = cesk_frame_store_peek_field(frame3, addr4, superclass, field, ubuf, 3)) >= 0 );
	assert(1 == k);

	cesk_frame_free(frame1);
	cesk_frame_free(frame2);
	cesk_frame_free(frame3);
	cesk_reloc_table_free(rtab);
	cesk_alloctab_free(atab);
	adam_finalize();
	return 0;
}
