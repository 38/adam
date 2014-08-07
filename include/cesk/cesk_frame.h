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
#ifndef __CESK_FRAME_H__
#define __CESK_FRAME_H__
typedef struct _cesk_frame_t cesk_frame_t;
#include <const_assertion.h>

#include <cesk/cesk_set.h>
#include <cesk/cesk_reloc.h>
#include <dalvik/dalvik_instruction.h>
#include <cesk/cesk_diff.h>
#include <cesk/cesk_static.h>

/* For all register and static field, we use a uint32_t interger to address them
 * The register is a 'register reference number'
 * The register reference number is actually another address space which is independent
 * to the store address space which contains object address, relocated address and constant
 * address.
 *
 * The layout of register referencing addresss space:
 *
 *  -------------------------------------------------------------------------------
 *  |R|E| General Register Reference |      unused     |   Static Field Refernce  |
 *  -------------------------------------------------------------------------------
 *  0 1 2                          0x10000           0x80000000                  0xffffffff
 *
 *  The lower address space is mapped to the register array `regs'
 *  The higher address space is mapped to the static field table `statics', which uses this reference number to locate the field
 */
/** 
 * @brief The register reference number for the result register
 **/
#define CESK_FRAME_RESULT_REG 0
/** 
 * @brief The register reference number for exception register
 **/
#define CESK_FRAME_EXCEPTION_REG 1
/** 
 * @brief convert a general register index to register reference address
 *  @param id general register index
 *  @return the result register reference
 */
#define CESK_FRAME_GENERAL_REG(id) (id + 2)

/* Helper Macros for the register reference which pointed to a static field */
/** 
 * @brief check if the register reference is an relocated address 
 **/
#define CESK_FRAME_REG_IS_STATIC(id) ((id)&CESK_FRAME_REG_STATIC_PREFIX)
/** 
 * @brief return the index of the static field 
 **/
#define CESK_FRAME_REG_STATIC_IDX(id) ((uint32_t)((id)&~CESK_FRAME_REG_STATIC_PREFIX))

/** 
 * @brief A Stack Frame of ADAM
 **/
struct _cesk_frame_t{
	uint32_t       size;     /*!<the number of registers in this frame */
	cesk_store_t*  store;    /*!<the store for this frame */ 
	cesk_static_table_t* statics; /*!< the static fields */ 
	cesk_set_t*    regs[0];  /*!<array of all registers, include result and exception */
	cesk_set_t*    reg_result; /*!<result register*/
	cesk_set_t*    reg_exception;  /*!<exception register*/
	cesk_set_t*    general_regs[0]; /*!<begin address of gneral registers */
};
CONST_ASSERTION_FOLLOWS(cesk_frame_t, regs, reg_result);
CONST_ASSERTION_FOLLOWS(cesk_frame_t, reg_result, reg_exception);
CONST_ASSERTION_FOLLOWS(cesk_frame_t, reg_exception, general_regs);
CONST_ASSERTION_SIZE(cesk_frame_t, regs, 0);
CONST_ASSERTION_SIZE(cesk_frame_t, general_regs, 0);

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
/**
 * @brief set the allocation table for this store
 * @param frame
 * @param alloctab
 * @return < 0 indicates an error
 **/
static inline int cesk_frame_set_alloctab(cesk_frame_t* frame, cesk_alloctab_t* alloctab)
{
	return cesk_store_set_alloc_table(frame->store, alloctab);
}
/**
 * @brief apply a diff to this frame
 * @todo implmentation
 * @param frame the target frame
 * @param diff  the diff to apply
 * @param reloctab the relocation table
 * @param invbuf the inverse diff buffer, NULL means do not record this
 * @param fwdbuf the forward diff buffer, NULL means do not record this
 * @return the number of changes has applied,  <0 indicates error
 **/
int cesk_frame_apply_diff(
	cesk_frame_t* frame, 
	const cesk_diff_t* diff, 
	const cesk_reloc_table_t* reloctab, 
	cesk_diff_buffer_t* fwdbuf,
	cesk_diff_buffer_t* invbuf);
/**
 * @brief load the value in the source register to destination static field
 * @param dst_reg the register reference address for the static field(which sould be a valid static field reference)
 * @param src_reg the source register reference
 * @param diff_buf the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return < 0 indicates errors 
 **/
int cesk_frame_static_load_from_register(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_reg,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);


/**
 * @brief load the value in the source static field to the destination register
 * @param dst_reg the register reference to destination register
 * @param src_reg the register reference to the source static field
 * @param diff_buf the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return result of the operation, < 0 indicates errors
 **/
int cesk_frame_register_load_from_static(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_reg,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);

/**
 * @brief move the content in source register to the destination register
 * @param frame the target frame
 * @param dst_reg destination register reference
 * @param src_reg source register reference
 * @param diff_buf the the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return < 0 indicates errors
 **/
int cesk_frame_register_move(
		cesk_frame_t* frame, 
		uint32_t dst_reg, 
		uint32_t src_reg, 
		cesk_diff_buffer_t* diff_buf, 
		cesk_diff_buffer_t* inv_buf);
/**
 * @brief clear the register, make the value set of the register is empty 
 * @param frame the target frame
 * @param dst_reg destination register reference
 * @param diff_buf the diff buffer
 * @param inv_buf  the inverse diff buffer
 * @return < 0 indicates errors 
 **/
