#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	/* load the program */
	dalvik_loader_from_directory("test/data/AndroidAntlr");
	/* build a new frame with 32 registers */
	cesk_frame_t* frame = cesk_frame_new(32);
	/* a stub instruction that used for testing */
	const dalvik_instruction_t* inst = dalvik_instruction_get(123);
	/* another stub instruction */
	const dalvik_instruction_t* inst2 = dalvik_instruction_get(124);
	
	const dalvik_instruction_t* inst3 = dalvik_instruction_get(125);
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
	uint32_t prev_hash = cesk_frame_hashcode(frame); 
	/* check the result of load */
	assert(1 == cesk_set_size(frame->regs[0]));
	/* open an iterator for the register */
	cesk_set_iter_t iter;
	assert(NULL != cesk_set_iter(frame->regs[0], &iter));
	/* check the content of the register */
	assert(CESK_STORE_ADDR_NEG == cesk_set_iter_next(&iter));
	/* it's the last element, so the next address should be NULL*/
	assert(CESK_STORE_ADDR_NULL == cesk_set_iter_next(&iter));

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
	assert(0 == cesk_frame_store_object_put(frame, inst, addr, superclass, field, 5));
	/* verify the hash code */
	assert(cesk_frame_compute_hashcode(frame) == cesk_frame_hashcode(frame));
	/* verify the refcount */
	assert(2 == cesk_store_get_refcnt(frame->store, addr2));
	
	/* test for reused object */
	uint32_t addr3 = cesk_frame_store_new_object(frame, inst, classpath);
	/* the address should equal to addr1 */
	assert(addr3 == addr);
	assert(addr3 != addr2);
	/* check hashcode again */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* check the reuse flag */
	assert(1 == cesk_store_is_reuse(frame->store, addr3));
	/* ok, let's test the logic of reuse */
	/* at the same time, we test the decref on old value when a new value are to be loaded to the register */
	/* move reg5, $true */
	assert(0 == cesk_frame_register_load(frame, inst, 5, CESK_STORE_ADDR_TRUE));
	/* check the hash code after load */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* now check the refcnt of addr2, should be 1, because register is not refer to the object any more */
	assert(1 == cesk_store_get_refcnt(frame->store, addr2));
	/* put the value to object 1 */
	/* ((antlr.CharScanner) obj1).literals = reg5 */
	assert(0 == cesk_frame_store_object_put(frame, inst, addr, superclass, field, 5)); 
	/* check the hashcode */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* then try to get the field */
	assert(0 == cesk_frame_store_object_get(frame, inst, 5, addr, superclass, field));
	/* the value should be a set {object2, true} */
	assert(2 == cesk_set_size(frame->regs[5]));
	assert(1 == cesk_set_contain(frame->regs[5], addr2));
	assert(1 == cesk_set_contain(frame->regs[5], CESK_STORE_ADDR_TRUE));
	/* check the refcnt */
	assert(2 == cesk_store_get_refcnt(frame->store, addr2));

	/* now check the refcnt of obj1 */
	assert(1 == cesk_store_get_refcnt(frame->store, addr));
	
	/* okay, test the garbage collector, refcount first */
	uint32_t addr4 = cesk_frame_store_new_object(frame, inst3, classpath);
	/* check the hash code*/
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* move reg21, addr4 */
	assert(0 == cesk_frame_register_load(frame, inst, 21, addr4));
	/* instance-put addr2, antlr.CharScanner.literals ,reg4 */
	assert(0 == cesk_frame_store_object_put(frame, inst, addr2, superclass, field, 21));
	/* check the hash code*/
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* override the register that contains address of object 2 */
	assert(0 == cesk_frame_register_load(frame, inst, 5, CESK_STORE_ADDR_TRUE));
	/* check the hash code*/
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* load a new value to the register, this cause dereference, this should also cause the death of object 2 */
	assert(0 == cesk_frame_register_load(frame, inst, 20, CESK_STORE_ADDR_ZERO));
	/* check the hash code*/
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* addr2 is empty now */
	assert(NULL == cesk_store_get_ro(frame->store, addr2));
	/* addr is empty now */
	assert(NULL == cesk_store_get_ro(frame->store, addr));
	/* because reg21 is still pointing to addr4, so addr4 should not be empty */
	assert(NULL != cesk_store_get_ro(frame->store, addr4));
	/* fianlly, clear reg21 cause death of addr4 */
	assert(0 == cesk_frame_register_clear(frame, inst, 21));
	/* check the hashcode again */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* now addr4 should be empty */
	assert(NULL == cesk_store_get_ro(frame->store, addr4));

	/* all object in the store is dead, restore the value of register, and check 
	 * if the hashcode is inital hash code */
	int i = 0;
	for(i = 0; i < frame->size; i ++)
		assert(0 == cesk_frame_register_clear(frame, inst, i));
	assert(0 == cesk_frame_register_load(frame, inst, 0, CESK_STORE_ADDR_NEG));
	/* check the hashcode again */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	assert(prev_hash == cesk_frame_hashcode(frame));

	/* now test the gc function, which dectect the matually refering garabage in memory */
	/* create 3 objects */
	uint32_t obj1 = cesk_frame_store_new_object(frame, inst, classpath);
	uint32_t obj2 = cesk_frame_store_new_object(frame, inst2, classpath);
	uint32_t obj3 = cesk_frame_store_new_object(frame, inst3, classpath);
	/* check 3 addresses are not same */
	assert(obj1 != obj2);
	assert(obj1 != obj3);
	assert(obj2 != obj3);
	/* check 3 address are valid */
	assert(obj1 != CESK_STORE_ADDR_NULL);
	assert(obj2 != CESK_STORE_ADDR_NULL);
	assert(obj3 != CESK_STORE_ADDR_NULL);
	/* verify the hashcode of the store */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* load 3 object to register */
	assert(0 == cesk_frame_register_load(frame, inst, 1, obj1));
	assert(0 == cesk_frame_register_load(frame, inst, 2, obj2));
	assert(0 == cesk_frame_register_load(frame, inst, 3, obj3));
	/* verify the hashcode of the store */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* build a circle */
	/* 1 --> 2 */
	assert(0 == cesk_frame_store_object_put(frame, inst, obj1, superclass, field, 2));
	/* 2 --> 3 */
	assert(0 == cesk_frame_store_object_put(frame, inst, obj2, superclass, field, 3));
	/* 3 --> 1 */
	assert(0 == cesk_frame_store_object_put(frame, inst, obj3, superclass, field, 1));
	/* verify the hashcode of the store */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* the refcnt for each object should be 2 */
	assert(2 == cesk_store_get_refcnt(frame->store, obj1));
	assert(2 == cesk_store_get_refcnt(frame->store, obj2));
	assert(2 == cesk_store_get_refcnt(frame->store, obj3));
	/* run gc now, not free any memory */
	assert(0 == cesk_frame_gc(frame));
	/* verify the hashcode of the store */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* now, decref from reg */
	assert(0 == cesk_frame_register_clear(frame, inst, 1));
	assert(0 == cesk_frame_register_clear(frame, inst, 2));
	/* verify the hashcode of the store */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* check the ref count */
	assert(1 == cesk_store_get_refcnt(frame->store, obj1));
	assert(1 == cesk_store_get_refcnt(frame->store, obj2));
	assert(2 == cesk_store_get_refcnt(frame->store, obj3));
	/* run gc now, not free any memory */
	assert(0 == cesk_frame_gc(frame));
	/* verify the hashcode of the store */
	assert(cesk_frame_hashcode(frame) == cesk_frame_compute_hashcode(frame));
	/* decref */
	assert(0 == cesk_frame_register_clear(frame, inst, 3));
	/* is the object still alive ? */
	assert(NULL != cesk_store_get_ro(frame->store, obj1));
	assert(NULL != cesk_store_get_ro(frame->store, obj2));
	assert(NULL != cesk_store_get_ro(frame->store, obj3));
	/* check the ref count */
	assert(1 == cesk_store_get_refcnt(frame->store, obj1));
	assert(1 == cesk_store_get_refcnt(frame->store, obj2));
	assert(1 == cesk_store_get_refcnt(frame->store, obj3));
	/* run gc */
	assert(0 == cesk_frame_gc(frame));
	/* now 3 object should be swipeed out */
	assert(NULL == cesk_store_get_ro(frame->store, obj1));
	assert(NULL == cesk_store_get_ro(frame->store, obj2));
	assert(NULL == cesk_store_get_ro(frame->store, obj3));
	/* check if the ref count maintained correctly */
	assert(0 == cesk_store_get_refcnt(frame->store, obj1));
	assert(0 == cesk_store_get_refcnt(frame->store, obj2));
	assert(0 == cesk_store_get_refcnt(frame->store, obj3));


	assert(cesk_frame_hashcode(frame) == prev_hash);
	
	cesk_frame_free(frame);
	adam_finalize();
	return 0;
}
