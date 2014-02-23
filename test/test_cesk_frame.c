#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	/* load the program */
	dalvik_loader_from_directory("../testdata/AndroidAntlr");
	/* build a new frame with 32 registers */
	cesk_frame_t* frame = cesk_frame_new(32);
	/* a dumb instruction that used for testing */
	dalvik_instruction_t* inst = dalvik_instruction_get(123);
	/* the class path we want to set */
	const char* classpath = stringpool_query("antlr/ANTLRLexer");
	const char* superclass = stringpool_query("antlr/CharScanner");
	const char* field = stringpool_query("literals");
	/* check the frame */
	assert(NULL != frame);
	
	/* try to load an address to the register */
	assert(0 == cesk_frame_register_load(frame, inst, 0 , CESK_STORE_ADDR_NEG));
	/* verify the hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* check the result of load */
	assert(1 == cesk_set_size(frame->regs[0]));
	/* verify the hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* open an iterator for the register */
	cesk_set_iter_t iter;
	assert(NULL != cesk_set_iter(frame->regs[0], &iter));
	/* verify the hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* check the content of the register */
	assert(CESK_STORE_ADDR_NEG == cesk_set_iter_next(&iter));
	/* verify the hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* it's the last element, so the next address should be NULL*/
	assert(CESK_STORE_ADDR_NULL == cesk_set_iter_next(&iter));
	/* verify the hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));

	/* try to move a register to another */
	assert(0 == cesk_frame_register_move(frame, inst, 0, 1));
	/* verify the hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* verify the result */
	assert(1 == cesk_set_equal(frame->regs[0], frame->regs[1]));

	/* try to clear the register */
	assert(0 == cesk_frame_register_clear(frame, inst, 0));
	/* verify the hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* verify the result */
	assert(0 == cesk_set_size(frame->regs[0]));

	/* create a new object */
	uint32_t addr = cesk_frame_store_new_object(frame, inst, classpath);
	/* verify hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* check the result */
	assert(CESK_STORE_ADDR_NULL != addr);
	/* test object_put */
	cesk_frame_store_object_put(frame, inst, addr, superclass, field, 0);
	//TODO: verify the hash code
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));

#if 0
	uint32_t addr3 = cesk_frame_store_new_object(frame, dalvik_instruction_get(123), stringpool_query("antlr/ANTLRLexer"));

	assert(addr2 == addr3);
#endif

	cesk_frame_free(frame);

	adam_finalize();
	return 0;
}
