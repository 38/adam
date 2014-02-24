#ifndef __CESK_STORE_H__
#define __CESK_STORE_H__
/**
 * @file cesk_store.h
 * @brief the virtual store of CESK Virtual Machine
 *
 * @detial
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
 * Readonly pointer & Writable pointer: user can aquire two kind of 
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
#include <constants.h>

#include <cesk/cesk_value.h>
#include <dalvik/dalvik_instruction.h>

/** @brief the invalid address in the virtual store */
#define CESK_STORE_ADDR_NULL 0xfffffffful

/* special address that used for constants */
/* this special address is stands for some constants value,
 * for example negative, zero, positive. true/false. and a string
 * This address does not actually exists, the address it self 
 * constains information about the value 
 */
/** @brief negative numeric value */
#define CESK_STORE_ADDR_NEG   (CESK_STORE_ADDR_CONST_PREFIX | 0x01ul)
/** @brief zero numeric value */
#define CESK_STORE_ADDR_ZERO  (CESK_STORE_ADDR_CONST_PREFIX | 0x02ul)
/** @brief positive numeric value */
#define CESK_STORE_ADDR_POS   (CESK_STORE_ADDR_CONST_PREFIX | 0x04ul)
/** @brief true boolean value */
#define CESK_STORE_ADDR_TRUE  (CESK_STORE_ADDR_CONST_PREFIX | 0x08ul)
/** @brief false boolean value */
#define CESK_STORE_ADDR_FALSE (CESK_STORE_ADDR_CONST_PREFIX | 0x10ul)

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

/** @brief slot in virtual store */
typedef struct {
    uint32_t        refcnt:31;        /*!<this refcnt is the counter inside this frame */
    uint8_t         reuse:1;          /*!<if this address is reused, because same insturction should allocate same address */
    uint32_t        idx;              /*!<instruction index created this object */
	uint32_t		parent;			  /*!<the parent address of this slot */
	const char*		field;		      /*!<filed name of the member */
    cesk_value_t*   value;			  /*!<the data payload */
} cesk_store_slot_t;
/** @brief the store block of virtual store */
typedef struct {
    uint32_t       refcnt;     /*!<for Copy-on-Write */
    uint32_t       num_ent;    /*!<number of entities */
    cesk_store_slot_t  slots[0];
} cesk_store_block_t;
/** @brief the number of slots in one block */
#define CESK_STORE_BLOCK_NSLOTS ((CESK_STORE_BLOCK_SIZE - sizeof(cesk_store_block_t))/sizeof(cesk_store_slot_t))
/** @brief the virtual store object */
typedef struct {
    uint32_t            nblocks;    /*!<number of blocks */
    uint32_t            num_ent;    /*!<number of entities */
    hashval_t           hashcode;   /*!<hashcode of content of this store */
    cesk_store_block_t* blocks[0];  /*!<block array */
} cesk_store_t;

/** @brief make an empty store 
 *  @return nothing
 */
cesk_store_t* cesk_store_empty_store();
/** @brief make a copy of a store 
 *  @param store the original store
 *  @return the copy of the store
 */
cesk_store_t* cesk_store_fork(cesk_store_t* store);

/** @brief get a writable pointer, you must release the adress when you are done
 *  @param store the virtual store
 *  @param addr address
 *  @return the writable value pointer
 */
cesk_value_t* cesk_store_get_rw(cesk_store_t* store, uint32_t addr);
/** @brief get a read-only value pointer, do not try to write 
 *  @param store the virtual store
 *  @param addr address
 *  @return the read-only value pointer
 */
const cesk_value_t* cesk_store_get_ro(cesk_store_t* store, uint32_t addr);

/** @brief check if the value is reused by multiple object 
 *  @param store the virtual store
 *  @param addr the virtual address
 *  @return result
 */
int cesk_store_is_reuse(cesk_store_t* store, uint32_t addr);
/** @brief set the reuse flag 
 *  @param store virtual store
 *  @param addr vitual address
 *  @return >=0 for success operation
 */
int cesk_store_set_reuse(cesk_store_t* store, uint32_t addr);


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
 * @param p_store the pointer to the pointer of store
 * @param inst current instruction
 * @param parent address of parent object
 * @param field the field name
 * @return the fresh address for this value
 */
uint32_t cesk_store_allocate(cesk_store_t** p_store, const dalvik_instruction_t* inst, uint32_t parent, const char* field);

/* attach a value to an address, >0 means success, <0 error. If the value is NULL, means
 * dettach the address.
 * NOTICE: After the attach function, your parameter value becomes a 
 * writable pointer as aquired from cesk_store_get_rw.
 * So YOU MUST RELEASE THE ADDRESS AFTER YOU DONE
 */
/** @brief attach a value to an address, 
 * 		   this function do not affect the refcnt of the address.
 * 		   And after the function return, the pointer value becomes
 * 		   a writable pointer automaticly. So you should release the value
 * 		   using cesk_store_get_rw after you done
 *  @param store virtual store
 *  @param addr virtual address
 *  @param value value
 *  @return the result of the operation >=0 means success 
 */
int cesk_store_attach(cesk_store_t* store, uint32_t addr,cesk_value_t* value);

/** @brief release an attached address 
 *  @param store virtual store
 *  @param addr virtual address
 */
void cesk_store_release_rw(cesk_store_t* store, uint32_t addr);

/* deallocate the store, you should free memory manually */
void cesk_store_free(cesk_store_t* store); 

/* increase the reference counter, return the new reference counter, negative means failure */
int cesk_store_incref(cesk_store_t* store, uint32_t addr);

/* decrease the reference counter, return the new reference counter, negative means failure */
int cesk_store_decref(cesk_store_t* store, uint32_t addr);

static inline hashval_t cesk_store_hashcode(cesk_store_t* store)
{
    return store->hashcode;
}

/* compute the hashcode rather than based on the increamental method, for debug purpose */
hashval_t cesk_store_compute_hashcode(cesk_store_t* store);

int cesk_store_equal(cesk_store_t* fisrt, cesk_store_t* second);

/* this function returns constants address from a constant operand */
uint32_t cesk_store_const_addr_from_operand(dalvik_operand_t* operand);

/* get the refcount of the address, used for testing */
uint32_t cesk_store_get_refcnt(cesk_store_t* store, uint32_t addr);

/* set the refcnt @ addr to zero, use for garbage clean */
int cesk_store_clear_refcnt(cesk_store_t* store, uint32_t addr);
#endif
