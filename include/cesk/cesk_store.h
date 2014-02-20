#ifndef __CESK_STORE_H__
#define __CESK_STORE_H__

#include <constants.h>

#include <cesk/cesk_value.h>
#include <dalvik/dalvik_instruction.h>

#define CESK_STORE_ADDR_NULL 0xfffffffful

/* This file contains declarations for Frame Store
 * The Frame Store unlike the nomally store, the 
 * store is owned by a signle frame, and have virtual address
 * Based on Frame Store, we can implement Copy-On-Write and
 * Gabage Collection easily.
 */
typedef struct {
    uint32_t        refcnt:31;        /* this refcnt is the counter inside this frame */
    uint8_t         reuse:1;          /* if this address is reused, because same insturction should allocate same address */
    uint32_t        idx;              /* instruction index created this object */
	uint32_t		parent;			  /* the parent address of this slot */
	const char*		field;		      /* filed name of the member */
    cesk_value_t*   value;
} cesk_store_slot_t;
typedef struct {
    uint32_t       refcnt;     /* for Copy-on-Write */
    uint32_t       num_ent;    /* number of entities */
    cesk_store_slot_t  slots[0];
} cesk_store_block_t;
#define CESK_STORE_BLOCK_NSLOTS ((CESK_STORE_BLOCK_SIZE - sizeof(cesk_store_block_t))/sizeof(cesk_store_slot_t))
typedef struct {
    uint32_t            nblocks;    /* number of blocks */
    uint32_t            num_ent;    /* number of entities */
    hashval_t           hashcode;   /* hashcode of content of this store */
    cesk_store_block_t* blocks[0];  /* block array */
} cesk_store_t;

/* make an empty store */
cesk_store_t* cesk_store_empty_store();
/* make a copy of a store */
cesk_store_t* cesk_store_fork(cesk_store_t* store);


/* get a read/write pointer */
cesk_value_t* cesk_store_get_rw(cesk_store_t* store, uint32_t addr);
/* get a read-only pointer */
const cesk_value_t* cesk_store_get_ro(cesk_store_t* store, uint32_t addr);
/* check if the value is reused by multiple object */
int cesk_store_is_reuse(cesk_store_t* store, uint32_t addr);
/* set the reuse flag */
void cesk_store_set_reuse(cesk_store_t* store, uint32_t addr);


/* allocate a fresh address for a new value, CESK_STORE_ADDR_NULL indicates an error 
 * If the store is resized, the address might be change,
 * So the parameter store might change after allocate function is called
 * 
 * Since the allocation function always returns the same addression for the same instruction
 * so we should know which instruction required the store to allocate a fresh address
 * If the address is for a member of an object. The address is based on the parent address and field name
 * If it's not a object member, parent = CESK_STORE_ADDR_NULL and field = NULL
 */
uint32_t cesk_store_allocate(cesk_store_t** p_store, const dalvik_instruction_t* inst, uint32_t parent, const char* field);

/* attach a value to an address, >0 means success, <0 error. If the value is NULL, means
 * dettach the address
 */
int cesk_store_attach(cesk_store_t* store, uint32_t addr,cesk_value_t* value);

/* release an attached address */
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

int cesk_store_equal(cesk_store_t* fisrt, cesk_store_t* second);
#endif
