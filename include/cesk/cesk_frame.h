#ifndef __CESK_FRAME_H__
#define __CESK_FRAME_H__
#include <cesk/cesk_set.h>
/** @file cesk_frame.h
 *  @brief A stack frame of the virtual machine
 *
 *  @details see 
 *  <a herf=http://source.android.com/devices/tech/dalvik/dalvik-bytecode.html>
 *  http://source.android.com/devices/tech/dalvik/dalvik-bytecode.html</a> for
 *  the detail of dalvik virtual machine.
 *
 *  We do not need any function for destory an object, because we have garbage 
 *  collector in the frame
 */
/** @brief the actual register id of result register */
#define CESK_FRAME_RESULT_REG 0
/** @brief the actual register id of exception register */
#define CESK_FRAME_EXCEPTION_REG 1
/** @brief convert a general register to acual index, so that you can use frame->regs[id] to visit the register
 *  @param id general register index
 *  @return acutal index, 
 */
#define CESK_FRAME_GENERAL_REG(id) (id + 2)

/** @brief A Stack Frame of The Dalvik CESK Machine */
typedef struct {
    uint32_t       size;     /*!<the number of registers in this frame */
    cesk_store_t*  store;    /*!<the store for this frame */ 
	cesk_set_t*    regs[0];  /*!<array of all registers, include result and exception */
	cesk_set_t*    reg_result; /*!<result register*/
	cesk_set_t*    reg_exception;  /*!<exception register*/
    cesk_set_t*    general_regs[0]; /*!<begin address of gneral registers */
} cesk_frame_t;

/** @brief duplicate the frame 
 *  @param frame input frame
 *  @return a copy of this frame
 */
cesk_frame_t* cesk_frame_fork(const cesk_frame_t* frame);

/* merge two frame, dest <- dest + sour */
//int cesk_frame_merge(cesk_frame_t* dest, const cessk_frame_t* sour);

/** @brief create an empty stack which is empty
 *  @param size number of general registers 
 *  @return new frame
 */
cesk_frame_t* cesk_frame_new(uint16_t size);

/** @brief despose a cesk frame
 *  @param frame the frame to be freed
 *  @return nothing
 */
void cesk_frame_free(cesk_frame_t* frame);

/** @brief compare if two stack frame are equal 
 *  @param first the first frame 
 *  @param second the second frame
 *  @return 1 if first == second, 0 otherwise
 */
int cesk_frame_equal(const cesk_frame_t* first, const cesk_frame_t* second);

/** @brief run garbage collector on a frame 
 * @param frame
 * @return >=0 means success
 */
int cesk_frame_gc(cesk_frame_t* frame);

/** @brief the hash fucntion of this frame 
 *  @param frame
 *  @return the hash code of the frame
 */
hashval_t cesk_frame_hashcode(const cesk_frame_t* frame);

/** @brief the hash code compute without incremental style 
 *  @param frame
 *  @return the hash code of the frame */
hashval_t cesk_frame_compute_hashcode(const cesk_frame_t* frame);

/* operation on frames */
/** @brief copy the content of source register to destination regiseter
 * @param frame the frame we are operating
 * @param inst current instruction
 * @param dst_reg destination register
 * @param src_reg source register
 * @return the result of operation, >=0 means success
 */
int cesk_frame_register_move(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t dst_reg, uint32_t src_reg);

/** @brief load an address to destination regiseter
 * @param frame the frame we are operating
 * @param inst current instruction
 * @param dst_reg destination register
 * @param addr the address to load
 * @return the result of operation, >=0 means success
 */
int cesk_frame_register_load(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t dst_reg, uint32_t addr); 

/** @brief clear the value of the register
 * @param frame the frame we are operating 
 * @param inst current instruction
 * @param reg register id
 * @return the result of operation, >=0 means success
 */
int cesk_frame_register_clear(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t reg);

