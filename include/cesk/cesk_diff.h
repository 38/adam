/**
 * @file ceks_diff.h
 * @brief the data structure and utils that used for 
 *        tracking the modification of the frame
 **/
#ifndef __CESK_DIFF_H__
#define __CESK_DIFF_H__
#include <cesk/cesk_frame.h>
#include <dalvik/dalvik_instruction.h>
/*
 * TODO: a) a data structure to repr the diff between machine states 
 *       b) a method to merge two diffs
 *		 c) function to apply a diff to the store
 *		 d) keep an object list which needs to relocate in each store
 *		 e) reverse diff
 *
 *		 Problem: reverse diff behavious for new object(this is not a problem, because before
 *		          the diff operates the frame, the object must be converted to the store address)
 */
/** @brief type of frame diff items */
enum {
	CESK_DIFF_OPCODE_REUSE,
	CESK_DIFF_OPCODE_SET
};
/** @brief type of frame diff object addressing */
enum {
	CESK_DIFF_VALUE_FIXED,
	CESK_DIFF_VALUE_RELOCATED
};

/** @brief an address that to operate */
typedef struct {
	uint8_t  type:1;       /*!< the type of this item */
	uint32_t addr;         /*!< the address content */
} cesk_diff_addr_t;
/** @brief an value to patch */
typedef struct {
	uint8_t  type:1;       /*!< the type of the value, fixed or relocated */
	union {
		uint32_t      valueid;  /*!< the value id, if it's a relocated value */
		cesk_value_t* valueptr; /*!< the value ptr, if it's a fixed value */
	} value;					/*!< the value */
} cesk_diff_value_t;
/** @brief an patch items */ 
typedef struct _cesk_diff_item_t {
	uint8_t opcode:1;			/*!< the type of operation */
	cesk_diff_addr_t addr;		/*!< the affected address, if it's CESK_STORE_ADDR_NULL in memory, that means, we should allocate an address for it */
	cesk_diff_value_t value;    /*!< the value to be operated */
	struct _cesk_diff_item_t* next; /*!< next item in the list */
} cesk_diff_item_t;
//TODO: how to repr a diff using this item, an array that is sorted by the address
/** @brief reprs an diff */
typedef struct {
	uint32_t nitems; 			/*!< how many items in this diff */
	cesk_diff_item_t* head; /*!< the content head */
	cesk_diff_item_t* tail; /*!< the content tail */
} cesk_diff_t;

/** 
 * @brief make an empty diff
 * @return the pointer to the new diff
 */
cesk_diff_t* cesk_diff_new();
/**
 * @brief dispose a diff 
 * @return nothing
 */
void cesk_diff_free(cesk_diff_t* diff);


/** 
 * @brief push next diff item to end of the diff, this is used for gerneating diff
 * @param diff the diff
 * @param item the next item we want to push
 * @return < 0 for error
 */
int cesk_diff_push_back(cesk_diff_t* diff, cesk_diff_item_t* item);

/**
 * @brief push the next diff item to the begining of the diff list, this is used for generating the reverse of diff
 * @param diff the diff
 * @param item the item to insert
 * @return < 0 for error
 */
int cesk_diff_push_front(cesk_diff_t* diff, cesk_diff_item_t* item);

/** 
 * @brief reduce the diff e.g. {set 1 to 2, set 1 to 3} ==> {set 1 to 3} 
 * @param diff the diff to reduce
 * @return < 0 for error
 */
int cesk_diff_reduce(cesk_diff_t* diff);

/**
 * @brief merge two diff, notice that the merge operator is not commutative, that is merge(A,B) != merge(B,A)
 * @param first the first diff
 * @param second the second diff
 */
cesk_diff_t* cesk_diff_merge(cesk_diff_t* first, cesk_diff_t* second);

/**
 * @brief apply the diff to the frame
 * @param frame the frame to operate
 * @param diff 
 * @return <0 for error
 */
int cesk_diff_apply(cesk_frame_t* frame, cesk_diff_t* diff);
#endif
