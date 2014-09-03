#ifndef __CESK_STORE_H__
#define __CESK_STORE_H__
/**
 * @file cesk_store.h
 * @brief the virtual store of CESK Virtual Machine
 *
 * @details
 * This file contains declarations for Frame Store
 * The Frame Store unlike the nomally store, the 
 * store is owned by a signle frame, and have virtual address
 * Based on Frame Store, we can implement Copy-On-Write and
 * Gabage Collection easily.
 *
 * The store is a two level structure, the first level:
 *
 * 1. store object constains a block table
 *
 * 2. store block constains a gourp of slots which contains value 
 *
 * both store block and value are a copy-on-write objects.
 * So we can just copy the block table which is much smaller than the 
 * actual store.
 *
 * The store also have reference counters for each slot, in this way
 * we can perform gabage collection in the store.
 *
 * Readonly pointer & Writable pointer: user can acquire two kind of 
 * value pointer from the store, readonly one and writeable one. 
 * 
 * The readonly one should not be used when user want to write to the 
 * address. The readonly ponter is decorated with const keyword, so
 * do not convert it into a non-const pointer
 *
 * The writable pointer can write the value safely. But after finishing
 * writing, user must call cesk_store_release_rw function to release the
 * pointer
 */
#include <const_assertion.h>
#include <constants.h>

#include <cesk/cesk_alloc_param.h>

/* previous defs */
typedef struct _cesk_store_t cesk_store_t;


/* memory layout of a frame store : 
 *                                             OBJ_LIMIT
 * ------------------------------------------------------------------
 * |                     Object                  |  Reloc   | Const |
 * ------------------------------------------------------------------
 * 00000000                                      ff000000  ffffff00 ffffffff
 *                                                 RELOC    CONST     NULL
 *                                                 PREFIX   PREFIX
 */

/* special address that used for constants */
/* this special address is stands for some constants value,
 * for example negative, zero, positive. true/false. and a string
 * This address does not actually exists, the address it self 
 * constains information about the value 
 */

#define CESK_STORE_ADDR_OBJ_LIMIT CESK_STORE_ADDR_RELOC_PREFIX

/* CESK_STORE_ADDR_NULL must be 0xffffffff */
CONST_ASSERTION_EQ(CESK_STORE_ADDR_NULL, 0xffffffff);

/** @brief negative numeric value */
#define CESK_STORE_ADDR_NEG   (CESK_STORE_ADDR_CONST_PREFIX | 0x01ul)
/** @brief zero numeric value */
#define CESK_STORE_ADDR_ZERO  (CESK_STORE_ADDR_CONST_PREFIX | 0x02ul)
/** @brief positive numeric value */
#define CESK_STORE_ADDR_POS   (CESK_STORE_ADDR_CONST_PREFIX | 0x04ul)
/** @brief true boolean value */
#define CESK_STORE_ADDR_TRUE  (CESK_STORE_ADDR_CONST_PREFIX | 0x04ul)
/** @brief false boolean value */
#define CESK_STORE_ADDR_FALSE (CESK_STORE_ADDR_CONST_PREFIX | 0x02ul)
/** @brief an empty value, which is acutall null but using a different address, used for null object reference */
#define CESK_STORE_ADDR_EMPTY CESK_STORE_ADDR_CONST_PREFIX

/** @brief check the address is a const address */
#define CESK_STORE_ADDR_IS_CONST(addr) (((addr)&CESK_STORE_ADDR_CONST_PREFIX) == CESK_STORE_ADDR_CONST_PREFIX)
/** @brief check the address, if it's true, compute the expression, otherwise return false */
#define CESK_STORE_ADDR_CHECK_COMP(addr, exp) (CESK_STORE_ADDR_IS_CONST(addr) && (exp))
/** @brief the suffix of a constant address */
#define CESK_STORE_ADDR_CONST_SUFFIX(addr) ((addr)&(~CESK_STORE_ADDR_CONST_PREFIX))
/** @brief check if the address contain the value */
#define CESK_STORE_ADDR_CONST_CONTAIN(addr, elem) CESK_STORE_ADDR_CHECK_COMP(addr, ((addr) & CESK_STORE_ADDR_CONST_SUFFIX(CESK_STORE_ADDR_##elem)))
/** @brief return a new address that contains a value */
#define CESK_STORE_ADDR_CONST_SET(addr, elem) ((addr) | CESK_STORE_ADDR_CONST_SUFFIX(CESK_STORE_ADDR_##elem))

