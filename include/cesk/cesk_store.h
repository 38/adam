#ifndef __CESK_STORE_H__
#define __CESK_STORE_H__

#include <constants.h>

#include <cesk/cesk_value.h>

#define CESK_STORE_ADDR_NULL 0xfffffffful

/* This file contains declarations for Frame Store
 * The Frame Store unlike the nomally store, the 
 * store is owned by a signle frame, and have virtual address
 * Based on Frame Store, we can implement Copy-On-Write and
 * Gabage Collection easily.
 */
typedef struct {
    uint32_t        refcnt;
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


/* allocate a fresh address for a new value, CESK_STORE_ADDR_NULL indicates an error 
 * If the store is resized, the address might be change,
 * So the parameter store might change after allocate function is called
 */
uint32_t cesk_store_allocate(cesk_store_t** p_store);
/* attach a value to an address, >0 means success, <0 error */
int cesk_store_attach(cesk_store_t* store, uint32_t addr,cesk_value_t* value);

/* deallocate the store, you should free memory manually */
void cesk_store_free(cesk_store_t* store);

/* increase the reference counter, return the new reference counter, negative means failure */
int cesk_store_incref(cesk_store_t* store, uint32_t addr);
/* decrease the reference counter, return the new reference counter, negative means failure */
int cesk_store_decref(cesk_store_t* store, uint32_t addr);
#endif
