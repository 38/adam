/**
 * @file   cesk_valuetab.h
 * @brief  The defination of the global (intra-procedure) relocated value table
 * @detail The reason why we need relocation mecanism is discussed in the comment in the file 
 *         cesk_alloc_table.h 
 *         This file is a part of the relocation mechanism. This is the data structure used to
 *         hold the newly created value, and each value is assigned with a ID. And in each 
 *         store , the relocation address (CESK_STORE_ADDR_RELOC_PREFIX | X) is associated 
 *         with the value of which ID in the relocation table is X.
 *
 *         This file also handle how to install the relocated value in each store. 
 *
 * @note   This table also hold one refcount to make sure that all value in this table is valid
 **/
#ifndef __CESK_VALUETAB_H__
#define __CESK_VALUETAB_H__
#include <cesk/cesk_store.h>
#include <cesk/cesk_value.h>
#include <vector.h>
typedef vector_t cesk_valuetab_t;

/**
 * @brief create a newy value table
 * @return pointer to the newly created value table, NULL indicates error
 **/
cesk_valuetab_t *cesk_valuetab_new();

/**
 * @brief free the memory used by the value table
 * @param mem  the pointer to that value table
 * @return nothing
 **/
void cesk_valuetab_free(cesk_valuetab_t* mem);

/**
 * @brief append a new value to the value table
 * @param table the value table 
 * @param value  the value to append
 * @return the index of this value, CESK_STORE_ADDR_NULL indicates error 
 **/
uint32_t cesk_valuetab_append(cesk_valuetab_t* table, cesk_value_t* value);

/**
 * @breif install a relocated value to a store
 * @param table the realocated value table
 * @param store the store
 * @param addr  the relocated address
 * @return the object address for this relocated value, CESK_STORE_ADDR_NULL means error
 **/
uint32_t cesk_valuetab_install_reloc(cesk_valuetab_t* table, cesk_store_t* store, uint32_t addr);
#endif /*__CESK_VALUETAB_H__*/
