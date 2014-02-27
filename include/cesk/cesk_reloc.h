#ifndef __CESK_RELOC_H__
#define __CESK_RELOC_H__
/**
 * @file  cesk_reloc.c
 * @brief the relocation support for store
 * @detials Altough we guarantee that all allocation performed by different instruction
 * 			will return a different address in same store. 
 *
 * 			But when we merge two store, the conflict is possible and this conflict will
 * 			break the rule of address allocation. 
 *
 * 			The solution to this issue is to relocate one conflict address during the merge
 * 			procedure. 
 *
 * 			This file contains utils for relocate a confict address. Include a relocate table
 * 			which we shoud aquire first before we actually merge two store
 */
/* previous defination */
/** @brief the relocation table */
typedef struct _cesk_reloc_table_t cesk_reloc_table_t;

#include <cesk/cesk_store.h>
/** @brief build a relocation table from two stores. 
 *  @details this function will check the differences between two stores, and find all conflict and 
 *  		 return a conflict table as the result
 *  @param dest pointer to destination store, because the address of the destination may change during this
 *  @param sour the source store
 *  @return the relocation table
 */
cesk_reloc_table_t* cesk_reloc_table_from_store(cesk_store_t** p_dest, const cesk_store_t* sour);
/** @brief free a relocation table
 *  @param table the relocation table to be despose
 *  @return nothing 
 */
void cesk_reloc_table_free(cesk_reloc_table_t* table);

/** @brief look for the relocation table, if the address should be relocated, return the relocated address
 * 		   otherwise return the input address
 * 	@param table the relocation table
 * 	@param addr  address
 * 	@return a store address, CESK_STORE_ADDR_NULL when error
 */
uint32_t cesk_reloc_table_look_for(const cesk_reloc_table_t* table, uint32_t addr);
#endif
