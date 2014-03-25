/**
 * @file ceks_framediff.h
 * @brief the data structure and utils that used for 
 *        tracking the modification of the frame
 **/
#ifndef __CESK_FRAMEDIFF_H__
#define __CESK_FRAMEDIFF_H__
#include <cesk/frame.h>
#include <dalvik/dalvik_instruction.h>
/** @brief type of frame diff items */
enum {
	CESK_FRAMEDIFF_SUBS,    /*!< the old value is sustituded by the new value */
	CESK_FRAMEDIFF_MERGE    /*!< the new value will be added to the old value */
};
/** @brief type of frame diff object addressing */
enum {
	CESK_FRAMEDIFF_NEW,    /*!< this address refer to a new address that is not exist in the old store */
	CESK_FRAMEDIFF_STORE,  /*!< this address is a address in store */
	CESK_FRAMEDIFF_REG     /*!< this address is a register */
};

/** @brief an item descript a diff */
typedef struct {
	uint8_t  type:1;       /*!< the type of this item */

} cesk_framediff_item_t;
typedef struct {

} cesk_framediff_t;
#endif
