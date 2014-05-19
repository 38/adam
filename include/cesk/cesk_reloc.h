/**
 * @file   cesk_reloc.h
 * @brief  The defination of the global (intra-procedure) relocated value table
 * @details The reason why we need relocation mecanism is discussed in the comment in the file 
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
/*
 	example for allocate an relocated object

	cesk_reloc_allocate();   allocate a new RA
	cesk_store_attach;       set the new value
	cesk_store_release_rw;   finalize writing
	
	now we should allocate all fields with an empty set

	cesk_reloc_set_init_value; so that we can pass the default value to others


	in the diff, if there's a allocation request, just do
	cesk_reloc_addr_init();
*/
#ifndef __CESK_RELOC_H__
#define __CESK_RELOC_H__
#include <cesk/cesk_store.h>
#include <cesk/cesk_value.h>
#include <dalvik/dalvik_instruction.h>
#include <vector.h>
typedef vector_t cesk_reloc_table_t;

/**
 * @brief the value list item 
 * @note  see the documentation of cesk_store_allocate for details
 **/
typedef struct {
	const dalvik_instruction_t* instruction;	/*!< the instruction that allocate this item */
	uint32_t      parent_addr;					/*!< the parent address */
	uint32_t      field_offset;					/*!< the field offset */ 
} cesk_reloc_item_t;

/**
 * @brief create a newy value table
 * @return pointer to the newly created value table, NULL indicates error
 **/
cesk_reloc_table_t *cesk_reloc_table_new();

/**
 * @brief free the memory used by the value table
 * @param mem  the pointer to that value table
 * @return nothing
 **/
void cesk_reloc_table_free(cesk_reloc_table_t* mem);

/**
 * @brief append a new value to the value table
 * @param table the value table 
 * @param inst  the instruction that allocates this object
 * @param parent_addr the address of parent object
 * @param field_offset the offset of this field
 * @return the index of this value, CESK_STORE_ADDR_NULL indicates error 
 **/
uint32_t cesk_reloc_table_append(
		cesk_reloc_table_t* table, 
		const dalvik_instruction_t* inst,
		uint32_t parent_addr,
		uint32_t field_offset);
/**
 * @brief allocate a `fresh' relocated address and attach it to the global relocate allocate table
 * 		  (but do not install the value in the store if it's new) useful when creating new object
 * @param value_tab the global relocated value table
 * @param store
 * @param inst the instruction that allocate the object
 * @param parent the address of parent object
 * @param field the field offset
 * @return the relocated address of the vlaue, CESK_STORE_ADDR_NULL means error
 * @note   DO NOT FORGET SET THE INITIAL VALUE TO THIS RELOCATED ADDRESS
 **/
uint32_t cesk_reloc_allocate(
		cesk_reloc_table_t* value_tab, 
		cesk_store_t* store, 
		const dalvik_instruction_t* inst,
		uint32_t parent,
		uint32_t field);

#endif /*__CESK_RELOC_H__*/
