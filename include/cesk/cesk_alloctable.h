/**
 * @file cesk_alloc_table.h
 * @brief The global allocation table, this file is a part of intra-procedure analyzer.
 * 		  In the intra-procedure analyzer, the terminology 'global' means the object is
 * 		  owned by all abstact frames that is involved in this function
 * @details The allocation table is a table that keeps tracking the newly allocated store
 * 			address in each store. The function of this table is 
 * 			
 * 			1. keep tracking the alloction in each store
 *
 * 			2. query the relocated address using an store object address 
 *
 * 			3. get all newly allocated object in this store
 *
 * 			4. flush relocated address to normal address space, clean the record that is
 * 			   relocated to the store
 *
 * @todo    modify the store fork function, so that each newly forked store contains no relocated address
 */
#ifndef __CESK_ALLOC_TABLE_H__
#define __CESK_ALLOC_TABLE_H__
/**
 * @brief the allocation table object 
 **/
typedef struct _cesk_alloc_table_t cesk_alloc_table_t;
#include <cesk/cesk_store.h>
#include <cesk/cesk_addr_arithmetic.h>
/**
 * @brief create a new allocation table
 * @return the pointer to the allocation table
 **/
cesk_alloc_table_t* cesk_alloc_table_new();

/**
 * @brief free the memory used by the allocation table
 * @param mem the allocatoin table object
 * @return nothing
 **/
void cesk_alloc_table_free(cesk_alloc_table_t* mem);

/**
 * @brief insert an allocation record to the table
 * @param table the allocation table
 * @param store the store that is operated
 * @param val_addr the key address
 * @param key_addr  the val address
 * @return the result of insertion <0 means error
 **/
int cesk_alloc_table_insert(
		cesk_alloc_table_t* table,
		const cesk_store_t* store, 
		uint32_t key_addr, 
		uint32_t val_addr);

/**
 * @brief query an relocation address using object address
 * @param table the allocation table
 * @param store the frame store we want to query
 * @param addr the object address
 * @return the relocated address in that store, CESK_STORE_ADDR_NULL 
 * 		   means no record in this address
 * @note if the input is an object address, the return address is an 
 * 		   relocated address. If the input address is a relocated address
 * 		   the return address is an object address
 **/
uint32_t cesk_alloc_table_query(
		const cesk_alloc_table_t* table,
		const cesk_store_t* store,
		uint32_t addr);


#endif
