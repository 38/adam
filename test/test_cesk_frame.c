#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	/* load the program */
	dalvik_loader_from_directory("../testdata/AndroidAntlr");
	/* build a new frame with 32 registers */
	cesk_frame_t* frame = cesk_frame_new(32);
	/* a stub instruction that used for testing */
	dalvik_instruction_t* inst = dalvik_instruction_get(123);
	/* another stub instruction */
	dalvik_instruction_t* inst2 = dalvik_instruction_get(124);
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
	/* check the result */
	assert(CESK_STORE_ADDR_NULL != addr);
	/* verify hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* make the object a active object */
	assert(0 == cesk_frame_register_load(frame, inst, 20, addr));
	/* check the refcount */
	assert(1 == cesk_store_get_refcnt(frame->store, addr));
	/* test object get for new object */
	assert(0 == cesk_frame_store_object_get(frame, inst, 1, addr, superclass, field));
	/* the result should be empty set */
	assert(0 == cesk_set_size(frame->regs[0]));
	/* move reg0, $0 */
	assert(0 == cesk_frame_register_load(frame, inst, 0, CESK_STORE_ADDR_ZERO));
	/* verify the size of reg0 */
	assert(1 == cesk_set_size(frame->regs[0]));
	/* object-put addr, antlr.CharScanner.literals, reg0 */
	cesk_frame_store_object_put(frame, inst, addr, superclass, field, 0);
	/* verify the hash code */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* try to get the field */
	assert(0 == cesk_frame_store_object_get(frame, inst, 1, addr, superclass, field));
	/* verify the store */
	assert(cesk_set_equal(frame->regs[0], frame->regs[1]) == 1);
	
	/* the address is attached to signle object, put will override the old value */
	/* move reg2, $-1 */
	assert(0 == cesk_frame_register_load(frame, inst, 2, CESK_STORE_ADDR_NEG));
	/* verify the size of reg2 */
	assert(1 == cesk_set_size(frame->regs[2]));
	/* verify the hash code */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* object-put addr, antlr.CharScanner.literals, reg2 */
	assert(0 == cesk_frame_store_object_put(frame, inst, addr, superclass, field, 2));
	/* verify the hash code */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* the value should be overrided , verify it */
	assert(0 == cesk_frame_store_object_get(frame, inst, 3, addr, superclass, field));
	/* so the value should be same as reg2 */
	assert(cesk_set_equal(frame->regs[2], frame->regs[3]) == 1);

	/* test for cascade decref */
	uint32_t addr2 = cesk_frame_store_new_object(frame, inst2, classpath);
	/* check the validity of allocation */
	assert(addr != addr2);
	/* check the return value */
	assert(CESK_STORE_ADDR_NULL != addr);
	/* verify hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* load the add ress of address 2 to reg5. move reg5, addr2 */
	assert(0 == cesk_frame_register_load(frame, inst, 5, addr2));
	/* verify hashcode */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* object-put addr, antlr.CharScanner.literals, reg5 */
	cesk_frame_store_object_put(frame, inst, addr, superclass, field, 5);
	/* verify the hash code */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* verify the refcount */
	assert(2 == cesk_store_get_refcnt(frame->store, addr2));
	
	//TODO: test for reused object



#if 0
	uint32_t addr3 = cesk_frame_store_new_object(frame, dalvik_instruction_get(123), stringpool_query("antlr/ANTLRLexer"));

	assert(addr2 == addr3);
#endif

	cesk_frame_free(frame);

	adam_finalize();
	return 0;
}
