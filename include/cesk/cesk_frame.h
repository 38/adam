#ifndef __CESK_FRAME_H__
#define __CESK_FRAME_H__
#include <cesk/cesk_set.h>

#define CESK_FRAME_RESULT_REG 0
#define CESK_FRAME_EXCEPTION_REG 1
#define CESK_FRAME_GENERAL_REG(id) (id + 2)

typedef struct {
    uint32_t       size;     /* the number of registers in this frame */
    cesk_store_t*  store;    /* the store for this frame */ 
	cesk_set_t*    regs[0];  /* array of all registers, include result and exception */
	cesk_set_t*    reg_result;
	cesk_set_t*    reg_exception; 
    cesk_set_t*    general_regs[0];
} cesk_frame_t;

/* duplicate the frame */
cesk_frame_t* cesk_frame_fork(cesk_frame_t* frame);

/* merge two frame, dest <- dest + sour */
//int cesk_frame_merge(cesk_frame_t* dest, const cessk_frame_t* sour);

/* create an empty stack which is empty */
cesk_frame_t* cesk_frame_new(uint16_t size);

/* despose a cesk frame */
void cesk_frame_free(cesk_frame_t* frame);

/* compare if two stack frame are equal */
int cesk_frame_equal(cesk_frame_t* first, cesk_frame_t* second);

/* garbage collect on a frame */
int cesk_frame_gc(cesk_frame_t* frame);

/* the hash fucntion of this frame */
hashval_t cesk_frame_hashcode(cesk_frame_t* frame);

/* operation on frames */
int cesk_frame_register_move(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dst_reg, uint32_t src_reg);

int cesk_frame_register_load(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dst_reg, uint32_t addr); 

int cesk_frame_register_clear(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t reg);

int cesk_frame_store_object_get(cesk_frame_t* frame, dalvik_instruction_t* inst , uint32_t dst_reg, uint32_t src_addr, const char* classpath, const char* field);

int cesk_frame_store_object_put(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dst_addr, const char* classpath, const char* field, uint32_t src_reg);

int cesk_frame_store_array_get(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dst_addr, uint32_t index, uint32_t src_reg);
int cesk_frame_store_array_put(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t index, uint32_t dst_reg, uint32_t src_reg);

/* allocate a 'fresh' address in this frame, and create a new object. The value is the address of the object */
uint32_t cesk_frame_store_new_object(cesk_frame_t* frame, const dalvik_instruction_t* inst, const char* classpath);

/* TODO: allocate a fresh address for an array object */
uint32_t cesk_frame_store_new_array(cesk_frame_t* frame, const dalvik_instruction_t* inst);

/* push a new value to this register (keep the old value) */
int cesk_frame_register_push(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t reg, uint32_t addr);

/* load a value from the store to register */
int cesk_frame_register_load_from_store(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dest, uint32_t src_addr);

/* append a value from the store to register */
int cesk_frame_register_append_from_store(cesk_frame_t* frame, dalvik_instruction_t* inst, uint32_t dest, uint32_t src_addr);
#endif /* __CESK_FRAME_H__ */