/* special addresses used for relocation */
/** @brief check if or not an address is a relocated address */
#define CESK_STORE_ADDR_IS_RELOC(addr) ((((addr)&CESK_STORE_ADDR_RELOC_PREFIX) == CESK_STORE_ADDR_RELOC_PREFIX) && !CESK_STORE_ADDR_IS_CONST(addr))
/** @brief get the index of global relocation object table from the address */
#define CESK_STORE_ADDR_RELOC_IDX(addr) ((addr)&~CESK_STORE_ADDR_RELOC_PREFIX)
/** @brief the size of relocated address space */
#define CESK_STORE_ADDR_RELOC_SIZE ((1 + ~CESK_STORE_ADDR_RELOC_PREFIX) -  (1 + ~CESK_STORE_ADDR_CONST_PREFIX))

/** @brief check if or not an address is an object address */
#define CESK_STORE_ADDR_IS_OBJ(addr) ((addr) < CESK_STORE_ADDR_OBJ_LIMIT)

#include <cesk/cesk_alloctab.h>

int cesk_store_set_alloc_table(cesk_store_t* store, cesk_alloctab_t* table);

#include <cesk/cesk_value.h>
#include <dalvik/dalvik_instruction.h>


#include <cesk/cesk_static.h>

/** @brief slot in virtual store */
typedef struct {
	uint32_t        refcnt:31;        /*!<this refcnt is the counter inside this frame */
	uint8_t         reuse:1;          /*!<if this address is reused, because same insturction should allocate same address */
	uint8_t         inuse:1;          /*!<if the address is in use */
	cesk_alloc_param_t param;         /*!<the allocation parameter for this address */
	cesk_value_t*   value;			  /*!<the data payload */
} cesk_store_slot_t;
/** @brief the store block of virtual store */
typedef struct {
	uint32_t       refcnt;     /*!<for Copy-on-Write */
	uint32_t       num_ent:31;    /*!<number of entities */
	uint8_t		   reloc:1;    /*!<wether or not this block contains a reference to relocated address*/
	cesk_store_slot_t  slots[0];
} cesk_store_block_t;
CONST_ASSERTION_LAST(cesk_store_block_t, slots);
CONST_ASSERTION_SIZE(cesk_store_block_t, slots, 0);
/** @brief the number of slots in one block */
#define CESK_STORE_BLOCK_NSLOTS ((CESK_STORE_BLOCK_SIZE - sizeof(cesk_store_block_t))/sizeof(cesk_store_slot_t))
/** @brief the virtual store object */
struct _cesk_store_t {
	uint32_t            nblocks:31; /*!<number of blocks */
	uint32_t            num_ent;    /*!<number of entities */
	hashval_t           hashcode;   /*!<hashcode of content of this store */
	cesk_alloctab_t*   alloc_tab;   /*!<the allocation table */
	uint32_t            alloc_token;/*!<the token for allocation table use */
	cesk_store_block_t**  blocks;   /*!<block array */
};

/** 
 * @brief make an empty store 
 * @return nothing
 **/
cesk_store_t* cesk_store_empty_store();
/**
 * @brief set the allocation table associated to this store
 * @param store 
 * @param table the allocation table
 * @return < 0 if an error occurred 
 **/
int cesk_store_set_alloc_table(cesk_store_t* store, cesk_alloctab_t* table);
/** 
 * @brief make a copy of a store
 * @param store the original store
 * @return the copy of the store
 */
cesk_store_t* cesk_store_fork(const cesk_store_t* store);
/**
 * @brief apply the allocation table attach to the store, that means substitude all
 *        relocated address using object address
 * @todo   a function that can apply the relocation table to a signle object
 * @param  store the store
 * @return the result of the operation <0 indicate error
 **/
int cesk_store_apply_alloctab(cesk_store_t* store);
/**
 * @brief get a writable pointer, you must release the adress when you are done
 * @param store the virtual store
 * @param addr address
 * @param noval if noval == 1, the function do no copy the value 
 * @return the writable value pointer
 **/
cesk_value_t* cesk_store_get_rw(cesk_store_t* store, uint32_t addr, int noval);
/** @brief get a read-only value pointer, do not try to write 
 *  @param store the virtual store
 *  @param addr address
 *  @return the read-only value pointer
 */
cesk_value_const_t* cesk_store_get_ro(const cesk_store_t* store, uint32_t addr);

/** @brief check if the value is reused by multiple object 
 *  @param store the virtual store
 *  @param addr the virtual address
 *  @return result
 */
