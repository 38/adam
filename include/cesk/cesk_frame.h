#ifndef __CESK_FRAME_H__
#define __CESK_FRAME_H__
#include <cesk/cesk_set.h>
#include <dalvik/dalvik_instruction.h>
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

/* functions that used for applying diffs */
/** @brief set the value of the register 
 *  @param frame the frame to operated
 *  @param set   the value set you want to set to the register
 *  @return -1 indicates an error */
int cesk_frame_register_put(cesk_frame_t* frame, uint32_t reg, cesk_set_t* set);

/** @brief put a value at a fresh address
 *  @param frame the frame to operated
 *  @param object the object 
 *  @param inst instruction
 *  @return the address of the object */
uint32_t cesk_frame_store_put(cesk_frame_t* frame, cesk_value_t* value, const dalvik_instruction_t* inst);

/** @brief set the reuse flag of a given address
 *  @param frame the frame to be oprated
 *  @param addr the address to operated
 *  @param value the reused value to be set
 *  @return -1 for error */
int cesk_frame_store_object_reuse(cesk_frame_t* frame, uint32_t addr, uint8_t value);

#endif /* __CESK_FRAME_H__ */