int cesk_frame_register_clear(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);
/**
 * @brief load a constant to the register
 * @param frame the target frame
 * @param dst_reg destination register reference
 * @param src_addr source instant number address
 * @param diff_buf the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return < 0 indicates errors
 **/
int cesk_frame_register_load(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_addr,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);
/**
 * @brief push a value to the register
 * @param frame the target frame
 * @param dst_reg destination register reference
 * @param src_addr the source constant address
 * @param incref if this param is 1 then this function will change the refcount
 * @param diff_buf the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return < 0 indicates errors
 **/
int cesk_frame_register_push(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_addr,
		uint32_t incref,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);

/** 
 * @brief append a value to store
 * @param frame the target frame
 * @param dst_reg destination register reference
 * @param src_addr source store address
 * @param diff_buf
 * @param inv_buf
 * @return < 0 indicates error
 **/
int cesk_frame_register_append_from_store(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_addr,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);

/**
 * @brief load a object field from object bearing register to destiniation register
 * @param frame the target frame
 * @param dst_reg destination register reference
 * @param src_reg source boject bearing register reference
 * @param clspath the class path
 * @param fldname the field name
 * @param diff_buf the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return < 0 indicates error
 **/
int cesk_frame_register_load_from_object(
		cesk_frame_t* frame,
		uint32_t dst_reg,
		uint32_t src_reg,
		const char* clspath,
		const char* fldname,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);

/**
 * @brief allocate a new object in the frame store
 * @param frame the targbet frame
 * @param reloctab the relocation table
 * @param inst current instruction
 * @param alloc_param the allocation parameter (except the instruction index)
 * @param clspath the class path for the newly created object
 * @param bci_init_param the addtion initialaization parameter
 * @param diff_buf the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return the address of newly created object , CESK_STORE_ADDR_NULL indicates an error
 **/
uint32_t cesk_frame_store_new_object(
		cesk_frame_t* frame,
		cesk_reloc_table_t* reloctab,
		const dalvik_instruction_t* inst,
		const cesk_alloc_param_t* alloc_param,
		const char* clspath,
		const void* bci_init_param,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);
/**
 * @brief put the content of a register to the a field in store
 * @note because we allocate all fields in the new_object function, so if we 
 *       can see uninitialized value here, this must be an error
 * @param frame the target frame
 * @param dst_addr the destination object address
 * @param src_reg  the source register reference
 * @param clspath the class path
 * @param fldname the field name
 * @param keep_old_value wether or not we need to keep the old value during the assignemnt.
 *        This is because if the analyzer just know a set of address that is possiblely the
 *        destination, in this situation, override the old value will cause lost of possible value,
 *        so we keep the old value of every involed address
 * @param diff_buf the diff buffer
 * @param inv_buf the inverse diff buffer
 * @return the result of the operation < 0 indicates an error
 **/
int cesk_frame_store_put_field(
		cesk_frame_t* frame,
		uint32_t dst_addr,
		uint32_t src_reg,
		const char* clspath,
		const char* fldname,
		int keep_old_value,
		cesk_diff_buffer_t* diff_buf,
		cesk_diff_buffer_t* inv_buf);

/** 
 *  @brief return all possible values of a register in an array (for debugging)
 *  @param frame the frame object
 *  @param regid register reference
 *  @param buf   output buffer
 *  @param size	 buffer size
 *  @return the number of values returned, <0 when error 
 */
int cesk_frame_register_peek(const cesk_frame_t* frame, uint32_t regid, uint32_t* buf, size_t size);
/** 
 *  @brief get all possible value of a class member in store
 *  @param frame frame object
 *  @param addr store address
 *  @param classpath class path of the object
 *  @param fieldname name of the field
 *  @param buf output buffer
 *  @param size buffer size
 *  @return the number of value in result, < 0 error
 */
int cesk_frame_store_peek_field(const cesk_frame_t* frame, 
		uint32_t addr, 
		const char* classpath, const char* fieldname, 
		uint32_t* buf, size_t size);
/**
 * @brief make a new frame that is used for function invocation
 * @param frame current stack frame
 * @param nregs the number of registers the caller function requires
 * @param nargs the number of arguments this function rquires
 * @param args the argument array
 * @note the frame dose borrow reference from the args, that means, the if a set is put in array args 
 *       it should increase the refcount(which means we should fork a new reference to put in array)
 *       the reference is passed from the args array to the frame, that means, after this function return,
 *       the caller does not need to free the args array
 * @return the newly created frame
 */
cesk_frame_t* cesk_frame_make_invoke(const cesk_frame_t* frame, uint32_t nregs, uint32_t nargs, cesk_set_t** args);

/**
 * @brief print human readable frame content of the frame
 * @param frame the frame
 * @param buf the string buffer
 * @param sz the buffer size
 * @return a string describes this frame
 **/
const char* cesk_frame_to_string(const cesk_frame_t* frame, char* buf, size_t sz); 
/**
 * @brief print human readable context in the log file
 * @param frame 
 * @return nothing
 **/
void cesk_frame_print_debug(const cesk_frame_t* frame);
#endif /* __CESK_FRAME_H__ */