int cesk_store_get_reuse(const cesk_store_t* store, uint32_t addr);
/** @brief set the reuse flag 
 *  @param store virtual store
 *  @param addr vitual address
 *  @return >=0 for success operation
 */
int cesk_store_set_reuse(cesk_store_t* store, uint32_t addr);

/** @brief clear the reuse flag 
 *  @param store virtual store
 *  @param addr vitual address
 *  @return >=0 for success operation
 */
int cesk_store_clear_reuse(cesk_store_t* store, uint32_t addr);

/** @brief
 * allocate a fresh address for a new value, CESK_STORE_ADDR_NULL indicates an error 
 * If the store is resized, the address might be change,
 * So the parameter store might change after allocate function is called
 * 
 * Since the allocation function always returns the same addression for the same instruction
 * so we should know which instruction required the store to allocate a fresh address
 * If the address is for a member of an object. The address is based on the parent address and field name
 * If it's not a object member, parent = CESK_STORE_ADDR_NULL and field = NULL
 *
 * @param store the store
 * @param param the allocation parameter
 * @return the fresh address for this value
 */
uint32_t cesk_store_allocate(cesk_store_t* store, const cesk_alloc_param_t* param);

/* attach a value to an address, >0 means success, <0 error. If the value is NULL, means
 * dettach the address.
 * NOTICE: After the attach function, your parameter value becomes a 
 * writable pointer as acquired from cesk_store_get_rw.
 * So YOU MUST RELEASE THE ADDRESS AFTER YOU DONE
 */
/** @brief attach a value to an object address(OA), 
 * 		   this function do not affect the refcnt of the address.
 * 		   And after the function return, the pointer value becomes
 * 		   a writable pointer automaticly. So you should release the value
 * 		   using cesk_store_get_rw after you done
 *  @param store virtual store
 *  @param addr object address
 *  @param value value
 *  @return the result of the operation >=0 means success 
 */
int cesk_store_attach(cesk_store_t* store, uint32_t addr,cesk_value_t* value);

/** @brief release an attached address 
 *  @param store virtual store
 *  @param addr virtual address
 *  @return nothing
 */
void cesk_store_release_rw(cesk_store_t* store, uint32_t addr);

/** @brief deallocate the store, you should free memory manually 
 *  @param store virtual store
 *  @return nothing
 */
void cesk_store_free(cesk_store_t* store); 

/** @brief increase the reference counter, return the new reference counter, negative means failure 
 *  @param store virtual store
 *  @param addr virtual address
 *  @return the result of the opreation >=0 means success
 */
int cesk_store_incref(cesk_store_t* store, uint32_t addr);

/** @brief decrease the reference counter, return the new reference counter, negative means failure 
 *  @return retsult
 */
int cesk_store_decref(cesk_store_t* store, uint32_t addr);

/** @brief get the hash code of the store 
 *  @return hash code 
 */
static inline hashval_t cesk_store_hashcode(const cesk_store_t* store)
{
	return store->hashcode;
}

/** @brief compute the hashcode rather than based on the increamental method, for debug purpose 
 *  @return hash code
 */
hashval_t cesk_store_compute_hashcode(const cesk_store_t* store);
/** @brief return if two store are equal 
 *  @return compare result 
 */
int cesk_store_equal(const cesk_store_t* fisrt, const cesk_store_t* second);

/** @brief this function returns constants address from a constant operand 
 *  @param operand this operand must carry a constant value
 *  @return the constant address convert from the operand
 */
uint32_t cesk_store_const_addr_from_operand(const dalvik_operand_t* operand);

/** @brief get the refcount of the address, used for testing 
 *  @return the refcnt of the adress
 */
uint32_t cesk_store_get_refcnt(const cesk_store_t* store, uint32_t addr);

/** @brief set the refcnt @ addr to zero, use for garbage clean 
 * @return the result of operation
 */
int cesk_store_clear_refcnt(cesk_store_t* store, uint32_t addr);

/**
 * @brief store to string
 * @param store the input store
 * @param buf buffer
 * @param sz buffer size
 * @return the result string
 **/
const char* cesk_store_to_string(const cesk_store_t* store, char* buf, size_t sz);
/**
 * @brief print human readable context in the log file
 * @param store 
 * @return nothing
 **/
void cesk_store_print_debug(const cesk_store_t* store);
/**
 * @brief compact the store, strip all empty blocks in the end of the blocks list
 * @param store 
 * @return the result of operation, < 0 indicates errors
 **/
int cesk_store_compact_store(cesk_store_t* store);
#endif
