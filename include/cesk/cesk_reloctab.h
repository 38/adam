/**
 * @file cesk_reloctable.h
 * @brief relocation table
 * @detials Since different frame has different memory layout, therefore, it's possible that
 * 			two same object using the same address if the each frame allocate the address 
 * 			independently. It has no problem when there's no merge operations, since the address
 * 			space of each frame are independent. However, when we merge two store contains a 
 * 			confilict element, how to handel this case is an issue.
 *
 * 			The solution to this problem is, do not determine the address of the object in the 
 * 			frame, instead of using the store address to access this object, the diff object uses
 * 			the reloc object id to access this. When the diff is applied to the frame, then the
 * 			program try to allocate an fresh address to substitude the reloctable reference.
 *			
 *			The table hold ref count for each item in the list
 */
#ifndef __CESK_RELOCTAB_H__
#define __CESK_RELOCTAB_H__
#include <cesk/cesk_diff.h>
#include <cesk/cesk_frame.h>
#include <cesk/cesk_value.h>
/** @brief the struct of the relocation table */
typedef struct {
	int _capacity; /*!< the capacity of the table */
	int _nxtidx;   /*!< the next index */
	cesk_value_t **table;   /*!< relocation table */
} cesk_reloctab_t;
/**
 * @brief create a new relocation table
 * @return pointer to the newly created relocation table 
 **/
cesk_reloctab_t* cesk_reloctab_new();

/**
 * @brief free memory for the relocation table
 * @param table the table to free
 * @return nothing
 **/
void cesk_reloctab_free(cesk_reloctab_t* table);

/**
 * @brief insert a new value to this table 
 * @param table 
 * @param value the value to insert
 * @return the index of the index of the object < 0 for error
 **/
int cesk_reloctab_insert(cesk_reloctab_t* table, cesk_value_t* value);

/**
 * @brief get the value by index
 * @param table the table contains the value
 * @param index the index of the value
 * @return the pointer to the value(writable but do not need to release, because the value is not aquired from the copy-on-write store)
 * 		   If the return value is NULL, an error occured
 **/
static inline cesk_value_t* cesk_reloctab_get(cesk_reloctab_t* table, int index)
{
	if(NULL == table || index < 0 || index >= table->_nxtidx)
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	return table->table[index];
}
#endif