/** @brief load value of a field from source object to destination register
 *  @param frame the frame we are operating
 *  @param inst current instruction
 *  @param dst_reg destiantion register
 *  @param src_addr address of source object, must be an address that carries an object
 *  @param classpath the class path of the object
 *  @param field	the field name of the field
 *  @return the result of the opreation, >=0 means success
 */
int cesk_frame_store_object_get(cesk_frame_t* frame, 
								const dalvik_instruction_t* inst , 
								uint32_t dst_reg, uint32_t src_addr, const char* classpath, const char* field);

/** @brief save the value of source register to the field of destination object 
 *  @param frame the frame we are operating
 *  @param inst current instruction
 *  @param dst_addr destiantion address, must be an address carrying object
 *  @param classpath the class path of the object
 *  @param field	the field name of the field
 *  @param src_reg source register
 *  @return the result of the opreation, >=0 means success
 */
int cesk_frame_store_object_put(cesk_frame_t* frame, 
		                        const dalvik_instruction_t* inst, 
								uint32_t dst_addr, const char* classpath, const char* field, uint32_t src_reg);


/** @brief load value of a field from source array to destination register(to be implemented)
 *  @param frame the frame we are operating
 *  @param inst current instruction
 *  @param dst_addr destiantion register
 *  @param index index in the array
 *  @param src_reg address of source object
 *  @return the result of the opreation, >=0 means success
 */
int cesk_frame_store_array_get(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t dst_addr, uint32_t index, uint32_t src_reg);
/** @brief save the value of source register to the field of destination array(to be implemented) 
 *  @param frame the frame we are operating
 *  @param inst current instruction
 *  @param index index in the array
 *  @param dst_reg destiantion addrest
 *  @param src_reg source register
 *  @return the result of the opreation, >=0 means success
 */
int cesk_frame_store_array_put(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t index, uint32_t dst_reg, uint32_t src_reg);

/** @brief allocate a 'fresh' address in this frame, and create a new object.
 *
 * @details For the same allocation instruction, the virtual machine just return the same address, in this we
 *  we can get a finate store. 
 *
 *  The function do not incref the return address, so you should save the return value after the function return
 *  or it may lead some bug.
 *
 * @param frame the frame we are operating
 * @param inst current instruction
 * @param classpath the class path of the clas
 * @param return the address of the new object
 */
uint32_t cesk_frame_store_new_object(cesk_frame_t* frame, const dalvik_instruction_t* inst, const char* classpath);

/** @brief TODO: allocate a fresh address for an array object */
uint32_t cesk_frame_store_new_array(cesk_frame_t* frame, const dalvik_instruction_t* inst);

/** @brief push a new value to this register (keep the old value) 
 * 
 * @details the function like cesk_frame_register_load function, save an addr in a register.
 * But unlike cesk_frame_register_load, which clear the old value first, this function
 * keep the old value and append the new value.
 *
 * @param frame the frame we are oeprating 
 * @param inst current instruction
 * @param reg destination register
 * @param addr source address
 * @return the result of the opration, >=0 means success
 */
int cesk_frame_register_push(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t reg, uint32_t addr);

/** @brief load a value from the store to register 
 *
 * @details this function loads a set saved in store to a register.
 *
 * @param frame
 * @param inst current instruction
 * @param dest the destination register
 * @param src_addr the address of source seti
 * @return the result of the opration, >=0 means success
 */
int cesk_frame_register_load_from_store(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t dest, uint32_t src_addr);

/** @brief append a value from the store to register 
 *
 * @details like cesk_frame_register_load_from_store, but the function do not clear the destination register 
 *
 * @param frame
 * @param inst current instruction
 * @param dest the destination register
 * @param src_addr the address of source seti
 * @return the result of the opration, >=0 means success
 */
int cesk_frame_register_append_from_store(cesk_frame_t* frame, const dalvik_instruction_t* inst, uint32_t dest, uint32_t src_addr);
#endif /* __CESK_FRAME_H__ */
